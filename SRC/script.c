#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- script module ---- */

/* VM stack for script push/pop operations.
 * The original Watcom binary uses the x86 hardware stack for VM push/pop.
 * IDA decompiler could not represent this, so we use a software stack.
 * Size 256 allows deep nesting of recursive EvaluateScriptBytecode calls. */
static int g_VMStack[256];
static int g_VMStackTop = 0;
static int g_NativeArgsConsumed = 0;

/* CallNativeWithVMStack: Calls a native function, passing VM stack
 * entries as __stdcall arguments on the x86 stack.
 * Uses __declspec(naked) to have full control over the stack.
 * After the call, measures how many bytes the callee cleaned via ret N
 * and stores the count (in DWORDs) in g_NativeArgsConsumed. */
__declspec(naked) int __cdecl CallNativeWithVMStack(
    int (*func)(), int *vmStack, int vmTop)
{
  __asm {
    push ebp
    mov ebp, esp
    push esi
    push edi
    push ebx
    mov esi, [ebp+16]  /* vmTop */
    mov edi, [ebp+12]  /* vmStack */
    /* Push VM stack entries to x86 stack (index 0 first = deepest) */
    xor ecx, ecx
  _push_loop:
    cmp ecx, esi
    jge _do_call
    push dword ptr [edi + ecx*4]
    inc ecx
    jmp _push_loop
  _do_call:
    mov ebx, esp        /* save ESP before call */
    mov eax, [ebp+8]    /* func */
    call eax
    /* __stdcall callee cleaned N bytes via ret N. */
    /* Measure: cleaned_bytes = ESP_after - ESP_before */
    mov ecx, esp        /* ecx = ESP_after */
    sub ecx, ebx        /* ecx = ESP_after - ESP_before (positive, callee cleaned) */
    shr ecx, 2          /* ecx = cleaned_dwords */
    mov dword ptr [g_NativeArgsConsumed], ecx
    /* Restore ESP and return */
    lea esp, [ebp-12]
    pop ebx
    pop edi
    pop esi
    pop ebp
    ret
  }
}

int __stdcall EvaluateScriptBytecode(unsigned __int64 accumulator, BYTE *bytecode)
{
  int _savedVMTop = g_VMStackTop; /* Save VM stack depth for reentrancy */
  int globalBase; // edi
  int objectBase; // ebx
  BYTE *ip; // esi
  int opcode_after_true; // ecx
  int opcode_after_false; // ecx
  BYTE *nextIp; // esi
  int opcode_fetch; // ecx
  int opcode_dispatch; // ecx
  int globalOffset; // eax
  int objectOffset; // eax
  int localOffset; // eax
  int remainder; // et2
  int loadGlobalOff; // eax
  int loadObjectOff; // eax
  int loadGlobalDwOff; // eax
  int loadObjectDwOff; // eax
  int nativeFuncIndex; // eax
  BYTE *savedIp; // [esp-4h] [ebp-10h]
  __int32 localStack[3]; // [esp+0h] [ebp-Ch] BYREF

  globalBase = g_GlobalVarsPtr;
  objectBase = g_CurrentObjectData;
  ip = bytecode + 1;
  switch ( *bytecode )
  {
    case OP_RETURN:
      { g_VMStackTop = _savedVMTop; return accumulator; }
    case OP_CMP_EQ:
op_cmp_eq:
      if ( HIDWORD(accumulator) == (_DWORD)accumulator )
        goto set_true;                          // OP_CMP_EQ: if (hi == lo) -> true
      goto set_false;
    case OP_CMP_NE:
op_cmp_ne:
      if ( HIDWORD(accumulator) == (_DWORD)accumulator )
        goto set_false;                         // OP_CMP_NE: if (hi != lo) -> true
      goto set_true;
    case OP_CMP_LT_SIGNED:
op_cmp_lt_signed:
      if ( SHIDWORD(accumulator) >= (int)accumulator )
        goto set_false;                         // OP_CMP_LT_SIGNED: if (hi < lo) signed
      goto set_true;
    case OP_CMP_LE_SIGNED:
op_cmp_le_signed:
      if ( SHIDWORD(accumulator) > (int)accumulator )
        goto set_false;                         // OP_CMP_LE_SIGNED: if (hi <= lo) signed
      goto set_true;
    case OP_CMP_GT_SIGNED:
op_cmp_gt_signed:
      if ( SHIDWORD(accumulator) <= (int)accumulator )
        goto set_false;                         // OP_CMP_GT_SIGNED: if (hi > lo) signed
      goto set_true;
    case OP_CMP_GE_SIGNED:
op_cmp_ge_signed:
      if ( SHIDWORD(accumulator) < (int)accumulator )
        goto set_false;                         // OP_CMP_GE_SIGNED: if (hi >= lo) signed
      goto set_true;
    case OP_CMP_LT_UNSIGNED:
op_cmp_lt_unsigned:
      if ( HIDWORD(accumulator) >= (unsigned int)accumulator )
        goto set_false;                         // OP_CMP_LT_UNSIGNED: if (hi < lo) unsigned
      goto set_true;
    case OP_CMP_LE_UNSIGNED:
op_cmp_le_unsigned:
      if ( HIDWORD(accumulator) > (unsigned int)accumulator )
        goto set_false;                         // OP_CMP_LE_UNSIGNED: if (hi <= lo) unsigned
      goto set_true;
    case OP_CMP_GT_UNSIGNED:
op_cmp_gt_unsigned:
      if ( HIDWORD(accumulator) <= (unsigned int)accumulator )
        goto set_false;                         // OP_CMP_GT_UNSIGNED: if (hi > lo) unsigned
      goto set_true;
    case OP_CMP_GE_UNSIGNED:
op_cmp_ge_unsigned:
      while ( 2 )
      {
        if ( HIDWORD(accumulator) >= (unsigned int)accumulator )
        {
set_true:
          LODWORD(accumulator) = 1;             // SET_TRUE: acc = 1, then dispatch next opcode
          opcode_after_true = *ip++;
          switch ( opcode_after_true )
          {
            case 0:
              { g_VMStackTop = _savedVMTop; return accumulator; }
            case 1:
              goto op_cmp_eq;
            case 2:
              goto op_cmp_ne;
            case 3:
              goto op_cmp_lt_signed;
            case 4:
              goto op_cmp_le_signed;
            case 5:
              goto op_cmp_gt_signed;
            case 6:
              goto op_cmp_ge_signed;
            case 7:
              goto op_cmp_lt_unsigned;
            case 8:
              goto op_cmp_le_unsigned;
            case 9:
              goto op_cmp_gt_unsigned;
            case 10:
              continue;
            case 11:
              goto op_jz;
            case 12:
              goto op_jmp;
            case 13:
              goto fetch_after_dword;
            case 14:
              goto op_store_byte_global;
            case 15:
              goto op_store_byte_object;
            case 16:
              goto op_store_dword_global;
            case 17:
              goto op_store_dword_object;
            case 18:
              goto op_ret_call;
            case 19:
              goto op_ret_call_skip4;
            case 20:
              goto op_dup;
            case 21:
              goto op_add;
            case 22:
              goto op_mul;
            case 23:
              goto op_or;
            case 24:
              goto op_xor;
            case 25:
              goto op_and;
            case 26:
              goto op_neg;
            case 27:
              goto op_sar;
            case 28:
              goto op_shl;
            case 29:
              goto op_push_imm32;
            case 30:
              goto op_inc;
            case 31:
              goto op_dec;
            case 32:
              goto op_xchg;
            case 33:
              goto op_push_lo;
            case 34:
              goto op_pop_hi;
            case 35:
            case 36:
              goto op_lea_global;
            case 37:
              goto op_lea_object;
            case 38:
              goto op_lea_local;
            case 39:
              goto op_store_byte_ind;
            case 40:
              goto op_store_dword_ind;
            case 41:
              goto op_mul4;
            case 42:
              goto op_add4;
            case 43:
              goto op_sub4;
            case 44:
              goto op_xchg_stack;
            case 45:
              goto op_sub;
            case 46:
              goto op_idiv;
            case 47:
              goto op_load_sbyte_global;
            case 48:
              goto op_load_sbyte_object;
            case 49:
              goto op_load_dword_global;
            case 50:
              goto op_load_dword_object;
            case 51:
              goto op_load_sbyte_ind;
            case 52:
              goto op_load_dword_ind;
            case 53:
              goto op_call_script;
            case 54:
              goto op_call_native;
            case 55:
              goto op_call_indirect;
          }
        }
        else
        {
set_false:
          LODWORD(accumulator) = 0;             // SET_FALSE: acc = 0, then dispatch next opcode
          opcode_after_false = *ip++;
          switch ( opcode_after_false )
          {
            case OP_RETURN:
              { g_VMStackTop = _savedVMTop; return accumulator; }
            case OP_CMP_EQ:
              goto op_cmp_eq;
            case OP_CMP_NE:
              goto op_cmp_ne;
            case OP_CMP_LT_SIGNED:
              goto op_cmp_lt_signed;
            case OP_CMP_LE_SIGNED:
              goto op_cmp_le_signed;
            case OP_CMP_GT_SIGNED:
              goto op_cmp_gt_signed;
            case OP_CMP_GE_SIGNED:
              goto op_cmp_ge_signed;
            case OP_CMP_LT_UNSIGNED:
              goto op_cmp_lt_unsigned;
            case OP_CMP_LE_UNSIGNED:
              goto op_cmp_le_unsigned;
            case OP_CMP_GT_UNSIGNED:
              goto op_cmp_gt_unsigned;
            case OP_CMP_GE_UNSIGNED:
              continue;
            case OP_JZ:
              goto op_jz;
            case OP_JMP:
              goto op_jmp;
            case OP_SKIP4:
              goto fetch_after_dword;
            case OP_STORE_BYTE_GLOBAL:
              goto op_store_byte_global;
            case OP_STORE_BYTE_OBJECT:
              goto op_store_byte_object;
            case OP_STORE_DWORD_GLOBAL:
              goto op_store_dword_global;
            case OP_STORE_DWORD_OBJECT:
              goto op_store_dword_object;
            case OP_RET_CALL:
              goto op_ret_call;
            case OP_RET_CALL_SKIP4:
              goto op_ret_call_skip4;
            case OP_DUP:
              goto op_dup;
            case OP_ADD:
              goto op_add;
            case OP_MUL:
              goto op_mul;
            case OP_OR:
              goto op_or;
            case OP_XOR:
              goto op_xor;
            case OP_AND:
              goto op_and;
            case OP_NEG:
              goto op_neg;
            case OP_SAR:
              goto op_sar;
            case OP_SHL:
              goto op_shl;
            case OP_PUSH_IMM32:
              goto op_push_imm32;
            case OP_INC:
              goto op_inc;
            case OP_DEC:
              goto op_dec;
            case OP_XCHG:
              goto op_xchg;
            case OP_PUSH_LO:
              goto op_push_lo;
            case OP_POP_HI:
              goto op_pop_hi;
            case OP_LEA_GLOBAL:
            case OP_LEA_GLOBAL_ALT:
              goto op_lea_global;
            case OP_LEA_OBJECT:
              goto op_lea_object;
            case OP_LEA_LOCAL:
              goto op_lea_local;
            case OP_STORE_BYTE_IND:
              goto op_store_byte_ind;
            case OP_STORE_DWORD_IND:
              goto op_store_dword_ind;
            case OP_MUL4:
              goto op_mul4;
            case OP_ADD4:
              goto op_add4;
            case OP_SUB4:
              goto op_sub4;
            case OP_XCHG_STACK:
              goto op_xchg_stack;
            case OP_SUB:
              goto op_sub;
            case OP_IDIV:
              goto op_idiv;
            case OP_LOAD_SBYTE_GLOBAL:
              goto op_load_sbyte_global;
            case OP_LOAD_SBYTE_OBJECT:
              goto op_load_sbyte_object;
            case OP_LOAD_DWORD_GLOBAL:
              goto op_load_dword_global;
            case OP_LOAD_DWORD_OBJECT:
              goto op_load_dword_object;
            case OP_LOAD_SBYTE_IND:
              goto op_load_sbyte_ind;
            case OP_LOAD_DWORD_IND:
              goto op_load_dword_ind;
            case OP_CALL_SCRIPT:
              goto op_call_script;
            case OP_CALL_NATIVE:
              goto op_call_native;
            case OP_CALL_INDIRECT:
              goto op_call_indirect;
          }
        }
      }
      { g_VMStackTop = _savedVMTop; return accumulator; }                       // OP_CMP_GE_UNSIGNED: if (hi >= lo) unsigned
    case OP_JZ:
op_jz:
      if ( !(_DWORD)accumulator )
        goto op_jmp;                            // OP_JZ: jump if lo == 0
      goto fetch_after_dword;
    case OP_JMP:
op_jmp:
      nextIp = &ip[*(_DWORD *)ip];              // OP_JMP: ip += *(int*)ip (relative jump)
      goto fetch_next;
    case OP_SKIP4:
      goto fetch_after_dword;
    case OP_STORE_BYTE_GLOBAL:
op_store_byte_global:
      *(_BYTE *)(globalBase + *(_DWORD *)ip) = accumulator;// OP_STORE_BYTE_GLOBAL: global[offset] = (byte)lo
      goto fetch_after_dword;
    case OP_STORE_BYTE_OBJECT:
op_store_byte_object:
      *(_BYTE *)(objectBase + *(_DWORD *)ip) = accumulator;// OP_STORE_BYTE_OBJECT: object[offset] = (byte)lo
      goto fetch_after_dword;
    case OP_STORE_DWORD_GLOBAL:
op_store_dword_global:
      *(_DWORD *)(globalBase + *(_DWORD *)ip) = accumulator;// OP_STORE_DWORD_GLOBAL: global[offset] = lo
      goto fetch_after_dword;
    case OP_STORE_DWORD_OBJECT:
op_store_dword_object:
      *(_DWORD *)(objectBase + *(_DWORD *)ip) = accumulator;// OP_STORE_DWORD_OBJECT: object[offset] = lo
      goto fetch_after_dword;
    case OP_RET_CALL:
op_ret_call:
      if (g_VMStackTop > 0) ip = (BYTE *)g_VMStack[--g_VMStackTop];
      else ip = (BYTE *)localStack[0];
      goto fetch_after_dword;
    case OP_RET_CALL_SKIP4:
op_ret_call_skip4:
      if (g_VMStackTop > 0) ip = (BYTE *)g_VMStack[--g_VMStackTop];
      else ip = (BYTE *)localStack[0];
      { int _adj = *(_DWORD *)ip / 4;
        if (g_VMStackTop >= _adj) g_VMStackTop -= _adj;
        else g_VMStackTop = 0;
      }                                        // OP_RET_CALL_SKIP4: pop IP + adjust VM stack
fetch_after_dword:
      while ( 2 )
      {
        nextIp = ip + 4;                        // FETCH_AFTER_DWORD: ip += 4, then fetch next
fetch_next:
        opcode_fetch = *nextIp;
        ip = nextIp + 1;
        switch ( opcode_fetch )
        {
          case 0:
            { g_VMStackTop = _savedVMTop; return accumulator; }
          case 1:
            goto op_cmp_eq;
          case 2:
            goto op_cmp_ne;
          case 3:
            goto op_cmp_lt_signed;
          case 4:
            goto op_cmp_le_signed;
          case 5:
            goto op_cmp_gt_signed;
          case 6:
            goto op_cmp_ge_signed;
          case 7:
            goto op_cmp_lt_unsigned;
          case 8:
            goto op_cmp_le_unsigned;
          case 9:
            goto op_cmp_gt_unsigned;
          case 10:
            goto op_cmp_ge_unsigned;
          case 11:
            goto op_jz;
          case 12:
            goto op_jmp;
          case 13:
            continue;
          case 14:
            goto op_store_byte_global;
          case 15:
            goto op_store_byte_object;
          case 16:
            goto op_store_dword_global;
          case 17:
            goto op_store_dword_object;
          case 18:
            goto op_ret_call;
          case 19:
            goto op_ret_call_skip4;
          case 20:
            goto op_dup;
          case 21:
            goto op_add;
          case 22:
            goto op_mul;
          case 23:
            goto op_or;
          case 24:
            goto op_xor;
          case 25:
            goto op_and;
          case 26:
            goto op_neg;
          case 27:
            goto op_sar;
          case 28:
            goto op_shl;
          case 29:
            goto op_push_imm32;
          case 30:
            goto op_inc;
          case 31:
            goto op_dec;
          case 32:
            goto op_xchg;
          case 33:
            goto op_push_lo;
          case 34:
            goto op_pop_hi;
          case 35:
          case 36:
            goto op_lea_global;
          case 37:
            goto op_lea_object;
          case 38:
            goto op_lea_local;
          case 39:
            goto op_store_byte_ind;
          case 40:
            goto op_store_dword_ind;
          case 41:
            goto op_mul4;
          case 42:
            goto op_add4;
          case 43:
            goto op_sub4;
          case 44:
            goto op_xchg_stack;
          case 45:
            goto op_sub;
          case 46:
            goto op_idiv;
          case 47:
            goto op_load_sbyte_global;
          case 48:
            goto op_load_sbyte_object;
          case 49:
            goto op_load_dword_global;
          case 50:
            goto op_load_dword_object;
          case 51:
            goto op_load_sbyte_ind;
          case 52:
            goto op_load_dword_ind;
          case 53:
            goto op_call_script;
          case 54:
            goto op_call_native;
          case 55:
            goto op_call_indirect;
        }
      }
      { g_VMStackTop = _savedVMTop; return accumulator; }
    case OP_DUP:
op_dup:
      HIDWORD(accumulator) = accumulator;       // OP_DUP: hi = lo
      goto dispatch_normal;
    case OP_ADD:
op_add:
      LODWORD(accumulator) = HIDWORD(accumulator) + accumulator;// OP_ADD: lo = hi + lo
      goto dispatch_normal;
    case OP_MUL:
op_mul:
      accumulator = HIDWORD(accumulator) * (unsigned __int64)(unsigned int)accumulator;// OP_MUL: acc = hi * lo (64-bit)
      goto dispatch_normal;
    case OP_OR:
op_or:
      LODWORD(accumulator) = HIDWORD(accumulator) | accumulator;// OP_OR: lo = hi | lo
      goto dispatch_normal;
    case OP_XOR:
op_xor:
      LODWORD(accumulator) = HIDWORD(accumulator) ^ accumulator;// OP_XOR: lo = hi ^ lo
      goto dispatch_normal;
    case OP_AND:
op_and:
      LODWORD(accumulator) = HIDWORD(accumulator) & accumulator;// OP_AND: lo = hi & lo
      goto dispatch_normal;
    case OP_NEG:
op_neg:
      LODWORD(accumulator) = -(int)accumulator; // OP_NEG: lo = -lo
      goto dispatch_normal;
    case OP_SAR:
op_sar:
      LODWORD(accumulator) = SHIDWORD(accumulator) >> accumulator;// OP_SAR: lo = hi >> lo (arithmetic)
      goto dispatch_normal;
    case OP_SHL:
op_shl:
      LODWORD(accumulator) = HIDWORD(accumulator) << accumulator;// OP_SHL: lo = hi << lo
      goto dispatch_normal;
    case OP_PUSH_IMM32:
op_push_imm32:
      LODWORD(accumulator) = *(_DWORD *)ip;     // OP_PUSH_IMM32: lo = immediate32
      ip += 4;
      goto dispatch_normal;
    case OP_INC:
op_inc:
      LODWORD(accumulator) = accumulator + 1;   // OP_INC: lo++
      goto dispatch_normal;
    case OP_DEC:
op_dec:
      LODWORD(accumulator) = accumulator - 1;   // OP_DEC: lo--
      goto dispatch_normal;
    case OP_XCHG:
op_xchg:
      accumulator = __PAIR64__(accumulator, HIDWORD(accumulator));// OP_XCHG: swap hi and lo
      goto dispatch_normal;
    case OP_PUSH_LO:
op_push_lo:
      g_VMStack[g_VMStackTop++] = (int)accumulator; // OP_PUSH_LO: push lo to VM stack
      savedIp = (BYTE *)accumulator;
      goto dispatch_normal;
    case OP_POP_HI:
op_pop_hi:
      if (g_VMStackTop > 0) HIDWORD(accumulator) = g_VMStack[--g_VMStackTop];
      else HIDWORD(accumulator) = localStack[0];
      goto dispatch_normal;
    case OP_LEA_GLOBAL:
    case OP_LEA_GLOBAL_ALT:
op_lea_global:
      globalOffset = *(_DWORD *)ip;             // OP_LEA_GLOBAL: lo = &global[offset]
      ip += 4;
      LODWORD(accumulator) = globalBase + globalOffset;
      goto dispatch_normal;
    case OP_LEA_OBJECT:
op_lea_object:
      objectOffset = *(_DWORD *)ip;             // OP_LEA_OBJECT: lo = &object[offset]
      ip += 4;
      LODWORD(accumulator) = objectBase + objectOffset;
      goto dispatch_normal;
    case OP_LEA_LOCAL:
op_lea_local:
      localOffset = *(_DWORD *)ip;              // OP_LEA_LOCAL: lo = &local[offset]
      ip += 4;
      LODWORD(accumulator) = (char *)localStack + localOffset;
      goto dispatch_normal;
    case OP_STORE_BYTE_IND:
op_store_byte_ind:
      *(_BYTE *)HIDWORD(accumulator) = accumulator;// OP_STORE_BYTE_IND: *(byte*)hi = lo
      goto dispatch_normal;
    case OP_STORE_DWORD_IND:
op_store_dword_ind:
      *(_DWORD *)HIDWORD(accumulator) = accumulator;// OP_STORE_DWORD_IND: *(dword*)hi = lo
      goto dispatch_normal;
    case OP_MUL4:
op_mul4:
      LODWORD(accumulator) = 4 * accumulator;   // OP_MUL4: lo = lo * 4
      goto dispatch_normal;
    case OP_ADD4:
op_add4:
      LODWORD(accumulator) = accumulator + 4;   // OP_ADD4: lo = lo + 4
      goto dispatch_normal;
    case OP_SUB4:
op_sub4:
      LODWORD(accumulator) = accumulator - 4;   // OP_SUB4: lo = lo - 4
      goto dispatch_normal;
    case OP_XCHG_STACK:
op_xchg_stack:
      { int _tmp;
        if (g_VMStackTop > 0) {
          _tmp = g_VMStack[g_VMStackTop-1];
          g_VMStack[g_VMStackTop-1] = (int)accumulator;
        } else {
          _tmp = localStack[0];
          localStack[0] = (int)accumulator;
        }
        LODWORD(accumulator) = _tmp;
      } // OP_XCHG_STACK: xchg lo with VM stack top
      goto dispatch_normal;
    case OP_SUB:
op_sub:
      LODWORD(accumulator) = HIDWORD(accumulator) - accumulator;// OP_SUB: lo = hi - lo
      goto dispatch_normal;
    case OP_IDIV:
op_idiv:
      remainder = SHIDWORD(accumulator) % (int)accumulator;// OP_IDIV: lo = hi / lo, hi = hi % lo (signed)
      LODWORD(accumulator) = SHIDWORD(accumulator) / (int)accumulator;
      HIDWORD(accumulator) = remainder;
      goto dispatch_normal;
    case OP_LOAD_SBYTE_GLOBAL:
op_load_sbyte_global:
      loadGlobalOff = *(_DWORD *)ip;            // OP_LOAD_SBYTE_GLOBAL: lo = (signed char)global[offset]
      ip += 4;
      LODWORD(accumulator) = *(char *)(globalBase + loadGlobalOff);
      goto dispatch_normal;
    case OP_LOAD_SBYTE_OBJECT:
op_load_sbyte_object:
      loadObjectOff = *(_DWORD *)ip;            // OP_LOAD_SBYTE_OBJECT: lo = (signed char)object[offset]
      ip += 4;
      LODWORD(accumulator) = *(char *)(objectBase + loadObjectOff);
      goto dispatch_normal;
    case OP_LOAD_DWORD_GLOBAL:
op_load_dword_global:
      loadGlobalDwOff = *(_DWORD *)ip;          // OP_LOAD_DWORD_GLOBAL: lo = global[offset]
      ip += 4;
      LODWORD(accumulator) = *(_DWORD *)(globalBase + loadGlobalDwOff);
      goto dispatch_normal;
    case OP_LOAD_DWORD_OBJECT:
op_load_dword_object:
      loadObjectDwOff = *(_DWORD *)ip;          // OP_LOAD_DWORD_OBJECT: lo = object[offset]
      ip += 4;
      LODWORD(accumulator) = *(_DWORD *)(objectBase + loadObjectDwOff);
      goto dispatch_normal;
    case OP_LOAD_SBYTE_IND:
op_load_sbyte_ind:
      LODWORD(accumulator) = *(char *)accumulator;// OP_LOAD_SBYTE_IND: lo = *(signed char*)lo
      goto dispatch_normal;
    case OP_LOAD_DWORD_IND:
op_load_dword_ind:
      LODWORD(accumulator) = *(_DWORD *)accumulator;// OP_LOAD_DWORD_IND: lo = *(dword*)lo
      goto dispatch_normal;
    case OP_CALL_SCRIPT:
op_call_script:
      g_VMStack[g_VMStackTop++] = (int)ip;     // OP_CALL_SCRIPT: push IP to VM stack
      savedIp = ip;
      ip = (BYTE *)(globalBase + *(_DWORD *)ip);
      goto dispatch_normal;
    case OP_CALL_NATIVE:
op_call_native:
      nativeFuncIndex = *(_DWORD *)ip;          // OP_CALL_NATIVE: call g_NativeFuncTable[index]()
      ip += 4;
      /* Pass the FULL VM stack (from index 0), not just this invocation's slice.
         Native __stdcall functions read [esp+4..] which maps to the topmost
         VM stack entries. In the original binary, the x86 hardware stack
         accumulates all pushes across nested EvaluateScriptBytecode calls,
         including leaked entries from non-returning natives (e.g. auxMainLoop
         which calls LongjmpRestart). We must replicate that by passing all
         entries, not just those from _savedVMTop onwards. */
      LODWORD(accumulator) = CallNativeWithVMStack(g_NativeFuncTable[nativeFuncIndex],
        g_VMStack, g_VMStackTop);
      g_VMStackTop -= g_NativeArgsConsumed; // Clean consumed args from VM stack
      goto dispatch_normal;
    case OP_CALL_INDIRECT:
op_call_indirect:
      while ( 2 )
      {
        g_VMStack[g_VMStackTop++] = (int)ip;   // OP_CALL_INDIRECT: push IP to VM stack
        savedIp = ip;
        ip = (BYTE *)(HIDWORD(accumulator) + globalBase);
dispatch_normal:
        opcode_dispatch = *ip++;
        switch ( opcode_dispatch )
        {
          case 0:
            { g_VMStackTop = _savedVMTop; return accumulator; }
          case 1:
            goto op_cmp_eq;
          case 2:
            goto op_cmp_ne;
          case 3:
            goto op_cmp_lt_signed;
          case 4:
            goto op_cmp_le_signed;
          case 5:
            goto op_cmp_gt_signed;
          case 6:
            goto op_cmp_ge_signed;
          case 7:
            goto op_cmp_lt_unsigned;
          case 8:
            goto op_cmp_le_unsigned;
          case 9:
            goto op_cmp_gt_unsigned;
          case 10:
            goto op_cmp_ge_unsigned;
          case 11:
            goto op_jz;
          case 12:
            goto op_jmp;
          case 13:
            goto fetch_after_dword;
          case 14:
            goto op_store_byte_global;
          case 15:
            goto op_store_byte_object;
          case 16:
            goto op_store_dword_global;
          case 17:
            goto op_store_dword_object;
          case 18:
            goto op_ret_call;
          case 19:
            goto op_ret_call_skip4;
          case 20:
            goto op_dup;
          case 21:
            goto op_add;
          case 22:
            goto op_mul;
          case 23:
            goto op_or;
          case 24:
            goto op_xor;
          case 25:
            goto op_and;
          case 26:
            goto op_neg;
          case 27:
            goto op_sar;
          case 28:
            goto op_shl;
          case 29:
            goto op_push_imm32;
          case 30:
            goto op_inc;
          case 31:
            goto op_dec;
          case 32:
            goto op_xchg;
          case 33:
            goto op_push_lo;
          case 34:
            goto op_pop_hi;
          case 35:
          case 36:
            goto op_lea_global;
          case 37:
            goto op_lea_object;
          case 38:
            goto op_lea_local;
          case 39:
            goto op_store_byte_ind;
          case 40:
            goto op_store_dword_ind;
          case 41:
            goto op_mul4;
          case 42:
            goto op_add4;
          case 43:
            goto op_sub4;
          case 44:
            goto op_xchg_stack;
          case 45:
            goto op_sub;
          case 46:
            goto op_idiv;
          case 47:
            goto op_load_sbyte_global;
          case 48:
            goto op_load_sbyte_object;
          case 49:
            goto op_load_dword_global;
          case 50:
            goto op_load_dword_object;
          case 51:
            goto op_load_sbyte_ind;
          case 52:
            goto op_load_dword_ind;
          case 53:
            goto op_call_script;
          case 54:
            goto op_call_native;
          case 55:
            continue;
        }
      }
      { g_VMStackTop = _savedVMTop; return accumulator; }
  }
}

#pragma pack(pop)

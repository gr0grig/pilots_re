/*
 * Машина скриптов.
 *
 * Скрипты игры — байт-код с аккумулятором из двух 32-разрядных регистров:
 * lo (результат) и hi (второй операнд). Двухместные операции считают
 * «lo = hi op lo», сравнения кладут в lo 0 или 1. Переменные лежат в двух
 * областях: глобальные (g_GlobalVars) и переменные текущего объекта (g_CurVars);
 * код подпрограмм тоже адресуется смещением от g_GlobalVars.
 *
 * В оригинале интерпретатор написан на ассемблере и пользуется стеком x86
 * напрямую: на нём лежат доводы встроенных функций, локали скриптов и адреса
 * возврата подпрограмм. Здесь стек машины — отдельный массив, растущий вниз,
 * с той же побайтовой адресацией (операнды «esp += n» и «адрес esp + n»
 * задаются в байтах). Встроенные функции снимают свои доводы сами, как
 * __stdcall-функции оригинала.
 */
#include "gm.h"

#include <string.h>

#define VM_STACK_BYTES (256 * 1024)

/* Стек машины. Вложенные запуски (встроенная функция → скрипт) продолжают
   тот же стек ниже вызывающего, как это было со стеком x86. */
static int32_t s_vmStack[VM_STACK_BYTES / sizeof(int32_t)];
static uint8_t *const s_vmStackTop = (uint8_t *)s_vmStack + VM_STACK_BYTES;
static uint8_t *s_vmSp = (uint8_t *)s_vmStack + VM_STACK_BYTES;

static int32_t ReadOperand(const uint8_t *ip)
{
    int32_t value;
    memcpy(&value, ip, sizeof(value));
    return value;
}

static void Push(uint8_t **sp, int32_t value)
{
    *sp -= sizeof(int32_t);
    memcpy(*sp, &value, sizeof(value));
}

static int32_t Pop(uint8_t **sp)
{
    int32_t value;
    memcpy(&value, *sp, sizeof(value));
    *sp += sizeof(int32_t);
    return value;
}

/* Сбросить стек машины. Зовётся в точке перезапуска такта: в оригинале longjmp
   на g_RestartJmp восстанавливал ESP и тем самым бросал всё, что скрипты
   успели положить на стек x86. */
void GmVm_ResetStack(void)
{
    s_vmSp = s_vmStackTop;
}

/* Выполнить скрипт с начала code до кода OP_RETURN; результат — lo. */
int GmVm_Run(const uint8_t *code)
{
    uint8_t *const globals = g_GlobalVars;
    uint8_t *const objectVars = g_CurVars;
    uint8_t *const entrySp = s_vmSp;
    uint8_t *sp = entrySp;
    const uint8_t *ip = code;
    int32_t lo = 0;
    /* Особенность оригинала: hi на входе — случайное содержимое EDX. Скрипты
       не читают hi до первой записи в него, поэтому здесь он просто обнулён. */
    int32_t hi = 0;

    for (;;)
    {
        const GM_OPCODE op = (GM_OPCODE)*ip++;
        switch (op)
        {
        case OP_RETURN:
            s_vmSp = entrySp;
            return lo;

        /* Сравнения: lo = (hi op lo). */
        case OP_CMP_EQ:          lo = hi == lo; break;
        case OP_CMP_NE:          lo = hi != lo; break;
        case OP_CMP_LT_SIGNED:   lo = hi < lo; break;
        case OP_CMP_LE_SIGNED:   lo = hi <= lo; break;
        case OP_CMP_GT_SIGNED:   lo = hi > lo; break;
        case OP_CMP_GE_SIGNED:   lo = hi >= lo; break;
        case OP_CMP_LT_UNSIGNED: lo = (uint32_t)hi < (uint32_t)lo; break;
        case OP_CMP_LE_UNSIGNED: lo = (uint32_t)hi <= (uint32_t)lo; break;
        case OP_CMP_GT_UNSIGNED: lo = (uint32_t)hi > (uint32_t)lo; break;
        case OP_CMP_GE_UNSIGNED: lo = (uint32_t)hi >= (uint32_t)lo; break;

        /* Переходы: смещение отсчитывается от начала 4-байтового операнда. */
        case OP_JZ:
            ip += lo == 0 ? ReadOperand(ip) : 4;
            break;
        case OP_JMP:
            ip += ReadOperand(ip);
            break;

        case OP_ADJUST_SP:
            /* Выделить (операнд < 0) или освободить место под локали. */
            sp += ReadOperand(ip);
            ip += 4;
            break;

        case OP_STORE_BYTE_GLOBAL:
            globals[ReadOperand(ip)] = (uint8_t)lo;
            ip += 4;
            break;
        case OP_STORE_BYTE_OBJECT:
            objectVars[ReadOperand(ip)] = (uint8_t)lo;
            ip += 4;
            break;
        case OP_STORE_DWORD_GLOBAL:
            memcpy(globals + ReadOperand(ip), &lo, sizeof(lo));
            ip += 4;
            break;
        case OP_STORE_DWORD_OBJECT:
            memcpy(objectVars + ReadOperand(ip), &lo, sizeof(lo));
            ip += 4;
            break;

        /* Возврат из подпрограммы. Адрес возврата указывает на операнд кода
           вызова, поэтому после возврата 4 байта пропускаются. */
        case OP_RET_CALL:
            ip = (const uint8_t *)(intptr_t)Pop(&sp) + 4;
            break;
        case OP_RET_FREE:
        {
            /* Возврат с освобождением кадра: операнд — размер локалей. */
            const uint8_t *returnIp = (const uint8_t *)(intptr_t)Pop(&sp);
            sp += ReadOperand(ip);
            ip = returnIp + 4;
            break;
        }

        case OP_DUP: hi = lo; break;
        case OP_ADD: lo = (int32_t)((uint32_t)hi + (uint32_t)lo); break;
        case OP_MUL:
        {
            /* Беззнаковое произведение 32 x 32: старшая половина уходит в hi. */
            const uint64_t product = (uint64_t)(uint32_t)hi * (uint32_t)lo;
            lo = (int32_t)(uint32_t)product;
            hi = (int32_t)(uint32_t)(product >> 32);
            break;
        }
        case OP_OR:  lo = hi | lo; break;
        case OP_XOR: lo = hi ^ lo; break;
        case OP_AND: lo = hi & lo; break;
        case OP_NEG: lo = (int32_t)(0u - (uint32_t)lo); break;
        /* Счётчик сдвига берётся по модулю 32, как у команд x86. */
        case OP_SAR: lo = hi >> (lo & 31); break;
        case OP_SHL: lo = (int32_t)((uint32_t)hi << (lo & 31)); break;
        case OP_PUSH_IMM32:
            lo = ReadOperand(ip);
            ip += 4;
            break;
        case OP_INC: lo = (int32_t)((uint32_t)lo + 1); break;
        case OP_DEC: lo = (int32_t)((uint32_t)lo - 1); break;
        case OP_XCHG:
        {
            const int32_t t = lo;
            lo = hi;
            hi = t;
            break;
        }
        case OP_PUSH_LO: Push(&sp, lo); break;
        case OP_POP_HI:  hi = Pop(&sp); break;

        case OP_LEA_GLOBAL:
        case OP_LEA_GLOBAL_ALT:
            lo = (int32_t)(intptr_t)(globals + ReadOperand(ip));
            ip += 4;
            break;
        case OP_LEA_OBJECT:
            lo = (int32_t)(intptr_t)(objectVars + ReadOperand(ip));
            ip += 4;
            break;
        case OP_LEA_LOCAL:
            lo = (int32_t)(intptr_t)(sp + ReadOperand(ip));
            ip += 4;
            break;

        case OP_STORE_BYTE_IND:
            *(uint8_t *)(intptr_t)hi = (uint8_t)lo;
            break;
        case OP_STORE_DWORD_IND:
            memcpy((void *)(intptr_t)hi, &lo, sizeof(lo));
            break;

        case OP_MUL4: lo = (int32_t)((uint32_t)lo << 2); break;
        case OP_ADD4: lo = (int32_t)((uint32_t)lo + 4); break;
        case OP_SUB4: lo = (int32_t)((uint32_t)lo - 4); break;
        case OP_XCHG_STACK:
        {
            int32_t top;
            memcpy(&top, sp, sizeof(top));
            memcpy(sp, &lo, sizeof(lo));
            lo = top;
            break;
        }
        case OP_SUB: lo = (int32_t)((uint32_t)hi - (uint32_t)lo); break;
        case OP_IDIV:
        {
            /* lo = hi / lo, hi = hi % lo; деление на ноль, как и в оригинале,
               завершается исключением процессора. */
            const int32_t dividend = hi;
            const int32_t divisor = lo;
            lo = dividend / divisor;
            hi = dividend % divisor;
            break;
        }

        case OP_LOAD_SBYTE_GLOBAL:
            lo = (int8_t)globals[ReadOperand(ip)];
            ip += 4;
            break;
        case OP_LOAD_SBYTE_OBJECT:
            lo = (int8_t)objectVars[ReadOperand(ip)];
            ip += 4;
            break;
        case OP_LOAD_DWORD_GLOBAL:
            memcpy(&lo, globals + ReadOperand(ip), sizeof(lo));
            ip += 4;
            break;
        case OP_LOAD_DWORD_OBJECT:
            memcpy(&lo, objectVars + ReadOperand(ip), sizeof(lo));
            ip += 4;
            break;
        case OP_LOAD_SBYTE_IND:
            lo = *(const int8_t *)(intptr_t)lo;
            break;
        case OP_LOAD_DWORD_IND:
            memcpy(&lo, (const void *)(intptr_t)lo, sizeof(lo));
            break;

        /* Вызов подпрограммы: на стек кладётся адрес операнда кода вызова. */
        case OP_CALL_SCRIPT:
            Push(&sp, (int32_t)(intptr_t)ip);
            ip = globals + ReadOperand(ip);
            break;
        case OP_CALL_INDIRECT:
            Push(&sp, (int32_t)(intptr_t)ip);
            ip = globals + hi;
            break;

        case OP_CALL_NATIVE:
        {
            const GM_NATIVE *native = &g_NativeFunctions[ReadOperand(ip)];
            ip += 4;
            /* Вложенный запуск машины из встроенной функции продолжит стек ниже. */
            s_vmSp = sp;
            lo = native->invoke((const int32_t *)sp);
            sp += native->argCount * (int)sizeof(int32_t);
            s_vmSp = entrySp;
            /* Особенность оригинала: после вызова встроенной функции в hi
               оставался испорченный ею EDX; скрипты его не читают, здесь hi
               сохраняется. */
            break;
        }
        }
    }
}

/* Выполнить скрипт класса (создания/уничтожения) от имени заданного объекта:
   подставить контекст скрипта (класс, объект, его переменные, тайл, направление),
   запустить машину и вернуть прежний контекст. Пустой скрипт ничего не делает. */
void GmRunScriptWithContext(uint8_t *script, GM_CLASS *cls, GM_OBJECT *obj, int objectIndex,
                            uint8_t *vars, uint8_t tileX, uint8_t tileY, uint8_t direction)
{
    if (script == NULL)
        return;

    GM_CLASS *const savedClass = g_CurClass;
    GM_OBJECT *const savedObject = g_CurObject;
    const int savedObjectIndex = g_CurObjectIndex;
    uint8_t *const savedVars = g_CurVars;
    const int savedTileX = g_CurTileX;
    const int savedTileY = g_CurTileY;
    const int savedRuleTileX = g_RuleTileX;
    const int savedRuleTileY = g_RuleTileY;
    const GM_DIRECTION savedDirection = g_CurDirection;

    g_CurClass = cls;
    g_CurObject = obj;
    g_CurObjectIndex = objectIndex;
    g_CurVars = vars;
    g_CurTileX = g_RuleTileX = tileX;
    g_CurTileY = g_RuleTileY = tileY;
    g_CurDirection = direction;

    GmVm_Run(script);

    g_CurDirection = savedDirection;
    g_RuleTileY = savedRuleTileY;
    g_RuleTileX = savedRuleTileX;
    g_CurTileY = savedTileY;
    g_CurTileX = savedTileX;
    g_CurVars = savedVars;
    g_CurObjectIndex = savedObjectIndex;
    g_CurObject = savedObject;
    g_CurClass = savedClass;
}

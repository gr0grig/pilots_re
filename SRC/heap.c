/*
 * Собственная куча движка.
 *
 * Куча занимает арену, отведённую под игру (g_Arena.freePtr / freeBytes).
 * Снизу вверх: постоянные захваты загрузчика (GmHeap_Reserve), затем блоки
 * GmHeap_Alloc. Блок — заголовок-двойное слово (длина блока в двойных словах
 * вместе с заголовком, разряд 31 — блок занят) и данные. Последний блок,
 * начинающийся с g_HeapTop, — свободная вершина длиной g_HeapTopFreeDwords.
 * Сверху вниз от конца арены растёт пул записей (g_PoolBottom).
 */
#include "gm.h"

#include <string.h>

/* Длина блока по заголовку (без признака занятости). */
static uint32_t GmHeap_BlockDwords(uint32_t header)
{
    return header & GMHEAP_SIZE_MASK;
}

/* Обнулить состояние кучи и пула перед заведением новой арены. */
void GmHeap_ClearState(void)
{
    g_PoolBottom = NULL;
    g_PoolTop = NULL;
    g_HeapTop = NULL;
    g_HeapBase = NULL;
    g_HeapTopFreeDwords = 0;
}

/*
 * Завести кучу на арене: начало и вершина — g_Arena.freePtr, свободно freeBytes / 4
 * двойных слов. Возвращает начало кучи (по нему загрузчик кладёт таблицу смещений фильмов).
 */
uint32_t *GmHeap_Init(void)
{
    g_HeapTopFreeDwords = (uint32_t)g_Arena.freeBytes >> 2;
    g_HeapBase = (uint32_t *)g_Arena.freePtr;
    g_HeapTop = (uint32_t *)g_Arena.freePtr;
    return g_HeapTop;
}

/*
 * Постоянный захват size байт (с округлением до 4) с вершины кучи: вершина сдвигается,
 * начало кучи переносится на неё, захваченное больше не освобождается.
 * Возвращает новую вершину, то есть конец захваченного куска — загрузчик пользуется
 * ею как указателем на следующий свободный байт. При нехватке памяти — ошибка
 * и g_Arena.freePtr.
 */
uint8_t *GmHeap_Reserve(uint32_t size)
{
    uint32_t dwords = (size + 3) >> 2;

    if (dwords >= g_HeapTopFreeDwords)
    {
        GmError_OutOfMemory();
        return g_Arena.freePtr;
    }

    g_HeapTopFreeDwords -= dwords;
    g_HeapTop += dwords;
    g_HeapBase = g_HeapTop;
    return (uint8_t *)g_HeapTop;
}

/*
 * Выделить блок: первый свободный блок, строго больший нужного, делится на занятую
 * часть и свободный остаток. Если делится вершинный блок, вершина переносится на остаток.
 * При нехватке памяти — ошибка и NULL.
 */
void *GmHeap_Alloc(uint32_t size)
{
    uint32_t needed = (size >> 2) + ((size & 3) != 0) + 1;   /* данные + заголовок */
    uint32_t *block = g_HeapBase;
    uint32_t header = *block;

    /* Особенность оригинала: блок ровно нужной длины не подходит (сравнение строгое),
       поэтому остаток после деления всегда непуст. */
    while ((header & GMHEAP_USED) != 0 || header <= needed)
    {
        if (block >= g_HeapTop)
            return GmError_OutOfMemory();
        block += GmHeap_BlockDwords(header);
        header = *block;
    }

    block[needed] = header - needed;
    if (block == g_HeapTop)
    {
        g_HeapTop = &block[needed];
        g_HeapTopFreeDwords = header - needed;
    }
    *block = needed | GMHEAP_USED;
    return block + 1;
}

/*
 * Освободить блок кучи: найти его проходом от начала кучи и слить со свободными
 * соседями (следующим и предыдущим). Если поглощена свободная вершина — опустить вершину.
 */
void GmHeap_Free(void *block)
{
    uint32_t *header = (uint32_t *)block - 1;
    uint32_t *cur = g_HeapBase;
    uint32_t *prev = NULL;
    uint32_t *next;
    uint32_t mergedDwords;

    while (cur != header)
    {
        prev = cur;
        cur += GmHeap_BlockDwords(*cur);
    }

    mergedDwords = GmHeap_BlockDwords(*cur);
    next = cur + mergedDwords;
    if ((*next & GMHEAP_USED) == 0)
        mergedDwords += *next;
    else
        next = NULL;

    if (prev != NULL && (*prev & GMHEAP_USED) == 0)
    {
        mergedDwords += *prev;
        cur = prev;
    }

    if (next == g_HeapTop)
    {
        g_HeapTop = cur;
        g_HeapTopFreeDwords = mergedDwords;
    }
    *cur = mergedDwords;
}

/*
 * Временный буфер под упакованные данные (не выделяется и не освобождается):
 * до 0x98 байт — статический g_SmallTempBuffer, иначе size байт под нижней границей
 * пула или, если пула ещё нет, в конце свободной вершины кучи.
 */
uint8_t *GmTempBuffer(uint32_t size)
{
    if (size <= sizeof(g_SmallTempBuffer))
        return g_SmallTempBuffer;
    if (g_PoolBottom != NULL)
        return (uint8_t *)g_PoolBottom - size;
    if (g_HeapTop != NULL)
        return (uint8_t *)&g_HeapTop[g_HeapTopFreeDwords] - size;

    /* Особенность оригинала: без пула и кучи возвращался мусор из регистра.
       На деле недостижимо — куча заводится раньше первого чтения записи. */
    return NULL;
}

/* Освободить блок кучи, если указатель не NULL. */
void GmHeap_FreeIfSet(void *block)
{
    if (block != NULL)
        GmHeap_Free(block);
}

/* Обнулить size байт буфера; возвращает buffer. */
void *GmMemZero(void *buffer, uint32_t size)
{
    return memset(buffer, 0, size);
}

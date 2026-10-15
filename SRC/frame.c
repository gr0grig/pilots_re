/*
 * Кадровый буфер: заведение, копирование фона, вывод спрайтов с прозрачностью,
 * заливка и вывод в окно.
 *
 * Буфер — DIB «снизу вверх»: экранная строка y лежит по смещению
 * (g_FrameHeight − 1 − y) · g_FrameStride, поэтому переход к следующей экранной строке —
 * шаг назад на g_FrameStride. В оригинале функции вывода написаны на ассемблере.
 * Циклы по строкам и столбцам, как и там, выполняются хотя бы один раз.
 */
#include "gm.h"

#include <string.h>

/* Адрес пикселя (x, y) экранных координат в кадровом буфере. */
static uint8_t *FramePixel(int x, int y)
{
    return g_FrameBits + (g_FrameHeight - 1 - y) * g_FrameStride + x;
}

/* Пустая стадия запуска модуля кадрового буфера (цепочка GmGame_Initialize). */
bool GmFrameModule_Startup(void)
{
    return true;
}

/* Пустая стадия завершения модуля кадрового буфера (зовётся из GmGame_Shutdown). */
void GmFrameModule_Shutdown(void)
{
}

/* Завести кадровый буфер размером с экран игры; если DIB-секция не создана
   (arena->frameBuffer пуст) — взять frameBufferSize байт с начала свободной арены. */
bool GmFrame_Init(GM_ARENA *arena, GM_GAME_HEADER *header)
{
    g_FrameStride = header->screenWidth;
    g_FrameHeight = header->screenHeight;
    g_FrameBits = arena->frameBuffer;
    if (!g_FrameBits)
    {
        g_FrameBits = arena->freePtr;
        arena->frameBuffer = g_FrameBits;
        arena->freePtr += arena->frameBufferSize;
        arena->freeBytes -= arena->frameBufferSize;
    }
    return true;
}

/* Скопировать прямоугольник [left,right)×[top,bottom) фона комнаты src (строка srcStride
   байт, начало координат фона — (srcOriginX, srcOriginY)) в кадровый буфер.
   Довод srcHeight не используется. */
void GmFrame_CopyRect(uint8_t *src, int srcOriginX, int srcOriginY, int srcStride, int srcHeight,
                      int left, int top, int right, int bottom)
{
    int width = right - left;
    int rows = bottom - top;
    uint8_t *dst = FramePixel(left, top);

    (void)srcHeight;
    src += -srcOriginY * srcStride - srcOriginX;
    do
    {
        memcpy(dst, src, (size_t)width);
        dst -= g_FrameStride;
        src += srcStride;
    } while (--rows > 0);
}

/*
 * Четыре функции вывода спрайта: прозрачность по байту 0, обрезка прямоугольником
 * [left,right)×[top,bottom). pixels — изображение width×height; skipX/skipY ≤ 0 — сколько
 * столбцов/строк изображения отрезано слева/сверху. Ширина выводимой строки запоминается
 * в g_BlitRowWidth.
 *
 * Общее ядро: вывести строку из rowWidth пикселей, читая источник с шагом srcStep (±1).
 * Возвращает адрес источника за последним прочитанным пикселем.
 */
static const uint8_t *DrawSpriteRow(uint8_t *dst, const uint8_t *src, int srcStep)
{
    int cols = g_BlitRowWidth;

    do
    {
        uint8_t pixel = *src;
        src += srcStep;
        if (pixel)
            *dst = pixel;
        dst++;
    } while (--cols != 0);
    return src;
}

/* Общий цикл по строкам: после строки источник сдвигается на srcRowSkip. */
static void DrawSpriteRows(const uint8_t *src, int srcStep, int srcRowSkip,
                           int left, int top, int right, int bottom)
{
    uint8_t *dst = FramePixel(left, top);
    int rows = bottom - top;

    g_BlitRowWidth = right - left;
    do
    {
        src = DrawSpriteRow(dst, src, srcStep) + srcRowSkip;
        dst -= g_FrameStride;
    } while (--rows > 0);
}

/* Вывести спрайт без отражения. */
void GmFrame_DrawSprite(uint8_t *pixels, int skipX, int skipY, int width, int height,
                        int left, int top, int right, int bottom)
{
    (void)height;
    DrawSpriteRows(pixels - skipY * width - skipX, 1, width - (right - left),
                   left, top, right, bottom);
}

/* Вывести спрайт, отражённый по вертикали (строки изображения с конца). */
void GmFrame_DrawSpriteMirrorY(uint8_t *pixels, int skipX, int skipY, int width, int height,
                               int left, int top, int right, int bottom)
{
    DrawSpriteRows(pixels + (height - 1 + skipY) * width - skipX, 1, -(width + (right - left)),
                   left, top, right, bottom);
}

/* Вывести спрайт, отражённый по горизонтали (пиксели строки с конца). */
void GmFrame_DrawSpriteMirrorX(uint8_t *pixels, int skipX, int skipY, int width, int height,
                               int left, int top, int right, int bottom)
{
    (void)height;
    DrawSpriteRows(pixels - skipY * width + width - 1 + skipX, -1, width + (right - left),
                   left, top, right, bottom);
}

/* Вывести спрайт, отражённый по обеим осям. */
void GmFrame_DrawSpriteMirrorXY(uint8_t *pixels, int skipX, int skipY, int width, int height,
                                int left, int top, int right, int bottom)
{
    DrawSpriteRows(pixels + (height - 1 + skipY) * width + width - 1 + skipX, -1,
                   -(width - (right - left)), left, top, right, bottom);
}

/*
 * Залить весь кадровый буфер цветом color (двойными словами: хвост размера, не кратный 4,
 * не трогается).
 * Особенность оригинала: узор собирался в регистре, где заполнялись только младшие
 * 16 разрядов, а старшие оставались от вызывающего и подмешивались (OR) в два старших
 * байта каждого двойного слова. Здесь — цвет во всех байтах, как при чистом регистре
 * (практический случай).
 */
void GmFrame_Fill(uint8_t color)
{
    uint32_t dwordCount = (uint32_t)(g_FrameHeight * g_FrameStride) >> 2;

    memset(g_FrameBits, color, (size_t)dwordCount * 4);
}

/* Вывести прямоугольник кадрового буфера в окно (с растворением при dissolve). */
void GmFrame_Present(int left, int top, int right, int bottom, bool dissolve)
{
    Host_Present(g_FrameBits, left, top, right - left, bottom - top, g_FrameStride, g_FrameHeight,
                 dissolve);
}

/* Пустая стадия завершения модуля блитов фильма (зовётся из GmGame_Shutdown). */
void GmBlitModule_Shutdown(void)
{
}

/*
 * Печать текста шрифтовой графикой и ввод строки с клавиатуры.
 *
 * Текст — поток байтов: обычные знаки выводятся глифами шрифта (каждый глиф — экранный
 * спрайт текущего объекта), байт GMTEXT_FIELD начинает поле: при печати — подстановку
 * значения переменной, при вводе — поле ввода, куда пишется набранная строка.
 */
#include "gm.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Прочитать 32-разрядное смещение поля из текстового потока (без выравнивания). */
static int32_t ReadInt32(const uint8_t *p)
{
    int32_t value;
    memcpy(&value, p, sizeof value);
    return value;
}

/* Поставить глиф символа character шрифта fontIndex в (*x, y) как экранный спрайт-глиф
   текущего объекта. *x сдвигается на ширину кадра глифа минус его горячую точку. */
GM_SPRITE *GmText_PutGlyph(int character, int fontIndex, int *x, uint16_t y)
{
    GM_FONT *font = (GM_FONT *)&g_Graphics[fontIndex];
    GM_SPRITE *glyph = &GmPool_Alloc()->sprite;
    GM_FRAME *frame;

    glyph->flags |= GMSPR_TEXT_GLYPH | GMSPR_SCREEN_ANCHORED | GMSPR_VISIBLE;
    glyph->frameIndex = 0;
    glyph->frameCount = 1;
    glyph->layer = g_CurObject->layer;
    glyph->fromTile.y = 0xFF;
    glyph->fromTile.x = 0xFF;
    glyph->ownerIndex = (uint16_t)g_CurObjectIndex;
    glyph->x = (int16_t)*x;
    glyph->y = (int16_t)y;

    frame = &font->glyphs[character - font->firstChar];
    glyph->frame = frame;
    *x += frame->image->width - frame->hotX;

    GmSprite_Invalidate(glyph);
    return glyph;
}

/* Начать вывод текста: убрать прежний текст объекта и поднять признак «есть текст».
   Возвращает text. */
uint8_t *GmText_Begin(uint8_t *text)
{
    Native_RemoveText();
    g_CurObject->underTile.flags |= GMUT_HAS_TEXT;
    return text;
}

/* Стереть count (не меньше 1) последних введённых знаков, сдвигая перо назад. */
void GmText_EraseChars(int count)
{
    do
    {
        GM_SPRITE *glyph = g_TextGlyphSprites[--g_TextInputLength];
        g_TextX -= glyph->frame->image->width - glyph->frame->hotX;
        GmSprite_Destroy(glyph);
    } while (--count != 0);
}

/* Записать набранную строку в переменную поля ввода по виду поля. */
static void StoreInputValue(void)
{
    g_TextInputBuffer[g_TextInputLength] = '\0';
    switch (g_TextInputMode & GMFMT_KIND_MASK)
    {
    case GMFMT_SBYTE:
        *g_TextInputTarget = (char)atoi(g_TextInputBuffer);
        break;
    case GMFMT_STRING_PTR:
        strcpy(*(char **)g_TextInputTarget, g_TextInputBuffer);
        break;
    case GMFMT_INLINE_STRING:
        strcpy(g_TextInputTarget, g_TextInputBuffer);
        break;
    case GMFMT_INT:
        *(int32_t *)g_TextInputTarget = atoi(g_TextInputBuffer);
        break;
    case GMFMT_INT_PTR:
        **(int32_t **)g_TextInputTarget = atoi(g_TextInputBuffer);
        break;
    default:
        break;
    }
}

/*
 * Обработчик текстового потока и ввода (key — нажатый знак или 0 для запуска потока).
 * Без активного поля выводит знаки потока до поля ввода или конца. Поле (GMTEXT_FIELD):
 * байт режима (разряды 4–6 — где переменная: глобальные или переменные объекта; разряд 7 —
 * 4-байтовое смещение; разряды 0–2 — вид), байт наибольшей длины, смещение. При активном
 * поле знаки вне диапазона шрифта приводятся регистром, у числовых полей допускаются только
 * цифры; Enter записывает значение и продолжает поток, Backspace стирает знак, Esc — всю
 * строку (на пустой строке — как конец потока). Байт 0 в первый раз поднимает
 * g_TextPending и начинает поток, во второй — завершает его и убирает текст.
 */
uint8_t GmText_Process(uint8_t key)
{
    g_CurObject = g_TextOwnerObject;
    g_CurObjectIndex = g_TextOwnerIndex;
    g_CurClass = g_TextOwnerClass;

    for (;;)
    {
        if (g_TextInputActive)
        {
            if (key == '\b')
            {
                if (g_TextInputLength)
                    GmText_EraseChars(1);
                return 1;
            }
            if (key == '\x1B' && g_TextInputLength)
            {
                GmText_EraseChars(g_TextInputLength);
                return 1;
            }
        }

        /* Конец потока (или Esc на пустом поле ввода). */
        if (key == '\0' || (key == '\x1B' && g_TextInputActive))
        {
            if (g_TextPending)
            {
                g_TextPending = false;
                return (uint8_t)Native_RemoveText();
            }
            g_TextPending = true;
            g_TextInputActive = false;
            key = *g_TextCursor++;
            continue;
        }

        if (key == '\r' && g_TextInputActive)
        {
            StoreInputValue();
            g_TextInputActive = false;
            key = *g_TextCursor++;
            continue;
        }

        /* Особенность оригинала: GMTEXT_FIELD распознаётся и как нажатая клавиша при
           активном поле — тогда описание поля читается из продолжения потока. */
        if (key == GMTEXT_FIELD)
        {
            int source;

            g_TextInputMode = (int8_t)*g_TextCursor++;
            g_TextInputMaxLength = *g_TextCursor++;
            source = g_TextInputMode & GMFMT_SOURCE_MASK;
            if (source == GMFMT_FROM_GLOBALS || source == GMFMT_FROM_OBJECT)
            {
                g_TextInputTarget = (char *)(source == GMFMT_FROM_GLOBALS ? g_GlobalVars : g_CurVars);
                if (g_TextInputMode >= 0)
                {
                    g_TextInputTarget += *g_TextCursor++;
                }
                else
                {
                    g_TextInputTarget += ReadInt32(g_TextCursor);
                    g_TextCursor += 4;
                }

                switch (g_TextInputMode & GMFMT_KIND_MASK)
                {
                case GMFMT_SBYTE:
                case GMFMT_INT:
                case GMFMT_INT_PTR:
                    g_TextInputDigitsOnly = true;
                    break;
                case GMFMT_STRING_PTR:
                case GMFMT_INLINE_STRING:
                    g_TextInputDigitsOnly = false;
                    break;
                default:
                    break;
                }
                g_TextInputLength = 0;
                g_TextInputActive = true;
                return 1;
            }
            /* Иной источник: сам байт поля обрабатывается как обычный знак. */
        }

        if (g_TextInputActive)
        {
            if (g_TextInputLength < g_TextInputMaxLength)
            {
                if (key < g_TextFont->firstChar)
                    key = (uint8_t)tolower(key);
                if (key > g_TextFont->lastChar)
                    key = (uint8_t)toupper(key);
                if (key >= g_TextFont->firstChar && key <= g_TextFont->lastChar
                    && (!g_TextInputDigitsOnly || isdigit(key)))
                {
                    g_TextInputBuffer[g_TextInputLength] = (char)key;
                    g_TextGlyphSprites[g_TextInputLength] =
                        GmText_PutGlyph(key, g_TextFontIndex, &g_TextX, (uint16_t)g_TextY);
                    g_TextInputLength++;
                }
            }
            return 1;
        }

        GmText_PutGlyph(key, g_TextFontIndex, &g_TextX, (uint16_t)g_TextY);
        key = *g_TextCursor++;
    }
}

/* Начать текстовый поток с вводом в (x, y) шрифтом fontIndex от имени текущего объекта,
   если другой поток не идёт (иначе 0). Пока g_TextPending, такты идут через GmTick_TextMode. */
int GmText_StartInput(uint8_t *text, int fontIndex, int x, int y)
{
    if (g_TextPending)
        return 0;

    g_TextCursor = GmText_Begin(text);
    g_TextX = x;
    g_TextY = y;
    g_TextFontIndex = fontIndex;
    g_TextFont = (GM_FONT *)&g_Graphics[fontIndex];
    g_TextOwnerObject = g_CurObject;
    g_TextOwnerIndex = g_CurObjectIndex;
    g_TextOwnerClass = g_CurClass;
    return (int8_t)GmText_Process(0);
}

/*
 * Напечатать текст шрифтом fontIndex с (x, y) с подстановками GMTEXT_FIELD: байт описания
 * (разряды 4–6 — источник: глобальная переменная, переменная объекта, встроенная функция;
 * разряд 7 — 4-байтовое смещение; разряды 0–2 — вид: знаковый байт, строка по указателю,
 * строка на месте, int, int по указателю, вызов скрипта), байт наибольшей длины, смещение
 * (у встроенной функции — её номер). Возвращает 1.
 */
int GmText_Print(uint8_t *text, int fontIndex, int x, uint16_t y)
{
    uint8_t *cursor = GmText_Begin(text);
    GM_FORMAT_BUFFER buf;

    /* При виде поля 6 или 7 буфер не заполняется и печатается прежнее содержимое;
       в оригинале до первого заполнения это мусор стека — здесь пустая строка. */
    buf.text[0] = '\0';

    while (*cursor)
    {
        uint8_t descriptor;
        size_t maxLen;
        int source;
        uint8_t *varBase;
        uint8_t *var;
        const char *glyphText;

        if (*cursor != GMTEXT_FIELD)
        {
            GmText_PutGlyph(*cursor++, fontIndex, &x, y);
            continue;
        }

        descriptor = cursor[1];
        maxLen = cursor[2];
        cursor += 3;
        source = descriptor & GMFMT_SOURCE_MASK;

        if (source == GMFMT_FROM_NATIVE)
        {
            /* Особенность оригинала: результат встроенной функции переводится в строку,
               но не печатается. Функция вызывается без доводов (в данных такие поля
               ссылаются только на функции без доводов), поэтому args не нужен. */
            int32_t result = g_NativeFunctions[*cursor++].invoke(NULL);
            _itoa(result, buf.text, 10);
            continue;
        }
        if (source == GMFMT_FROM_GLOBALS)
            varBase = g_GlobalVars;
        else if (source == GMFMT_FROM_OBJECT)
            varBase = g_CurVars;
        else
            continue; /* Неизвестный источник: поле пропускается без смещения. */

        if ((int8_t)descriptor < 0) /* разряд 7: смещение — 4 байта */
        {
            var = varBase + ReadInt32(cursor);
            cursor += 4;
        }
        else
        {
            var = varBase + *cursor++;
        }

        switch (descriptor & GMFMT_KIND_MASK)
        {
        case GMFMT_SBYTE:
            _itoa(*(int8_t *)var, buf.text, 10);
            break;
        case GMFMT_STRING_PTR:
            strncpy(buf.text, *(const char **)var, maxLen);
            break;
        case GMFMT_INLINE_STRING:
            strncpy(buf.text, (const char *)var, maxLen);
            break;
        case GMFMT_INT:
            _itoa(*(int32_t *)var, buf.text, 10);
            break;
        case GMFMT_INT_PTR:
            _itoa(**(int32_t **)var, buf.text, 10);
            break;
        case GMFMT_SCRIPT_CALL:
            /* Крошечная программа: вызов скрипта по смещению от начала глобальных
               переменных (даже для переменной объекта) и возврат. */
            buf.call.opcode = OP_CALL_SCRIPT;
            buf.call.scriptOffset = (int)(var - g_GlobalVars);
            buf.call.terminator = OP_RETURN;
            _itoa(GmVm_Run((const uint8_t *)&buf.call), buf.text, 10);
            break;
        default:
            break;
        }

        buf.text[maxLen] = '\0';
        for (glyphText = buf.text; *glyphText; glyphText++)
            GmText_PutGlyph((uint8_t)*glyphText, fontIndex, &x, y);
    }
    return 1;
}

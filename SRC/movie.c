/*
 * Проигрыватель собственных фильмов движка и добавочные блиттеры кадров.
 *
 * Фильм — последовательность кусков: 12-байтовый заголовок GM_MOVIE_CHUNK и данные
 * (arg0 байт в файле; если arg0 ≠ arg1 — сжаты LZSS до arg1 байт). Кадр — список
 * прямоугольников-дельт, которые прибавляются к кадровому буферу (значения — дельты
 * индексов палитры, сложение по модулю 256).
 */
#include "gm.h"

#include <stdio.h>

/* Метка конца фильма «[-=VS=-]» в доводах служебного куска подтипа GMMH_END_MARK. */
#define GM_MOVIE_END_MARK_LO 0x563D2D5Bu /* "[-=V" */
#define GM_MOVIE_END_MARK_HI 0x5D2D3D53u /* "S=-]" */

/* Начать фильм: сбросить состояние проигрывателя, размер кадра = экран, остановить
   музыку и перейти к смещению фильма в файле. */
bool GmMovie_Begin(int fileOffset)
{
    g_MovieRepeatCount = 1;
    g_MovieY = 0;
    g_MovieX = 0;
    g_MovieMusicBufSize = 0;
    g_MovieSoundBufSize = 0;
    g_MoviePaletteBufSize = 0;
    g_MovieFrameBufSize = 0;
    g_MoviePackedBufSize = 0;
    g_MovieFrameDelay = 0;
    g_MovieLoopOffset = 0;
    g_MovieMusicBuf = NULL;
    g_MovieSoundBuf = NULL;
    g_MoviePaletteBuf = NULL;
    g_MovieFrameBuf = NULL;
    g_MoviePackedBuf = NULL;
    g_MovieMusicPlaying = false;
    g_MovieSoundPlaying = false;
    g_MovieWidth = g_GameHeader.screenWidth;
    g_MovieHeight = g_GameHeader.screenHeight;
    GmMusic_Stop();
    return Host_DataSeek(fileOffset, SEEK_SET);
}

/* Завершить фильм: остановить звук и музыку фильма, освободить буферы и перерисовать
   комнату. Возвращает true. */
bool GmMovie_End(void)
{
    if (g_MovieSoundPlaying)
        GmAudio_StopSounds();
    GmMusic_Stop();
    GmHeap_FreeIfSet(g_MoviePackedBuf);
    GmHeap_FreeIfSet(g_MovieFrameBuf);
    GmHeap_FreeIfSet(g_MoviePaletteBuf);
    GmHeap_FreeIfSet(g_MovieSoundBuf);
    GmHeap_FreeIfSet(g_MovieMusicBuf);
    GmRoom_RepaintAll();
    return true;
}

/* Ошибка фильма: завершить его и записать «Movie playback error.». Возвращает false. */
bool GmMovie_Fail(void)
{
    GmMovie_End();
    return Host_SetError(s_MoviePlaybackError);
}

/* Прочитать данные текущего куска в dst: как есть, если arg0 = arg1, иначе через
   g_MoviePackedBuf с распаковкой LZSS. Довод bufferSize не используется. */
bool GmMovie_ReadChunkData(void *dst, uint32_t bufferSize)
{
    (void)bufferSize;
    if (!g_MovieChunk.arg0)
        return true;
    if (g_MovieChunk.arg0 == g_MovieChunk.arg1)
        return Host_DataRead(dst, g_MovieChunk.arg0);

    /* Особенность оригинала: ошибка чтения сжатых данных не проверяется. */
    Host_DataRead(g_MoviePackedBuf, g_MovieChunk.arg0);
    GmLzss_Unpack(g_MoviePackedBuf, dst, (int)g_MovieChunk.arg0);
    return true;
}

/* Служебный кусок фильма (тип GMMC_HEADER) по подтипу: метка конца, повторы и задержка
   кадра, буферы, смещение и размер кадра. Наличие каждого довода задают hasArg0/hasArg1.
   Возвращает GMMS_FINISHED на метке конца, GMMS_ERROR при ошибке (фильм уже прерван
   через GmMovie_Fail), иначе GMMS_CONTINUE. */
GM_MOVIE_STEP GmMovie_HeaderChunk(void)
{
    bool ok = true;

    switch (g_MovieChunk.subtype)
    {
    case GMMH_END_MARK:
        if (g_MovieChunk.arg0 == GM_MOVIE_END_MARK_LO && g_MovieChunk.arg1 == GM_MOVIE_END_MARK_HI)
            return GMMS_FINISHED;
        ok = false;
        break;

    case GMMH_REPEAT:
        g_MovieRepeatCount = 1;
        g_MovieLoopOffset = 0;
        if (g_MovieChunk.hasArg0)
            g_MovieRepeatCount = g_MovieChunk.arg0;
        if (g_MovieChunk.hasArg1)
            g_MovieFrameDelay = g_MovieChunk.arg1;
        break;

    case GMMH_PACKED_BUFFER:
        if (g_MovieChunk.hasArg0)
        {
            g_MoviePackedBufSize = g_MovieChunk.arg0;
            g_MoviePackedBuf = GmHeap_Alloc(g_MovieChunk.arg0);
            ok = g_MoviePackedBuf != NULL;
        }
        break;

    case GMMH_FRAME_BUFFERS:
        if (g_MovieChunk.hasArg0)
        {
            g_MovieFrameBufSize = g_MovieChunk.arg0;
            g_MovieFrameBuf = GmHeap_Alloc(g_MovieChunk.arg0);
            ok = g_MovieFrameBuf != NULL;
        }
        if (ok && g_MovieChunk.hasArg1)
        {
            g_MoviePaletteBufSize = g_MovieChunk.arg1;
            g_MoviePaletteBuf = GmHeap_Alloc(g_MovieChunk.arg1);
            ok = g_MoviePaletteBuf != NULL;
        }
        break;

    case GMMH_AUDIO_BUFFERS:
        if (g_MovieChunk.hasArg0)
        {
            g_MovieSoundBufSize = g_MovieChunk.arg0;
            g_MovieSoundBuf = GmHeap_Alloc(g_MovieChunk.arg0);
            ok = g_MovieSoundBuf != NULL;
        }
        if (ok && g_MovieChunk.hasArg1)
        {
            g_MovieMusicBufSize = g_MovieChunk.arg1;
            g_MovieMusicBuf = GmHeap_Alloc(g_MovieChunk.arg1);
            ok = g_MovieMusicBuf != NULL;
        }
        break;

    case GMMH_OFFSET:
        /* Особенность оригинала: оба довода пишутся в g_MovieX, смещение по Y из фильма
           не применяется (g_MovieY всегда 0). */
        if (g_MovieChunk.hasArg0)
            g_MovieX = (int)g_MovieChunk.arg0;
        if (g_MovieChunk.hasArg1)
            g_MovieX = (int)g_MovieChunk.arg1;
        break;

    case GMMH_SIZE:
        if (g_MovieChunk.hasArg0)
            g_MovieWidth = (int)g_MovieChunk.arg0;
        if (g_MovieChunk.hasArg1)
            g_MovieHeight = (int)g_MovieChunk.arg1;
        break;

    default:
        break;
    }

    if (!ok)
        return GmMovie_Fail() ? GMMS_CONTINUE : GMMS_ERROR; /* GmMovie_Fail всегда даёт false */
    return GMMS_CONTINUE;
}

/*
 * Кусок кадра фильма (тип GMMC_FRAME): прочитать данные и наложить список прямоугольников.
 * Прямоугольник: байт флагов GM_MOVIE_RECT_FLAGS (кодек, 16-разрядные x/y/ширина/высота,
 * весь кадр, последний), поля, данные дельты. Подтип GMMF_FIRST — ключевой кадр (заливка
 * цветом fillColor, точка повтора), GMMF_LOOP_END — конец круга повтора. Держит темп
 * frameDelay мс на кадр; при отставании и звуке пропускает вывод (до 7 кадров подряд).
 * Результат: GMMS_SKIP_TO_HEADER — пропустить фильм (событие действия 1), GMMS_FINISHED —
 * прервать (действие 2 или WM_QUIT), GMMS_ERROR — ошибка чтения.
 */
GM_MOVIE_STEP GmMovie_FrameChunk(void)
{
    uint8_t *data;
    DWORD now;
    GM_EVENT event;

    if (!GmMovie_ReadChunkData(g_MovieFrameBuf, g_MovieFrameBufSize))
        return GMMS_ERROR;

    data = g_MovieFrameBuf;
    now = 1; /* Ненулевое: Host_PumpMessages не сбрасывает накопленное событие. */
    if (g_MovieChunk.subtype == GMMF_FIRST)
    {
        g_MovieLoopEnded = false;
        GmAudio_Resume();
        GmFrame_Fill(g_MovieChunk.fillColor);
        if (g_MovieRepeatCount > 1)
            g_MovieLoopOffset = Host_DataTell();
        now = 0; /* Событие, накопленное до ключевого кадра, отбрасывается. */
        g_MovieShowFrames = false;
    }
    else if (g_MovieChunk.subtype == GMMF_LOOP_END)
    {
        g_MovieLoopEnded = true;
        if (--g_MovieRepeatCount != 0)
            Host_DataSeek(g_MovieLoopOffset, SEEK_SET);
    }

    if (g_MovieChunk.arg0)
    {
        GM_MOVIE_RECT_FLAGS rectFlags;

        do
        {
            int rectX = g_MovieX;
            int rectY = g_MovieY;
            int rectWidth;
            int rectHeight;

            rectFlags = *data++;
            if (rectFlags & GMMR_FULL_FRAME)
            {
                rectWidth = g_MovieWidth;
                rectHeight = g_MovieHeight;
            }
            else
            {
                rectX += *data++;
                if (rectFlags & GMMR_X16)
                    rectX += *data++ << 8;
                rectY += *data++;
                if (rectFlags & GMMR_Y16)
                    rectY += *data++ << 8;
                rectWidth = *data++;
                if (rectFlags & GMMR_WIDTH16)
                    rectWidth += *data++ << 8;
                rectHeight = *data++;
                if (rectFlags & GMMR_HEIGHT16)
                    rectHeight += *data++ << 8;
            }

            data += GmMovie_BlitDelta(data, rectFlags & GMMR_CODEC_MASK, rectX, rectY, rectWidth,
                                      rectHeight);
            if (g_MovieShowFrames)
                GmFrame_Present(rectX, rectY, rectX + rectWidth, rectY + rectHeight, false);
        } while (!(rectFlags & GMMR_LAST));
    }

    if (g_MovieShowFrames)
        GmRoom_ResetView(true);

    event = Host_PumpMessages(&now);
    if (event == GMEV_ACTION1)
        return GMMS_SKIP_TO_HEADER;
    if (event == GMEV_ACTION2)
        return GMMS_FINISHED;

    if (g_MovieChunk.subtype == GMMF_FIRST)
    {
        GmFrame_Present(g_MovieX, g_MovieY, g_MovieX + g_MovieWidth, g_MovieY + g_MovieHeight,
                        g_GameHeader.transition != 0);
        Host_PumpMessages(&g_MovieStartTime);
        g_MovieFramesShown = 0;
        g_MovieSkippedFrames = 0;
        g_MovieShowFrames = true;
    }
    else if (!g_MovieFrameDelay)
    {
        g_MovieShowFrames = true;
    }
    else
    {
        DWORD avgFrameTime = (now - g_MovieStartTime) / g_MovieFramesShown;

        if (avgFrameTime > g_MovieFrameDelay && g_MovieSoundBufSize && ++g_MovieSkippedFrames != 8)
        {
            /* Отстаём при звуке: кадр собран, но не выводится. */
            g_MovieShowFrames = false;
        }
        else
        {
            /* Опережаем: ждать, пока средняя длительность кадра не дойдёт до задержки. */
            if (avgFrameTime < g_MovieFrameDelay)
            {
                do
                {
                    event = Host_PumpMessages(&now);
                    if (event == GMEV_ACTION1)
                        return GMMS_SKIP_TO_HEADER;
                    if (event == GMEV_ACTION2)
                        return GMMS_FINISHED;
                } while ((now - g_MovieStartTime) / g_MovieFramesShown < g_MovieFrameDelay);
            }
            g_MovieSkippedFrames = 0;
            g_MovieShowFrames = true;
        }
    }

    Host_Flush();
    ++g_MovieFramesShown;
    return GMMS_CONTINUE;
}

/* Кусок палитры фильма (тип GMMC_PALETTE): прочитать палитру и поставить её со стилем
   перехода из заголовка игры. */
bool GmMovie_PaletteChunk(void)
{
    return GmMovie_ReadChunkData(g_MoviePaletteBuf, g_MoviePaletteBufSize)
        && Host_SetPalette(g_MoviePaletteBuf, 256, (char)g_GameHeader.transition, false);
}

/* Кусок звука фильма (тип GMMC_SOUND): при ненулевом подтипе приостановить вывод,
   прочитать PCM и запустить в режиме −127 (смешать сразу). */
bool GmMovie_SoundChunk(void)
{
    if (g_MovieChunk.subtype)
        GmAudio_Pause();
    g_MovieSoundPlaying = GmMovie_ReadChunkData(g_MovieSoundBuf, g_MovieSoundBufSize)
                       && GmAudio_PlaySound(g_MovieSoundBuf, 0xFF, 0, -127);
    return g_MovieSoundPlaying;
}

/* Кусок музыки фильма (тип GMMC_MUSIC): в начале нового круга (или по ненулевому подтипу)
   остановить прежнюю музыку; если музыка ещё играет — пропустить данные, иначе прочитать
   и запустить. */
bool GmMovie_MusicChunk(void)
{
    if (g_MovieMusicPlaying && (!g_MovieLoopEnded || g_MovieChunk.subtype))
    {
        GmMusic_Stop();
        g_MovieMusicPlaying = false;
    }
    if (!g_MovieChunk.arg0)
        return true;
    if (g_MovieMusicPlaying)
        return Host_DataSeek((int)g_MovieChunk.arg0, SEEK_CUR);

    g_MovieMusicPlaying = GmMovie_ReadChunkData(g_MovieMusicBuf, g_MovieMusicBufSize)
                       && GmMusic_Start(g_MovieMusicBuf);
    return g_MovieMusicPlaying;
}

/* Пропустить куски до служебного куска подтипа GMMH_END_MARK или GMMH_REPEAT и
   обработать его; другие служебные куски (без данных) пропускаются. */
static GM_MOVIE_STEP GmMovie_SkipToHeader(void)
{
    while (Host_DataRead(&g_MovieChunk, sizeof g_MovieChunk))
    {
        if (g_MovieChunk.type != GMMC_HEADER)
        {
            if (!Host_DataSeek((int)g_MovieChunk.arg0, SEEK_CUR))
                return GMMS_ERROR;
        }
        else if (g_MovieChunk.subtype == GMMH_END_MARK || g_MovieChunk.subtype == GMMH_REPEAT)
        {
            return GmMovie_HeaderChunk();
        }
    }
    return GMMS_ERROR;
}

/* Проиграть фильм со смещения fileOffset в файле игры: читать заголовки кусков и
   раздавать их по типу. Возвращает false при ошибке (сообщение записано). */
bool GmMovie_Play(int fileOffset)
{
    if (!GmMovie_Begin(fileOffset))
        return GmMovie_Fail();

    for (;;)
    {
        GM_MOVIE_STEP step = GMMS_ERROR;

        if (Host_DataRead(&g_MovieChunk, sizeof g_MovieChunk))
        {
            switch (g_MovieChunk.type)
            {
            case GMMC_HEADER:
                step = GmMovie_HeaderChunk();
                break;
            case GMMC_FRAME:
                step = GmMovie_FrameChunk();
                break;
            case GMMC_PALETTE:
                step = GmMovie_PaletteChunk() ? GMMS_CONTINUE : GMMS_ERROR;
                break;
            case GMMC_SOUND:
                step = GmMovie_SoundChunk() ? GMMS_CONTINUE : GMMS_ERROR;
                break;
            case GMMC_MUSIC:
                step = GmMovie_MusicChunk() ? GMMS_CONTINUE : GMMS_ERROR;
                break;
            default:
                break;
            }
        }

        if (step == GMMS_SKIP_TO_HEADER)
            step = GmMovie_SkipToHeader();

        /* Особенность оригинала: если ошибка случилась в GmMovie_HeaderChunk, фильм
           прерывается повторно (GmMovie_End и освобождение буферов — второй раз). */
        if (step == GMMS_ERROR)
            return GmMovie_Fail();
        if (step == GMMS_FINISHED)
            return GmMovie_End();
    }
}

/* ========================================================================== */
/* Добавочные блиттеры (g_AddBlitters, в оригинале — ассемблер)               */
/* ========================================================================== */

/*
 * Общие доводы: dst — левый верхний пиксель прямоугольника в кадровом буфере, src — поток
 * данных дельты, width × height — размер, rowStep — на сколько сдвинуть приёмник назад
 * после строки (ширина + шаг строки буфера). Возвращают конец разобранного потока.
 * Счётчики проверяются только на точное равенство нулю, как в оригинале.
 */

/* Прибавить count (≥ 1) байтов потока к приёмнику; вернуть продвинутый поток. */
static const uint8_t *AddLiterals(uint8_t *dst, const uint8_t *src, uint32_t count)
{
    do
    {
        *dst++ += *src++;
    } while (--count != 0);
    return src;
}

/* Прибавить value к count (≥ 1) пикселям приёмника. */
static void AddRun(uint8_t *dst, uint8_t value, uint32_t count)
{
    do
    {
        *dst++ += value;
    } while (--count != 0);
}

/* Кодек 0: без сжатия, width·height байтов подряд. */
const uint8_t *GmAddBlit_Raw(uint8_t *dst, const uint8_t *src, int width, int height, int rowStep)
{
    do
    {
        src = AddLiterals(dst, src, (uint32_t)width);
        dst += width - rowStep;
    } while (--height != 0);
    return src;
}

/*
 * Особые коды RLE (общие для всех трёх RLE-кодеков), n — младшие разряды кода:
 * 0 — пропустить остаток строки, 1 — литералы до конца строки, иначе — n+1 раз прибавить
 * следующий байт потока. *cols — сколько пикселей строки осталось.
 */
static const uint8_t *AddSpecial(uint8_t n, uint8_t **dst, const uint8_t *src, int *cols)
{
    if (n == 0)
    {
        *dst += *cols;
        *cols = 0;
    }
    else if (n == 1)
    {
        src = AddLiterals(*dst, src, (uint32_t)*cols);
        *dst += *cols;
        *cols = 0;
    }
    else
    {
        AddRun(*dst, *src++, n + 1u);
        *dst += n + 1;
        *cols -= n + 1;
    }
    return src;
}

/* Кодек 1: 0nnnnnnn — пропустить n+1; 10nnnnnn — n+1 литералов; 11nnnnnn — особый код n. */
const uint8_t *GmAddBlit_RleShortSkip(uint8_t *dst, const uint8_t *src, int width, int height,
                                      int rowStep)
{
    do
    {
        int cols = width;

        do
        {
            uint8_t code = *src++;
            int count = (code & GMRLE_COUNT_MASK) + 1;

            if (!(code & GMRLE_NOT_LITERAL))
            {
                dst += code + 1;
                cols -= code + 1;
            }
            else if (!(code & GMRLE_SKIP_OR_SPECIAL))
            {
                src = AddLiterals(dst, src, (uint32_t)count);
                dst += count;
                cols -= count;
            }
            else
            {
                src = AddSpecial(code & GMRLE_COUNT_MASK, &dst, src, &cols);
            }
        } while (cols != 0);
        dst -= rowStep;
    } while (--height != 0);
    return src;
}

/* Кодек 2: 0nnnnnnn — особый код n (повтор до 128 раз); 10nnnnnn — пропустить n+1;
   11nnnnnn — n+1 литералов. */
const uint8_t *GmAddBlit_RleShortSpecial(uint8_t *dst, const uint8_t *src, int width, int height,
                                         int rowStep)
{
    do
    {
        int cols = width;

        do
        {
            uint8_t code = *src++;
            int count = (code & GMRLE_COUNT_MASK) + 1;

            if (!(code & GMRLE_NOT_LITERAL))
            {
                src = AddSpecial(code, &dst, src, &cols);
            }
            else if (!(code & GMRLE_SKIP_OR_SPECIAL))
            {
                dst += count;
                cols -= count;
            }
            else
            {
                src = AddLiterals(dst, src, (uint32_t)count);
                dst += count;
                cols -= count;
            }
        } while (cols != 0);
        dst -= rowStep;
    } while (--height != 0);
    return src;
}

/* Кодек 3: 0nnnnnnn — n+1 литералов; 10nnnnnn — пропустить n+1; 11nnnnnn — особый код n. */
const uint8_t *GmAddBlit_RleShortCopy(uint8_t *dst, const uint8_t *src, int width, int height,
                                      int rowStep)
{
    do
    {
        int cols = width;

        do
        {
            uint8_t code = *src++;
            int count = (code & GMRLE_COUNT_MASK) + 1;

            if (!(code & GMRLE_NOT_LITERAL))
            {
                src = AddLiterals(dst, src, code + 1u);
                dst += code + 1;
                cols -= code + 1;
            }
            else if (!(code & GMRLE_SKIP_OR_SPECIAL))
            {
                dst += count;
                cols -= count;
            }
            else
            {
                src = AddSpecial(code & GMRLE_COUNT_MASK, &dst, src, &cols);
            }
        } while (cols != 0);
        dst -= rowStep;
    } while (--height != 0);
    return src;
}

/* Наложить прямоугольник дельты кадра фильма в (x, y) кодеком codec (0..3).
   Возвращает число разобранных байтов потока. */
int GmMovie_BlitDelta(uint8_t *src, int codec, int x, int y, int width, int height)
{
    uint8_t *dst = g_FrameBits + (g_FrameHeight - 1 - y) * g_FrameStride + x;

    return (int)(g_AddBlitters[codec](dst, src, width, height, width + g_FrameStride) - src);
}

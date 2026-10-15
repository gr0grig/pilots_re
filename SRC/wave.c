/*
 * Звуковой слой: вывод волн через waveOut, музыка в собственном формате MIDI через
 * midiOut и мультимедийный таймер, а для Win32s — переходник к 16-разрядной GMW.DLL.
 */
#include "gm.h"

#include <string.h>

/* Число буферов вывода WAVE и размер каждого. */
#define WAVE_BUFFER_COUNT ((int)(sizeof g_WaveHeaders / sizeof g_WaveHeaders[0]))
#define WAVE_BUFFER_SIZE  ((DWORD)sizeof g_WaveBuffers[0])

/* Формат вывода: 8-разрядный моно PCM 11025 Гц. */
#define WAVE_SAMPLE_RATE 11025

/* Шаг таймера MIDI и единица задержки собственного формата, мс. */
#define MIDI_TICK_MS 10

/*
 * Громкость звуков и музыки (0..255) — программная.
 *
 * Отступление от оригинала: он менял громкость устройств через waveOutSetVolume и
 * midiOutSetVolume, а «системную» громкость читал через *GetVolume. В современных
 * Windows waveOutSetVolume задаёт громкость всего приложения (вместе с ней глохнет
 * и MIDI-синтезатор, звучащий в том же процессе), а midiOutGetVolume/SetVolume у
 * синтезатора не поддерживаются (исходная громкость музыки читалась как 0, и
 * «включить музыку» включало нулевую громкость). Поэтому громкость звуков
 * применяется к отсчётам буфера перед выводом, громкость музыки — к силе нажатия
 * нот, а исходная громкость обоих — наибольшая.
 */
#define FULL_VOLUME 255

static uint8_t s_waveVolume = FULL_VOLUME;

/* Применить громкость к 8-разрядному беззнаковому PCM (тишина — 0x80). */
static void ScaleWaveBuffer(uint8_t *samples, DWORD length, uint8_t volume)
{
    if (volume == FULL_VOLUME)
        return;
    for (DWORD i = 0; i < length; i++)
        samples[i] = (uint8_t)(0x80 + ((int)samples[i] - 0x80) * volume / FULL_VOLUME);
}

/* ------------------------------------------------------------------------ */
/* WAVE                                                                     */
/* ------------------------------------------------------------------------ */

/* Закрыть вывод WAVE: снять подготовку со всех заголовков и закрыть устройство. */
int GmWave_Close(void)
{
    for (int i = 0; i < WAVE_BUFFER_COUNT; i++)
        waveOutUnprepareHeader(g_hWaveOut, &g_WaveHeaders[i], sizeof(WAVEHDR));
    return (int)waveOutClose(g_hWaveOut);
}

/*
 * Открыть вывод WAVE с уведомлениями окну (CALLBACK_WINDOW; окно лежит в первом
 * двойном слове buffers). При отказе гасит sndPlaySound и пробует ещё раз.
 * Готовит заголовки на 8 буферов по 2048 байт из buffers.
 */
bool GmWave_Open(uint8_t *buffers)
{
    WAVEFORMATEX format;
    HWND notifyWindow;
    MMRESULT error;

    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nAvgBytesPerSec = WAVE_SAMPLE_RATE;
    format.nSamplesPerSec = WAVE_SAMPLE_RATE;
    format.nBlockAlign = 1;
    format.wBitsPerSample = 8;
    format.cbSize = 0;

    memcpy(&notifyWindow, buffers, sizeof notifyWindow);
    error = waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &format, (DWORD_PTR)notifyWindow, 0, CALLBACK_WINDOW);
    if (error != MMSYSERR_NOERROR)
    {
        /* Устройство может держать системный звук — остановить его и повторить. */
        sndPlaySoundA(NULL, 0);
        error = waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &format, (DWORD_PTR)notifyWindow, 0, CALLBACK_WINDOW);
    }
    if (error != MMSYSERR_NOERROR)
        return false;

    if (waveOutGetID(g_hWaveOut, &g_WaveOutId) == MMSYSERR_NOERROR)
    {
        MMRESULT prepareError = MMSYSERR_NOERROR;
        for (int i = 0; i < WAVE_BUFFER_COUNT; i++)
        {
            WAVEHDR *header = &g_WaveHeaders[i];
            header->dwFlags = WHDR_DONE;
            header->lpData = (LPSTR)(buffers + i * WAVE_BUFFER_SIZE);
            header->dwBufferLength = WAVE_BUFFER_SIZE;
            prepareError = waveOutPrepareHeader(g_hWaveOut, header, sizeof(WAVEHDR));
            /* Буфер сразу считается свободным. */
            header->dwFlags |= WHDR_DONE;
            if (prepareError != MMSYSERR_NOERROR)
                break;
        }
        if (prepareError == MMSYSERR_NOERROR)
            return true;
    }
    GmWave_Close();
    return false;
}

/* Номер первого буфера WAVE с признаком WHDR_DONE; если такого нет — сброс вывода и 0. */
int GmWave_FindFreeBuffer(void)
{
    for (int i = 0; i < WAVE_BUFFER_COUNT; i++)
    {
        if (g_WaveHeaders[i].dwFlags & WHDR_DONE)
            return i;
    }
    waveOutReset(g_hWaveOut);
    return 0;
}

/* Отдать свободный буфер WAVE устройству. */
int GmWave_Submit(void)
{
    uint8_t index = (uint8_t)GmWave_FindFreeBuffer();
    ScaleWaveBuffer((uint8_t *)g_WaveHeaders[index].lpData, g_WaveHeaders[index].dwBufferLength, s_waveVolume);
    return waveOutWrite(g_hWaveOut, &g_WaveHeaders[index], sizeof(WAVEHDR)) == MMSYSERR_NOERROR;
}

/* Сбросить вывод WAVE. */
int GmWave_Reset(void)
{
    return waveOutReset(g_hWaveOut) == MMSYSERR_NOERROR;
}

/* Приостановить вывод WAVE. */
int GmWave_Pause(void)
{
    return waveOutPause(g_hWaveOut) == MMSYSERR_NOERROR;
}

/* Возобновить вывод WAVE. */
int GmWave_Resume(void)
{
    return waveOutRestart(g_hWaveOut) == MMSYSERR_NOERROR;
}

/* Исходная громкость звуков — наибольшая (см. «Громкость звуков и музыки»). */
int GmWave_GetVolume(void)
{
    return FULL_VOLUME;
}

/* Задать громкость звуков (0..255); при нуле сбросить вывод. */
int GmWave_SetVolume(uint8_t volume)
{
    s_waveVolume = volume;
    if (volume == 0)
        waveOutReset(g_hWaveOut);
    return true;
}

/* ------------------------------------------------------------------------ */
/* MIDI собственного формата                                                */
/* ------------------------------------------------------------------------ */

/* Остановить MIDI: снять таймер и сбросить устройство. */
int GmMidi_Halt(void)
{
    if (g_MidiTimerId)
    {
        timeKillEvent(g_MidiTimerId);
        g_MidiTimerId = 0;
    }
    return (int)midiOutReset(g_hMidiOut);
}

/*
 * Прочитать задержку: 1 байт или 2 (разряд 7 первого — есть второй, он даёт
 * старшие разряды). Единица — тик таймера (10 мс).
 */
int GmMidi_ReadDelta(void)
{
    int first = *g_MidiCursor++;
    if (first & MIDIDELTA_CONTINUED)
        return (*g_MidiCursor++ << 7) | (first & MIDIDELTA_VALUE_MASK);
    return first;
}

/*
 * Отработать все наступившие события MIDI. Формат: бегущий статус; F0/F7 — пропуск
 * блока по длине; FF — конец дорожки (повтор через g_MidiLoopDelay тиков или стоп);
 * 2- и 3-байтовые сообщения уходят в midiOutShortMsg, «нота вкл/выкл» — только при
 * ненулевой громкости. Разряд 7 последнего байта 3-байтового сообщения — следующее
 * событие без задержки; иначе после события читается задержка.
 */
void GmMidi_Pump(void)
{
    for (;;)
    {
        bool noDelay = false;
        uint8_t status = *g_MidiCursor;

        if (status < MIDI_NOTE_OFF)
            status = g_MidiRunningStatus;
        else
            ++g_MidiCursor;
        g_MidiRunningStatus = status;

        if (status == MIDI_META)
        {
            if (g_MidiLoopDelay == GM_MIDI_NO_LOOP)
            {
                GmMidi_Halt();
            }
            else
            {
                g_MidiDelayCounter = g_MidiLoopDelay;
                g_MidiCursor = g_MidiTrackStart;
            }
            return;
        }

        if (status == MIDI_SYSEX || status == MIDI_SYSEX_END)
        {
            g_MidiCursor += GmMidi_ReadDelta();
        }
        else
        {
            uint8_t command = status & MIDI_COMMAND_MASK;
            DWORD message = status | ((DWORD)*g_MidiCursor++ << 8);
            bool send = true;

            if (command == MIDI_NOTE_OFF || command == MIDI_NOTE_ON)
                send = g_MidiVolume != 0;

            if (command != MIDI_PROGRAM_CHANGE && command != MIDI_CHANNEL_PRESSURE)
            {
                uint8_t data2 = *g_MidiCursor++;
                uint8_t value = data2 & MIDIDATA_VALUE_MASK;
                /* Громкость музыки — сила нажатия ноты. */
                if (command == MIDI_NOTE_ON && value != 0)
                    value = (uint8_t)((value * g_MidiVolume + FULL_VOLUME - 1) / FULL_VOLUME);
                message |= (DWORD)value << 16;
                noDelay = (data2 & MIDIDATA_NO_DELAY) != 0;
            }
            if (send)
                midiOutShortMsg(g_hMidiOut, message);
        }

        if (!noDelay)
        {
            int delta = GmMidi_ReadDelta();
            if (delta)
            {
                g_MidiNextEventTime += MIDI_TICK_MS * delta;
                if (timeGetTime() < g_MidiNextEventTime)
                    return;
            }
        }
    }
}

/*
 * Обратный вызов таймера (каждые 10 мс): выдержать задержку повтора дорожки, затем,
 * если время события наступило и обработка не идёт, вызвать GmMidi_Pump.
 */
void CALLBACK GmMidi_TimerProc(UINT timerId, UINT msg, DWORD_PTR user, DWORD_PTR reserved1, DWORD_PTR reserved2)
{
    (void)timerId;
    (void)msg;
    (void)user;
    (void)reserved1;
    (void)reserved2;

    if (g_MidiDelayCounter)
    {
        if (--g_MidiDelayCounter)
            return;
        g_MidiNextEventTime = timeGetTime();
    }

    /* В оригинале — одно 64-разрядное сравнение времени с парой
       (g_MidiInCallback, g_MidiNextEventTime): при ненулевом счётчике входа оно ложно. */
    DWORD now = timeGetTime();
    if (g_MidiInCallback == 0 && now >= g_MidiNextEventTime)
    {
        ++g_MidiInCallback;
        GmMidi_Pump();
        --g_MidiInCallback;
    }
}

/* Закрыть MIDI: остановить, вернуть разрешение таймера, закрыть устройство. */
int GmMidi_Close(void)
{
    int result = GmMidi_Halt();

    if (g_MidiTimerPeriod)
    {
        result = (int)timeEndPeriod(g_MidiTimerPeriod);
        g_MidiTimerPeriod = 0;
    }
    if (g_hMidiOut)
    {
        result = (int)midiOutClose(g_hMidiOut);
        g_hMidiOut = NULL;
    }
    return result;
}

/* Открыть MIDI-устройство по умолчанию и задать разрешение таймера не мельче 10 мс. */
bool GmMidi_Open(void)
{
    TIMECAPS caps;

    if (midiOutOpen(&g_hMidiOut, MIDI_MAPPER, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        return false;

    g_MidiTimerPeriod = 0;
    g_MidiVolume = 1;
    if (midiOutGetID(g_hMidiOut, &g_MidiOutId) == MMSYSERR_NOERROR
        && timeGetDevCaps(&caps, sizeof caps) == TIMERR_NOERROR)
    {
        UINT period = caps.wPeriodMin > MIDI_TICK_MS ? caps.wPeriodMin : MIDI_TICK_MS;
        if (period >= caps.wPeriodMax)
            period = caps.wPeriodMax;
        g_MidiTimerPeriod = period;
        if (timeBeginPeriod(period) == TIMERR_NOERROR)
            return true;
    }
    GmMidi_Close();
    return false;
}

/*
 * Начать дорожку: события идут после длины; вводный F8 с задержкой и ещё одним
 * байтом пропускается. Повтор — через 1 тик после конца; первое событие — на первом
 * тике. Запускает периодический таймер 10 мс. 1 — успех, 0 — таймер не запущен.
 */
int GmMidi_Play(const GM_MUSIC_DATA *music)
{
    GmMidi_Halt();

    g_MidiCursor = (uint8_t *)music->events;
    if (music->events[0] == MIDI_TIMING_CLOCK)
    {
        ++g_MidiCursor;
        GmMidi_ReadDelta();
        ++g_MidiCursor;
    }
    g_MidiTrackStart = g_MidiCursor;
    g_MidiLoopDelay = 1;
    g_MidiDelayCounter = 1;
    g_MidiInCallback = 0;

    g_MidiTimerId = timeSetEvent(MIDI_TICK_MS, g_MidiTimerPeriod, GmMidi_TimerProc, 0, TIME_PERIODIC);
    if (!g_MidiTimerId)
    {
        GmMidi_Halt();
        return 0;
    }
    return 1;
}

/* Остановить музыку MIDI; всегда 1. */
int GmMidi_Stop(void)
{
    GmMidi_Halt();
    return 1;
}

/* Исходная громкость музыки — наибольшая (см. «Громкость звуков и музыки»). */
int GmMidi_GetVolume(void)
{
    return FULL_VOLUME;
}

/* Задать громкость музыки (0..255); при нуле сбросить устройство (ноты при нулевой
   громкости не посылаются). */
int GmMidi_SetVolume(uint8_t volume)
{
    g_MidiVolume = volume;
    if (volume == 0)
        midiOutReset(g_hMidiOut);
    return true;
}

/* ------------------------------------------------------------------------ */
/* Шлюз Win32s в 16-разрядную GMW.DLL                                       */
/* ------------------------------------------------------------------------ */

/* Закрыть шлюз GMW: команда закрытия, UTUnRegister, освободить KERNEL32 и буфер обмена. */
void GmGmw_Close(void)
{
    if (g_pfnGmwThunk)
        g_pfnGmwThunk(NULL, GMW_CLOSE, NULL);
    if (g_pfnUTUnRegister)
        g_pfnUTUnRegister(g_hAppInstance);
    if (g_hKernel32ForUT)
        FreeLibrary(g_hKernel32ForUT);
    if (g_GmwBuffer)
    {
        GlobalFree(g_GmwBuffer);
        g_GmwBuffer = NULL;
    }
}

/* Отдать буфер вывода. */
int GmGmw_Submit(void)
{
    return (int)g_pfnGmwThunk(NULL, GMW_SUBMIT, NULL);
}

/* Сбросить вывод. */
int GmGmw_Reset(void)
{
    return (int)g_pfnGmwThunk(NULL, GMW_RESET, NULL);
}

/* Громкость звука. */
int GmGmw_GetSoundVolume(void)
{
    return (int)g_pfnGmwThunk(NULL, GMW_GET_SOUND_VOLUME, NULL);
}

/* Номер следующего свободного буфера. */
int GmGmw_NextBuffer(void)
{
    return (int)g_pfnGmwThunk(NULL, GMW_NEXT_BUFFER, NULL);
}

/* Приостановить вывод. */
int GmGmw_Pause(void)
{
    return (int)g_pfnGmwThunk(NULL, GMW_PAUSE, NULL);
}

/* Возобновить вывод. */
int GmGmw_Resume(void)
{
    return (int)g_pfnGmwThunk(NULL, GMW_RESUME, NULL);
}

/* Задать громкость звука: номер команды 0..255 и есть громкость. */
int GmGmw_SetSoundVolume(uint8_t volume)
{
    return (int)g_pfnGmwThunk(NULL, GMW_SET_SOUND_VOLUME_BASE + volume, NULL);
}

/*
 * Играть музыку: данные (длина и события) копируются в буфер обмена, доступный
 * 16-разрядной стороне. Без буфера оригинал возвращал случайное значение; здесь — 0.
 */
int GmGmw_PlayMusic(const GM_MUSIC_DATA *music)
{
    if (!g_GmwBuffer)
        return 0;
    memcpy((void *)g_GmwBuffer, music, offsetof(GM_MUSIC_DATA, events) + music->length);
    return (int)g_pfnGmwThunk((void *)g_GmwBuffer, GMW_PLAY_MUSIC, NULL);
}

/* Остановить музыку. */
int GmGmw_StopMusic(void)
{
    return (int)g_pfnGmwThunk(NULL, GMW_STOP_MUSIC, NULL);
}

/* Громкость музыки. */
int GmGmw_GetMusicVolume(void)
{
    return (int)g_pfnGmwThunk(NULL, GMW_GET_MUSIC_VOLUME, NULL);
}

/* Задать громкость музыки: номер команды 0x100 + громкость. */
int GmGmw_SetMusicVolume(uint8_t volume)
{
    return (int)g_pfnGmwThunk(NULL, GMW_SET_MUSIC_VOLUME_BASE + volume, NULL);
}

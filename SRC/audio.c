/*
 * Звуковая подсистема верхнего уровня: каналы звуков, микширование 8-разрядного PCM,
 * выбор звукового слоя (WinMM или шлюз Win32s в GMW.DLL) и заглушки после выключения.
 * Звуковой слой подключается через таблицу указателей звуковых функций (data.c).
 */
#include "gm.h"

#include <string.h>

/* Размер одного буфера вывода WAVE (2048 байт); столько смешивается за раз. */
#define WAVE_BUFFER_SIZE ((uint32_t)sizeof g_WaveBuffers[0])

/* Объём буфера обмена с 16-разрядной GMW.DLL. */
#define GMW_EXCHANGE_SIZE 0xF000u

/* Номер процедуры переходника в GMW.DLL для UTRegister. */
#define GMW_THUNK_PROC MAKEINTRESOURCEA(2)

/* ------------------------------------------------------------------------ */
/* Музыка и перезапуск звука                                                 */
/* ------------------------------------------------------------------------ */

/* Запустить музыку (данные дорожки) и запомнить, что она играет. */
bool GmMusic_Start(const GM_MUSIC_DATA *music)
{
    /* Признак берётся по младшему байту результата звукового слоя. */
    g_MusicActive = (uint8_t)GmAudio_PlayMusic(music, -1, 0) != 0;
    return g_MusicActive;
}

/* Остановить музыку, если играет; номер текущей дорожки = -1. */
void GmMusic_Stop(void)
{
    if (g_MusicActive)
        GmAudio_StopMusic();
    g_Settings.currentMusic = -1;
    g_MusicActive = false;
}

/* Остановить звуки и выключить звук (перед AVI или внешней программой). */
void GmAudio_StopAndShutdown(void)
{
    GmAudio_StopSounds();
    GmAudio_Shutdown();
}

/* Заново включить звук (системные громкости запоминаются) и возобновить играющую дорожку. */
void GmAudio_Restart(void)
{
    GmAudio_Init(g_Settings.soundChannels, 0, &g_SystemSoundVolume, &g_SystemMusicVolume);
    if (g_Settings.currentMusic != -1)
        Native_PlayMusic(g_Settings.currentMusic);
}

/* ------------------------------------------------------------------------ */
/* Заглушки звукового слоя (после GmAudio_Shutdown)                          */
/* ------------------------------------------------------------------------ */

/* Заглушка закрытия звука: ничего не делает. */
void GmAudioStub_Shutdown(void)
{
}

/* Заглушка звуковых функций без доводов: успех. */
int GmAudioStub_ReturnTrue(void)
{
    return 1;
}

/* Заглушка установки громкости: успех. */
int GmAudioStub_SetVolume(uint8_t volume)
{
    (void)volume;
    return 1;
}

/* Заглушка запуска музыки: успех. */
int GmAudioStub_PlayMusic(const GM_MUSIC_DATA *music)
{
    (void)music;
    return 1;
}

/* ------------------------------------------------------------------------ */
/* Выбор звукового слоя                                                      */
/* ------------------------------------------------------------------------ */

/*
 * Открыть звук для Win32s (Windows 3.x): переходник UTRegister в 16-разрядную GMW.DLL,
 * команда открытия с буферами WAVE, буфер обмена для музыки; указатели слоя — на GmGmw_*.
 * Возвращает 1 при успехе, 0 при отказе.
 */
int GmAudio_OpenGmw(uint8_t *buffers)
{
    g_hKernel32ForUT = LoadLibraryA(s_Kernel32Dll);
    if (g_hKernel32ForUT == NULL)
        return 0;

    /* На Windows 95/NT UTRegister в kernel32 нет — тогда здесь отказ. */
    g_pfnUTRegister = (PFN_UTREGISTER)(void *)GetProcAddress(g_hKernel32ForUT, s_UTRegister);
    if (g_pfnUTRegister == NULL)
        return 0;
    g_pfnUTUnRegister = (PFN_UTUNREGISTER)(void *)GetProcAddress(g_hKernel32ForUT, s_UTUnRegister);
    if (g_pfnUTUnRegister == NULL)
        return 0;

    if (!g_pfnUTRegister(g_hAppInstance, s_GmwDll, NULL, GMW_THUNK_PROC, &g_pfnGmwThunk, NULL, NULL))
        return 0;

    if (g_pfnGmwThunk(buffers, GMW_OPEN, NULL))
    {
        g_GmwBuffer = GlobalAlloc(GMEM_FIXED, GMW_EXCHANGE_SIZE);
        if (g_GmwBuffer != NULL)
        {
            g_pfnAudioSubmit = GmGmw_Submit;
            g_pfnAudioReset = GmGmw_Reset;
            g_pfnGetSoundVolume = GmGmw_GetSoundVolume;
            g_pfnSetSoundVolume = GmGmw_SetSoundVolume;
            g_pfnAudioNextBuffer = GmGmw_NextBuffer;
            g_pfnAudioPause = GmGmw_Pause;
            g_pfnAudioResume = GmGmw_Resume;
            g_pfnMusicPlay = GmGmw_PlayMusic;
            g_pfnMusicStop = GmGmw_StopMusic;
            g_pfnGetMusicVolume = GmGmw_GetMusicVolume;
            g_pfnSetMusicVolume = GmGmw_SetMusicVolume;
            g_pfnAudioShutdown = GmGmw_Close;
            return 1;
        }
    }
    GmGmw_Close();
    return 0;
}

/* Закрыть WAVE и MIDI. */
void GmAudio_CloseWinMM(void)
{
    GmWave_Close();
    GmMidi_Close();
}

/*
 * Открыть звук WinMM (Windows 95/NT): WAVE и MIDI, достаточно одного из них;
 * указатели слоя — на функции GmWave_ и GmMidi_. Возвращает 1 при успехе, 0 при отказе.
 */
int GmAudio_OpenWinMM(uint8_t *buffers)
{
    bool waveOpened = GmWave_Open(buffers);

    if (!GmMidi_Open() && !waveOpened)
    {
        GmAudio_CloseWinMM();
        return 0;
    }

    g_pfnAudioSubmit = GmWave_Submit;
    g_pfnAudioReset = GmWave_Reset;
    g_pfnGetSoundVolume = GmWave_GetVolume;
    g_pfnSetSoundVolume = GmWave_SetVolume;
    g_pfnAudioNextBuffer = GmWave_FindFreeBuffer;
    g_pfnAudioPause = GmWave_Pause;
    g_pfnAudioResume = GmWave_Resume;
    g_pfnMusicPlay = GmMidi_Play;
    g_pfnMusicStop = GmMidi_Stop;
    g_pfnGetMusicVolume = GmMidi_GetVolume;
    g_pfnSetMusicVolume = GmMidi_SetVolume;
    g_pfnAudioShutdown = GmAudio_CloseWinMM;
    return 1;
}

/* ------------------------------------------------------------------------ */
/* Микширование                                                              */
/* ------------------------------------------------------------------------ */

/*
 * Подмешать канал к буферу вывода: беззнаковые 8-разрядные отсчёты складываются
 * как знаковые (относительно 0x80) с насыщением. size — число байт, обрабатывается
 * кратно 4 (в оригинале — ассемблер, по двойному слову за шаг).
 */
void GmMix_AddSaturated(uint32_t *dst, const uint32_t *src, uint32_t size)
{
    uint8_t *out = (uint8_t *)dst;
    const uint8_t *in = (const uint8_t *)src;
    uint32_t dwordsLeft = size >> 2;

    /* Особенность оригинала: проверка счётчика в конце цикла, при size < 4 он
       не останавливается (на практике размер всегда не меньше 4). */
    do
    {
        for (int i = 0; i < 4; i++)
        {
            int sum = in[i] + out[i] - 0x80;

            /* Особенность оригинала: переполнение вверх зажимается в 0xF0, а не в 0xFF. */
            if (sum > 0xFF)
                out[i] = GMPCM_CLIP_HIGH;
            else if (sum < 0)
                out[i] = GMPCM_CLIP_LOW;
            else
                out[i] = (uint8_t)sum;
        }
        in += 4;
        out += 4;
    } while (--dwordsLeft != 0);
}

/*
 * Смешать очередные 2048 байт всех активных каналов в свободный буфер вывода
 * (первый канал копируется, прочие подмешиваются с насыщением) и отдать буфер
 * устройству. Зовётся по MM_WOM_DONE и при запуске звука. Если смешивать нечего,
 * возвращает номер свободного буфера, иначе — результат отдачи.
 */
int GmAudio_Mix(void)
{
    int mixed = 0;
    int bufferIndex = (uint8_t)g_pfnAudioNextBuffer();
    uint8_t *outBuffer = g_WaveBuffers[bufferIndex];
    GM_SOUND_CHANNEL *channel = g_SoundChannels;

    /* Как в оригинале, первый канал просматривается и при нулевом числе каналов. */
    do
    {
        if (channel->start != NULL)
        {
            uint32_t chunk = channel->remaining;
            if (chunk > WAVE_BUFFER_SIZE)
                chunk = WAVE_BUFFER_SIZE;

            if (mixed)
            {
                GmMix_AddSaturated((uint32_t *)outBuffer, (const uint32_t *)channel->cur, chunk);
            }
            else
            {
                /* Первый канал: копия, остаток буфера — тишина. */
                uint32_t copied = chunk & ~3u;
                memcpy(outBuffer, channel->cur, copied);
                memset(outBuffer + copied, 0x80, WAVE_BUFFER_SIZE - copied);
            }

            channel->remaining -= chunk;
            if (channel->remaining == 0)
                channel->start = NULL;
            else
                channel->cur += chunk;
            ++mixed;
        }
        ++channel;
    } while (channel < &g_SoundChannels[g_SoundChannelCount]);

    g_ChannelsInLastMix = mixed;
    if (mixed)
        return g_pfnAudioSubmit();
    return bufferIndex;
}

/* ------------------------------------------------------------------------ */
/* Внешний интерфейс звука                                                   */
/* ------------------------------------------------------------------------ */

/*
 * Включить звук: на Windows 95/NT — WinMM, на Win32s — шлюз GMW. Выдаёт системные
 * громкости звука и музыки и заводит channelCount пустых каналов. Всегда true.
 */
bool GmAudio_Init(uint8_t channelCount, int unused, uint8_t *soundVolume, uint8_t *musicVolume)
{
    DWORD version = GetVersion();
    int opened;

    (void)unused;

    /* Первое двойное слово буферов WAVE временно хранит окно для уведомлений. */
    memcpy(g_WaveBuffers[0], &g_MainWindow, sizeof g_MainWindow);

    /* Старший разряд сброшен — NT; иначе Windows 95 (версия 4+) или Win32s. */
    if ((version & 0x80000000u) == 0 || (uint8_t)version >= 4)
        opened = GmAudio_OpenWinMM(g_WaveBuffers[0]);
    else
        opened = GmAudio_OpenGmw(g_WaveBuffers[0]);

    if (opened)
    {
        *soundVolume = (uint8_t)g_pfnGetSoundVolume();
        *musicVolume = (uint8_t)g_pfnGetMusicVolume();
        g_SoundChannelCount = channelCount;
        g_ChannelsInLastMix = 0;
        g_NextSoundChannel = 0;

        GM_SOUND_CHANNEL *channel = g_SoundChannels;
        do
        {
            channel->start = NULL;
            ++channel;
        } while (channel < &g_SoundChannels[g_SoundChannelCount]);
    }
    return true;
}

/* Запустить музыку через текущий звуковой слой; два последних довода не используются. */
int GmAudio_PlayMusic(const GM_MUSIC_DATA *music, int unusedMinusOne, int unusedZero)
{
    (void)unusedMinusOne;
    (void)unusedZero;
    return g_pfnMusicPlay(music);
}

/*
 * Сыграть звук: занять свободный канал (по кругу от последнего выбранного; если все
 * заняты — вытеснить его). mode < 0 — сразу подмешать один буфер; mode >= 0 — если
 * вывод простаивал, сразу подмешать два (двойная буферизация), иначе звук подхватит
 * следующий MM_WOM_DONE. unreadFF и unreadZero сохраняются в канале, но не читаются.
 */
bool GmAudio_PlaySound(const GM_SOUND_DATA *sound, uint8_t unreadFF, uint8_t unreadZero, int8_t mode)
{
    uint32_t index = g_NextSoundChannel;

    do
    {
        if (g_SoundChannels[index].start == NULL)
            break;
        if (++index >= g_SoundChannelCount)
            index = 0;
    } while (index != g_NextSoundChannel);
    g_NextSoundChannel = index;

    GM_SOUND_CHANNEL *channel = &g_SoundChannels[index];
    channel->remaining = sound->length & ~3u;
    channel->start = (uint8_t *)sound->pcm;
    channel->cur = (uint8_t *)sound->pcm;
    channel->unreadZero = unreadZero;
    channel->unreadFF = unreadFF;
    channel->mode = mode;

    if (mode < 0)
    {
        GmAudio_Mix();
    }
    else if (g_ChannelsInLastMix == 0)
    {
        GmAudio_Mix();
        GmAudio_Mix();
    }
    return true;
}

/* Задать громкость музыки через текущий звуковой слой. */
int GmAudio_SetMusicVolume(uint8_t volume)
{
    return g_pfnSetMusicVolume(volume);
}

/* Задать громкость звука через текущий звуковой слой. */
int GmAudio_SetSoundVolume(uint8_t volume)
{
    return g_pfnSetSoundVolume(volume);
}

/* Остановить музыку через текущий звуковой слой. */
int GmAudio_StopMusic(void)
{
    return g_pfnMusicStop();
}

/* Остановить все звуки: освободить каналы и сбросить вывод. */
int GmAudio_StopSounds(void)
{
    GM_SOUND_CHANNEL *channel = g_SoundChannels;

    do
    {
        channel->start = NULL;
        ++channel;
    } while (channel < &g_SoundChannels[g_SoundChannelCount]);
    return g_pfnAudioReset();
}

/* Приостановить вывод звука. */
int GmAudio_Pause(void)
{
    return g_pfnAudioPause();
}

/* Возобновить вывод звука. */
int GmAudio_Resume(void)
{
    return g_pfnAudioResume();
}

/* Выключить звук: закрыть текущий слой и поставить во все указатели заглушки. */
void GmAudio_Shutdown(void)
{
    g_pfnAudioShutdown();
    g_pfnMusicPlay = GmAudioStub_PlayMusic;
    g_pfnAudioResume = GmAudioStub_ReturnTrue;
    g_pfnAudioPause = GmAudioStub_ReturnTrue;
    g_pfnAudioNextBuffer = GmAudioStub_ReturnTrue;
    g_pfnGetMusicVolume = GmAudioStub_ReturnTrue;
    g_pfnGetSoundVolume = GmAudioStub_ReturnTrue;
    g_pfnMusicStop = GmAudioStub_ReturnTrue;
    g_pfnAudioReset = GmAudioStub_ReturnTrue;
    g_pfnAudioSubmit = GmAudioStub_ReturnTrue;
    g_pfnSetMusicVolume = GmAudioStub_SetVolume;
    g_pfnSetSoundVolume = GmAudioStub_SetVolume;
    g_pfnAudioShutdown = GmAudioStub_Shutdown;
}

#include <globals.h>
#include "AudioTools.h"
#include "AudioTools/CoreAudio/AudioPlayer.h"
#include "AudioTools/AudioCodecs/CodecHelix.h"
#include "AudioTools/Disk/AudioSource.h"
#include "muse.h"

#if OTA_APP

using namespace audio_tools;

// Audio pipeline components (global so callbacks can reach them)
static AudioSourceCallback    s_source;       // callbacks open files from SD
static DecoderHelix           s_decoder;      // MP3/AAC/WAV auto-detect
static I2SStream              s_i2sOut;       // I2S DMA output on GPIO 17
AudioPlayer                   s_player(s_source, s_i2sOut, s_decoder);

// Currently-open audio file and its size (updated by callbacks)
static File s_audioFile;
static size_t s_fileSize = 0;

// AudioSourceCallback: opens files via global_fs
static Stream* onSelectStream(int index) {
    if (s_audioFile) s_audioFile.close();

    if (index == -1) {
        // Called from AudioPlayer::setPath(path) -> source.selectStream(path)
        const char* path = s_source.getPath();
        if (!path || !path[0]) return nullptr;
        s_audioFile = global_fs->open(path, "r");
    } else {
        // Direct index (not used in normal flow, but provide for completeness)
        char path[256];
        if (g_playlistMgr.source() == PlaySource::Library) {
            if (index < 0 || index >= g_library.count()) return nullptr;
            const String& p = g_library.path(index);
            strncpy(path, p.c_str(), sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
        } else {
            if (!g_playlistMgr.getEntry(index, path, sizeof(path))) return nullptr;
        }
        s_audioFile = global_fs->open(path, "r");
    }

    if (!s_audioFile) return nullptr;
    s_fileSize = s_audioFile.size();
    return &s_audioFile;
}

// Called by AudioPlayer::next() on EOF: returns the next stream based on loop mode.
static Stream* onNextStream(int offset) {
    if (offset <= 0) return nullptr;

    if (g_loopMode == LoopMode::One) {
        const char* path = s_source.getPath();
        if (!path || !path[0]) return nullptr;
        if (s_audioFile) s_audioFile.close();
        s_audioFile = global_fs->open(path, "r");
        if (s_audioFile) {
            s_fileSize = s_audioFile.size();
            return &s_audioFile;
        }
        return nullptr;
    }

    if (g_loopMode == LoopMode::All) {
        int next = (g_nowTrackIndex + 1) % g_trackCount;
        char path[256];
        if (!get_track_path(next, path, sizeof(path))) return nullptr;
        if (s_audioFile) s_audioFile.close();
        s_audioFile = global_fs->open(path, "r");
        if (s_audioFile) {
            s_fileSize = s_audioFile.size();
            g_nowTrackIndex = next;
            strncpy(g_nowPath, path, sizeof(g_nowPath) - 1);
            g_nowPath[sizeof(g_nowPath) - 1] = '\0';
            g_nowTitle[0] = '\0';
            g_nowDuration = compute_duration(path);
            g_nowProgress = 0.0f;
            g_needsRedraw = true;
            return &s_audioFile;
        }
        return nullptr;
    }

    return nullptr;
}

// Playback control helpers (called from ui_*.cpp)
void music_stop() {
    s_player.setActive(false);
}

void music_play(const char* path) {
    s_player.setAutoFade(true);
    s_player.setPath(path);
    s_player.play();
}

void music_pause() {
    s_player.setAutoFade(false);
    s_player.setActive(false);
}

void music_resume() {
    s_player.play();
}

// appTask: UI, progress, volume
static void appTask(void*) {
    vTaskDelay(pdMS_TO_TICKS(200));
    for (;;) {
        processKB_APP();

        // Touch slider: volume
        {
            static unsigned long lastTouchMs = 0;
            unsigned long now = millis();
            int scroll = 0;
            if (now - lastTouchMs >= 200) {
                lastTouchMs = now;
                scroll = TOUCH().getScrollVector();
            }
            if (scroll > 0 && g_volume < 245) {
                g_volume += 10;
                s_player.setVolume(g_volume / 255.0f);
                g_needsRedraw = true;
            } else if (scroll < 0 && g_volume > 10) {
                g_volume -= 10;
                s_player.setVolume(g_volume / 255.0f);
                g_needsRedraw = true;
            }
        }

        // Progress from file position
        if (s_audioFile && s_fileSize > 0) {
            g_nowProgress = (float)s_audioFile.position() / s_fileSize;
        }

        // Update global play state
        if (s_player.isActive()) {
            g_playState = PlayerState::Playing;
        } else if (g_nowTrackIndex >= 0 && g_nowPath[0]) {
            g_playState = PlayerState::Paused;
        } else {
            g_playState = PlayerState::Stopped;
        }

        ui_update_oled();
        vTaskDelay(pdMS_TO_TICKS(50));
        yield();
    }
}

// Application entry points (called from PocketMage framework)
void APP_INIT() {
    // Restore volume
    prefs.begin("PocketMuse", true);
    g_volume = prefs.getUChar("volume", 200);
    prefs.end();

    // Configure I2S output on BZ_PIN (GPIO 17).
    // Single-pin mode: BCK and WS clocks generated internally, only DATA pin used.
    // A simple RC low-pass filter on the pin converts the bitstream to analog audio.
    auto cfg = s_i2sOut.defaultConfig();
    cfg.pin_data = BZ_PIN;
    cfg.pin_bck = -1;
    cfg.sample_rate = 44100;
    cfg.channels = 2;
    cfg.bits_per_sample = 16;
    cfg.signal_type = PDM;
    s_i2sOut.begin(cfg);

    // Set up AudioSourceCallback
    s_source.setCallbackSelectStream(onSelectStream);

    // Initialize the pipeline (setupFade, volume_out.begin, etc.)
    // without selecting a stream or activating playback.
    s_player.begin(-1, false);

    // Enable auto-next for gapless track advancement
    s_player.setAutoNext(true);
    s_source.setCallbackNextStream(onNextStream);

    // Set volume
    s_player.setVolume(g_volume / 255.0f);

    // Scan library
    if (global_fs) {
        g_library.scan();
        g_playlistMgr.begin();
    }
    g_trackCount = g_library.count();
    browser_init();
}

void processKB_APP() {
    char ch = KB().updateKeypress();
    if (!ch) return;

    if (ch == 18) {
        KB().setKeyboardState(KB().getKeyboardState() == FUNC ? NORMAL : FUNC);
        return;
    }
    if (ch == 17) {
        KB().setKeyboardState(KB().getKeyboardState() == SHIFT ? NORMAL : SHIFT);
        return;
    }
    if (ch == 12 || ch == 27 || ch == 65) {
        music_stop();
        prefs.begin("PocketMuse", false);
        prefs.putUChar("volume", g_volume);
        prefs.end();
        KB().setKeyboardState(NORMAL);
        rebootToPocketMage();
        return;
    }

    switch (g_appMode) {
        case MODE_BROWSER:     browser_process_key(ch);     break;
        case MODE_NOWPLAYING:  nowplaying_process_key(ch);  break;
        case MODE_PLAYLIST:    playlist_process_key(ch);    break;
        case MODE_HELP:        g_appMode = g_prevMode; g_needsRedraw = true; break;
    }

    ui_update_oled();
}

void einkHandler_APP() {
    if (!g_needsRedraw) return;

    g_needsRedraw = false;

    switch (g_appMode) {
        case MODE_HELP:       ui_show_help();       break;
        case MODE_BROWSER:    browser_render();     break;
        case MODE_NOWPLAYING: nowplaying_render();  break;
        case MODE_PLAYLIST:   playlist_render();    break;
    }
}

void applicationEinkHandler() {
    einkHandler_APP();
}

void einkHandler(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(250));
    UBaseType_t minStack = UINT32_MAX;
    for (;;) {
        applicationEinkHandler();
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        if (hwm < minStack) {
            minStack = hwm;
            Serial.printf("einkHandlerTask HWM: %u\n", hwm);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        yield();
    }
}

void setup() {
    PocketMage_INIT();
    xTaskCreatePinnedToCore(appTask, "appTask", 32768, NULL, 2, NULL, 1);
    APP_INIT();
}

void loop() {
    s_player.copy();
}

#endif

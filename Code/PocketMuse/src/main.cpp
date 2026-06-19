#include <globals.h>
#include "ringbuf.h"
#include "decoder.h"
#include "player.h"
#include "muse.h"
#include "output/output_pwm.h"
#include "output/output_usb.h"

#if OTA_APP

static RingBuffer s_ringbuf;
static Decoder s_decoder(s_ringbuf);
Player player(s_decoder, s_ringbuf);
static PWMAudioOutput s_pwm(s_ringbuf, BZ_PIN);

static void appTask(void*) {
    vTaskDelay(pdMS_TO_TICKS(200));
    for (;;) {
        processKB_APP();

        // Update progress from player
        if (player.state() == PlayerState::Playing) {
            g_nowProgress = player.progress();
        }

        // Detect track end and auto-advance
        static PlayerState prevState = PlayerState::Stopped;
        PlayerState curState = player.state();
        if (prevState == PlayerState::Playing && curState == PlayerState::Stopped) {
            if (g_nowTrackIndex >= 0 && g_trackCount > 0) {
                if (g_loopMode == LoopMode::All || g_nowTrackIndex + 1 < g_trackCount) {
                    player_next_track();
                }
            }
        }
        prevState = curState;
        g_playState = curState;

        ui_update_oled();
        vTaskDelay(pdMS_TO_TICKS(50));
        yield();
    }
}

static void audioTask(void*) {
    vTaskDelay(pdMS_TO_TICKS(100));

    // Small batch buffer for draining s_ringbuf -> s_pwm. Batching avoids
    // paying pwm_audio_write()'s per-call overhead (volume math +
    // semaphore wait) once per single sample.
    static constexpr size_t kBatchSamples = 256;
    int16_t batch[kBatchSamples];

    uint8_t lastVolume = 255; // sentinel outside g_volume's range, forces initial sync

    for (;;) {
        // Decode more of the current file into s_ringbuf if there's room.
        if (s_ringbuf.freeSpace() > 2048) {
            player.tick();
        }

        // Push g_volume changes (UI-driven, see nowplaying_process_key)
        // through to the actual PWM output. Cheap to check every loop;
        // only calls into pwm_audio when it actually changed.
        uint8_t vol = g_volume;
        if (vol != lastVolume) {
            // g_volume is 0..255 (UI range); pwm_audio wants -16..16.
            int8_t pwmVol = (int8_t)((int)vol * 32 / 255) - 16;
            s_pwm.setVolume(pwmVol);
            lastVolume = vol;
        }

        // Drain decoded samples out of the shared ring buffer and feed
        // them to the active AudioOutput.
        size_t count = 0;
        while (count < kBatchSamples) {
            int16_t sample;
            if (!s_ringbuf.read(sample)) break;
            batch[count++] = sample;
        }

        if (count > 0) {
            size_t written = 0;
            while (written < count) {
                size_t n = s_pwm.write(batch + written, count - written);
                if (n == 0) break; // output buffer full; drop rest of batch rather than spin
                written += n;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void APP_INIT() {
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
        if (player.state() != PlayerState::Stopped) {
            player.stop();
        }
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
    player.setOutput(&s_pwm);
    s_pwm.begin(44100);
    xTaskCreatePinnedToCore(appTask, "appTask", 32768, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(audioTask, "audioTask", 16384, NULL, 5, NULL, 0);
    APP_INIT();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(100));
}

#endif

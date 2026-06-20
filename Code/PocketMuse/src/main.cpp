#include <globals.h>
#include "ringbuf.h"
#include "decoder.h"
#include "player.h"
#include "muse.h"
#include "output/output_pwm.h"
#include "output/output_usb.h"

extern volatile uint32_t g_isr_count;

#if OTA_APP

static RingBuffer s_ringbuf;
static Decoder s_decoder(s_ringbuf);
Player player(s_decoder, s_ringbuf);
static PWMAudioOutput s_pwm(s_ringbuf, BZ_PIN);

static void appTask(void*) {
    vTaskDelay(pdMS_TO_TICKS(200));
    for (;;) {
        processKB_APP();

        // Touch slider: volume control in all modes (rate-limited)
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
                g_needsRedraw = true;
            } else if (scroll < 0 && g_volume > 10) {
                g_volume -= 10;
                g_needsRedraw = true;
            }
        }

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

    static constexpr size_t kBatchSamples = 256;
    int16_t batch[kBatchSamples];

    uint8_t lastVolume = 255;

    uint32_t last_isr = 0;
    unsigned long lastDebug = 0;

    for (;;) {
        // Debug: print ISR rate and decoder stats every ~5 seconds
        {
            unsigned long now = millis();
            static unsigned long lastLog = 0;
            if (now - lastLog >= 5000) {
                lastLog = now;
                uint32_t cur = g_isr_count;
                uint32_t delta = cur - last_isr;
                last_isr = cur;
                Serial.printf("[DBG] ISR=%u delta/5s=%u rate=%u Hz  freeSpace=%zu\n",
                    cur, delta, delta / 5, s_ringbuf.freeSpace());
            }
        }

        if (s_ringbuf.freeSpace() > 2048) {
            player.tick();
        }

        // Drain decoded samples out of the shared ring buffer
        size_t count = 0;
        while (count < kBatchSamples) {
            int16_t sample;
            if (!s_ringbuf.read(sample)) break;
            batch[count++] = sample;
        }


        // Push g_volume changes through to PWM
        uint8_t vol = g_volume;
        if (vol != lastVolume) {
            int8_t pwmVol = (int8_t)((int)vol * 32 / 255) - 16;
            s_pwm.setVolume(pwmVol);
            lastVolume = vol;
        }

        if (count > 0) {
            static bool bdumped = false;
            if (!bdumped) {
                bdumped = true;
                Serial.printf("[BUF] head=%zu tail=%zu avail=%zu free=%zu  count=%zu\n",
                    s_ringbuf.debugHead(), s_ringbuf.debugTail(),
                    s_ringbuf.available(), s_ringbuf.freeSpace(), count);
                int n = count < 48 ? count : 48;
                Serial.printf("[BUF] %d samples -> pwm:", n);
                for (int i = 0; i < n; i++) Serial.printf(" %d", batch[i]);
                Serial.println();
            }

            size_t written = 0;
            while (written < count) {
                size_t n = s_pwm.write(batch + written, count - written);
                if (n == 0) break;
                written += n;
            }
            if (written < (size_t)count) {
                static bool warned = false;
                if (!warned) {
                    warned = true;
                    Serial.printf("[BUF] UNDERFLOW: wrote %zu / %zu samples\n", written, (size_t)count);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void APP_INIT() {
    // Restore saved volume
    prefs.begin("PocketMuse", true);
    g_volume = prefs.getUChar("volume", 200);
    prefs.end();

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
    player.setOutput(&s_pwm);
    s_pwm.begin(44100);
    xTaskCreatePinnedToCore(appTask, "appTask", 32768, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(audioTask, "audioTask", 32768, NULL, 5, NULL, 0);
    APP_INIT();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(100));
}

#endif

#include <globals.h>
#include "ringbuf.h"
#include "decoder.h"
#include "player.h"
#include "output/output_pwm.h"

#if OTA_APP

static RingBuffer s_ringbuf;
static Decoder s_decoder(s_ringbuf);
static Player s_player(s_decoder, s_ringbuf);
static PWMAudioOutput s_pwm(s_ringbuf, BZ_PIN);

void APP_INIT() {
}

void processKB_APP() {
}

void einkHandler_APP() {
}

void applicationEinkHandler() {
    einkHandler_APP();
}

void einkHandler(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(250));
    for (;;) {
        applicationEinkHandler();
        vTaskDelay(pdMS_TO_TICKS(50));
        yield();
    }
}

void audioTask(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(100));

    s_player.play("/music/test.mp3");

    for (;;) {
        if (s_player.state() == PlayerState::Playing) {
            if (s_ringbuf.freeSpace() > 2048) {
                s_player.tick();
            } else {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        } else {
            // Fallback square wave when nothing is playing
            int16_t sample = 0;
            static int phase = 0;
            phase = (phase + 1) % 441;
            sample = (phase < 220) ? 8192 : -8192;
            s_ringbuf.write(sample);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void setup() {
    PocketMage_INIT();
    s_pwm.begin();
    xTaskCreatePinnedToCore(audioTask, "audio", 8192, NULL, 5, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(100));
}

#endif

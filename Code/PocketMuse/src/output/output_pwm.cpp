#include "output_pwm.h"
#include <driver/timer.h>
#include <soc/ledc_struct.h>

static RingBuffer* g_rb = nullptr;

static bool IRAM_ATTR pwm_timer_cb(void* arg) {
    int16_t sample;
    if (g_rb && g_rb->read(sample)) {
        uint32_t duty = (uint32_t)(sample + 32768) >> 8;
        LEDC.channel_group[0].channel[PWMAudioOutput::kLEDCChannel].duty.duty = duty << 4;
        LEDC.channel_group[0].channel[PWMAudioOutput::kLEDCChannel].conf0.low_speed_update = 1;
    }
    return false;
}

PWMAudioOutput::PWMAudioOutput(RingBuffer& rb, int pin)
    : rb_(rb), pin_(pin), running_(false), sample_rate_(44100) {}

PWMAudioOutput::~PWMAudioOutput() {
    stop();
}

bool PWMAudioOutput::begin(int sampleRate) {
    if (running_) return true;

    sample_rate_ = (sampleRate > 0) ? sampleRate : 44100;

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = kResolution,
        .timer_num = kLEDCTimer,
        .freq_hz = kCarrierFreq,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    if (ledc_timer_config(&ledc_timer) != ESP_OK) {
        return false;
    }

    ledc_channel_config_t ledc_channel = {
        .gpio_num = pin_,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = kLEDCChannel,
        .timer_sel = kLEDCTimer,
        .duty = 0,
    };
    if (ledc_channel_config(&ledc_channel) != ESP_OK) {
        return false;
    }

    g_rb = &rb_;

    timer_config_t timer_cfg = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider = 2,
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_cfg);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 40000000 / sample_rate_);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, pwm_timer_cb, NULL, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);

    running_ = true;
    return true;
}

bool PWMAudioOutput::stop() {
    if (!running_) return false;
    timer_pause(TIMER_GROUP_0, TIMER_0);
    timer_disable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_callback_remove(TIMER_GROUP_0, TIMER_0);
    ledc_stop(LEDC_LOW_SPEED_MODE, kLEDCChannel, 0);
    g_rb = nullptr;
    running_ = false;
    return true;
}

bool PWMAudioOutput::isRunning() const {
    return running_;
}

void PWMAudioOutput::pause() {
    if (!running_) return;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, kLEDCChannel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, kLEDCChannel);
    timer_pause(TIMER_GROUP_0, TIMER_0);
}

void PWMAudioOutput::resume() {
    if (!running_) return;
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
}

size_t PWMAudioOutput::write(const int16_t* data, size_t samples) {
    size_t written = 0;
    for (size_t i = 0; i < samples; i++) {
        if (rb_.write(data[i])) {
            written++;
        } else {
            break;
        }
    }
    return written;
}

size_t PWMAudioOutput::availableForWrite() const {
    return rb_.freeSpace();
}

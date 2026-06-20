#include "output_pwm.h"

PWMAudioOutput::PWMAudioOutput(RingBuffer& rb, int pin)
    : rb_(rb), pin_(pin), running_(false), initialized_(false), sample_rate_(44100) {}

PWMAudioOutput::~PWMAudioOutput() {
    stop();
    if (initialized_) {
        pwm_audio_deinit();
        initialized_ = false;
    }
}

bool PWMAudioOutput::begin(int sampleRate) {
    if (sampleRate <= 0) sampleRate = 44100;
    int prev_rate = sample_rate_;
    sample_rate_ = sampleRate;
    bool rate_changed = (sampleRate != prev_rate);

    if (running_ && !rate_changed) return true;

    // Sample rate changed while already running so stop so we can reconfigure.
    if (running_) {
        pwm_audio_stop();   // flushes internal ringbuf, status -> IDLE
        running_ = false;
    }

    if (!initialized_) {
        pwm_audio_config_t pac = {};
        pac.tg_num             = TIMER_GROUP_0;
        pac.timer_num          = TIMER_0;
        pac.gpio_num_left      = pin_;
        pac.gpio_num_right     = -1;          // mono: buzzer is a single pin
        pac.ledc_channel_left  = LEDC_CHANNEL_0;
        pac.ledc_channel_right = LEDC_CHANNEL_1; // unused (gpio_num_right == -1)
        pac.ledc_timer_sel     = LEDC_TIMER_0;
        pac.duty_resolution    = kResolution;
        pac.ringbuf_len        = kRingBufLen;

        if (pwm_audio_init(&pac) != ESP_OK) {
            return false;
        }
        initialized_ = true;
    }

    // 16-bit signed PCM, mono, at the requested sample rate.
    if (pwm_audio_set_param(sample_rate_, (ledc_timer_bit_t)16, 1) != ESP_OK) {
        return false;
    }

    if (pwm_audio_start() != ESP_OK) {
        return false;
    }

    running_ = true;
    return true;
}

bool PWMAudioOutput::stop() {
    if (!running_) return false;
    // pwm_audio_stop() pauses the timer/ISR and flushes its internal ring
    // buffer, but deliberately leaves the PWM signal itself on the pin
    // (reduces switching noise vs. a hard stop)
    pwm_audio_stop();
    running_ = false;
    return true;
}

bool PWMAudioOutput::isRunning() const {
    return running_;
}

void PWMAudioOutput::pause() {
    if (!running_) return;
    pwm_audio_stop();
}

void PWMAudioOutput::resume() {
    if (!initialized_) return;
    pwm_audio_start();
    running_ = true;
}

void PWMAudioOutput::setVolume(int8_t volume) {
    if (!initialized_) return;
    pwm_audio_set_volume(volume);
}

size_t PWMAudioOutput::write(const int16_t* data, size_t samples) {
    if (!running_) return 0;

    size_t bytesWritten = 0;
    pwm_audio_write((uint8_t*)data, samples * sizeof(int16_t), &bytesWritten,
                     pdMS_TO_TICKS(20));

    // pwm_audio_write reports bytes written, not samples; truncated/partial
    // writes are always sample-aligned (2 bytes) per its internal masking,
    // so this division is exact.
    return bytesWritten / sizeof(int16_t);
}

size_t PWMAudioOutput::availableForWrite() const {
    // pwm_audio doesn't expose free-space directly; rb_ is no longer the
    // actual audio path, so just report a generous constant. Callers in
    // this codebase (Player::tick) only use this as a rough "is there
    // room" check before calling write(), and pwm_audio_write() itself
    // blocks/truncates safely if its internal buffer is full.
    return kRingBufLen;
}

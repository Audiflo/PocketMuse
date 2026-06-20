#pragma once
#include "output.h"
#include "ringbuf.h"
#include "pwm_audio.h"

// Thin AudioOutput adapter around Espressif's pwm_audio component (components/pwm_audio)
class PWMAudioOutput : public AudioOutput {
public:
    // Hardware runs at 8-bit duty resolution. PCM input is 16-bit signed;
    // pwm_audio shifts right by (16 - 8) = 8 internally.
    static constexpr ledc_timer_bit_t kResolution = LEDC_TIMER_8_BIT;
    static constexpr int              kPcmBits    = 16;
    static constexpr size_t kRingBufLen = 1024 * 8;

    PWMAudioOutput(RingBuffer& rb, int pin);
    ~PWMAudioOutput();

    bool begin(int sampleRate = 44100) override;
    bool stop() override;
    bool isRunning() const override;
    void pause() override;
    void resume() override;

    size_t write(const int16_t* data, size_t samples) override;
    size_t availableForWrite() const override;

    int sampleRate() const override { return sample_rate_; }

    // Volume in pwm_audio's native range: -16 (mute) .. 0 (unity) .. 16 (2x).
    // Call after begin().
    void setVolume(int8_t volume);

private:
    RingBuffer& rb_;   // unused for audio data now; kept for interface compat
    int pin_;
    bool running_;
    bool initialized_;
    int sample_rate_;
};

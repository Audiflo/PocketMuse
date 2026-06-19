#pragma once
#include "output.h"
#include "ringbuf.h"
#include <driver/ledc.h>

class PWMAudioOutput : public AudioOutput {
public:
    static constexpr ledc_timer_t kLEDCTimer = LEDC_TIMER_0;
    static constexpr ledc_channel_t kLEDCChannel = LEDC_CHANNEL_0;
    static constexpr int kCarrierFreq = 250000;
    static constexpr ledc_timer_bit_t kResolution = LEDC_TIMER_8_BIT;

    PWMAudioOutput(RingBuffer& rb, int pin);
    ~PWMAudioOutput();

    bool begin(int sampleRate = 44100) override;
    bool stop() override;
    bool isRunning() const override;
    void pause() override;
    void resume() override;

    size_t write(const int16_t* data, size_t samples) override;
    size_t availableForWrite() const override;

    int sampleRate() const { return sample_rate_; }

private:
    RingBuffer& rb_;
    int pin_;
    bool running_;
    int sample_rate_;
};

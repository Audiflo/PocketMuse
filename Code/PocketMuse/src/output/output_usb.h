#pragma once
#include "output.h"
#include "ringbuf.h"
#include <USB_STREAM.h>

class USBStreamOutput : public AudioOutput {
public:
    USBStreamOutput(RingBuffer& rb, int muxPin);
    ~USBStreamOutput();

    bool begin(int sampleRate = 44100) override;
    bool stop() override;
    bool isRunning() const override;

    size_t write(const int16_t* data, size_t samples) override;
    size_t availableForWrite() const override;

    int sampleRate() const { return spk_rate_; }
    int channels() const { return spk_channels_; }

private:
    static constexpr int kFeedIntervalMs = 5;

    static void feedTask(void* arg);

    RingBuffer&  rb_;
    USB_STREAM*  usb_;
    int          mux_pin_;
    bool         running_;
    int          spk_rate_;
    int          spk_channels_;
    TaskHandle_t feed_handle_;

    uint16_t feed_buf_[512];
};

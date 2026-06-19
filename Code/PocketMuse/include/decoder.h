#pragma once
#include <stdint.h>
#include <stddef.h>

class RingBuffer;

class Decoder {
public:
    Decoder(RingBuffer& rb);
    ~Decoder();

    size_t input(const uint8_t* data, size_t len);
    int process();
    void reset();

    int sampleRate() const { return sample_rate_; }
    int channels() const { return num_channels_; }
    int bitrate() const   { return bitrate_; }

private:
    static constexpr size_t kInputBufSize  = 8192;
    static constexpr size_t kOutputSamples = 1152;

    void compact_();

    RingBuffer& rb_;
    void*       dec_;

    uint8_t  input_buf_[kInputBufSize];
    size_t   input_len_;
    size_t   input_pos_;

    int16_t  output_buf_[kOutputSamples];

    int sample_rate_;
    int num_channels_;
    int bitrate_;
};

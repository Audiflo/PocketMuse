#pragma once
#include <stdint.h>
#include <stddef.h>
#include "minimp3.h"

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
    static constexpr size_t kInputBufSize  = 16384;
    static constexpr size_t kOutputSamples = 2304;

    void compact_();

    RingBuffer& rb_;
    mp3dec_t    dec_;

    uint8_t  input_buf_[kInputBufSize];
    size_t   input_len_;
    size_t   input_pos_;

    int16_t  output_buf_[kOutputSamples];

    int sample_rate_;
    int num_channels_;
    int bitrate_;
};

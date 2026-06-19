#include "decoder.h"
#include "ringbuf.h"
#include <mp3dec.h>
#include <cstring>
#include <algorithm>
#include <Arduino.h>

Decoder::Decoder(RingBuffer& rb)
    : rb_(rb)
    , dec_(nullptr)
    , input_len_(0)
    , input_pos_(0)
    , sample_rate_(0)
    , num_channels_(0)
    , bitrate_(0)
{
    dec_ = MP3InitDecoder();
    Serial.printf("[decoder] init: %s\n", dec_ ? "OK" : "FAILED (out of memory?)");
}

Decoder::~Decoder() {
    if (dec_) MP3FreeDecoder(dec_);
}

void Decoder::compact_() {
    if (input_pos_ >= input_len_) {
        input_len_ = 0;
        input_pos_ = 0;
    } else if (input_pos_ > 0) {
        size_t rem = input_len_ - input_pos_;
        memmove(input_buf_, input_buf_ + input_pos_, rem);
        input_len_ = rem;
        input_pos_ = 0;
    }
}

size_t Decoder::input(const uint8_t* data, size_t len) {
    compact_();
    size_t free = kInputBufSize - input_len_;
    size_t copy = std::min(len, free);
    if (copy > 0) {
        memcpy(input_buf_ + input_len_, data, copy);
        input_len_ += copy;
    }
    return copy;
}

int Decoder::process() {
    if (!dec_) return -1;

    size_t avail = input_len_ - input_pos_;
    if (avail < 2) {
        Serial.printf("[dec] underflow: no data (len=%u pos=%u)\n", input_len_, input_pos_);
        return 0;
    }

    int off = MP3FindSyncWord(input_buf_ + input_pos_, avail);
    if (off < 0) {
        Serial.printf("[dec] no sync word in %u bytes (pos=%u len=%u)\n",
            avail, input_pos_, input_len_);
        input_pos_ = input_len_;
        return 0;
    }
    input_pos_ += off;

    const unsigned char* inp  = input_buf_ + input_pos_;
    size_t               left = input_len_ - input_pos_;

    int ret = MP3Decode(dec_, &inp, &left, output_buf_, 0);

    // Advance past whatever helix consumed
    input_pos_ = (const uint8_t*)inp - input_buf_;

    if (ret < 0) {
        if (ret == ERR_MP3_INDATA_UNDERFLOW ||
            ret == ERR_MP3_MAINDATA_UNDERFLOW) {
            return 0;
        }
        // Corrupt frame, skip one frame's worth (~2KB worst case)
        if (input_pos_ + 2048 <= input_len_) {
            input_pos_ += 2048;
        } else {
            input_pos_ = input_len_;
        }
        return ret;
    }

    // Update stream info
    MP3FrameInfo info;
    MP3GetLastFrameInfo(dec_, &info);
    Serial.printf("[dec] sr=%d ch=%d samps=%d\n", info.samprate, info.nChans, info.outputSamps);
    sample_rate_    = info.samprate;
    num_channels_   = info.nChans;
    bitrate_        = info.bitrate;

    int total = info.outputSamps;
    if (total > kOutputSamples) total = kOutputSamples;
    size_t written = 0;
    for (int i = 0; i < total; i++) {
        if (rb_.write(output_buf_[i])) {
            written++;
        } else {
            break;
        }
    }
    return written;
}

void Decoder::reset() {
    if (dec_) MP3FreeDecoder(dec_);
    dec_ = MP3InitDecoder();
    input_len_ = 0;
    input_pos_ = 0;
    sample_rate_  = 0;
    num_channels_ = 0;
    bitrate_      = 0;
}

#include "decoder.h"
#include "ringbuf.h"
#include <mp3dec.h>
#include <cstring>
#include <algorithm>

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
}

Decoder::~Decoder() {
    if (dec_) MP3FreeDecoder(dec_);
}

void Decoder::compact_() {
    if (input_pos_ > 0 && input_pos_ < input_len_) {
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

    // Need at least a sync word (2 bytes) to attempt decode
    size_t avail = input_len_ - input_pos_;
    if (avail < 2) return 0;

    // Find next sync word
    int off = MP3FindSyncWord(input_buf_ + input_pos_, avail);
    if (off < 0) {
        input_pos_ = input_len_;
        return 0;
    }
    input_pos_ += off;

    // Decode one frame
    const unsigned char* inp  = input_buf_ + input_pos_;
    size_t               left = input_len_ - input_pos_;
    int outSamps = kOutputSamples;

    int ret = MP3Decode(dec_, &inp, &left, output_buf_, kOutputSamples / 2);

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
    sample_rate_    = info.samprate;
    num_channels_   = info.nChans;
    bitrate_        = info.bitrate;

    int total = ret * num_channels_;
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

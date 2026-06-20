#include "decoder.h"
#include "ringbuf.h"
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <cstring>
#include <algorithm>
#include <Arduino.h>

Decoder::Decoder(RingBuffer& rb)
    : rb_(rb)
    , input_len_(0)
    , input_pos_(0)
    , sample_rate_(0)
    , num_channels_(0)
    , bitrate_(0)
{
    mp3dec_init(&dec_);
    Serial.printf("[decoder] init: OK (minimp3)\n");
}

Decoder::~Decoder() {}

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
    mp3dec_frame_info_t info;
    int samples = mp3dec_decode_frame(&dec_, input_buf_ + input_pos_, input_len_ - input_pos_, output_buf_, &info);

    input_pos_ += info.frame_bytes;

    if (info.frame_bytes == 0) return 0;  // no sync found, need more data
    if (samples <= 0) return -1;          // frame consumed but no audio (Xing/Info header)

    sample_rate_    = info.hz;
    num_channels_   = info.channels;
    bitrate_        = info.bitrate_kbps;

    int maxSamples = (int)(kOutputSamples / (info.channels > 0 ? info.channels : 1));
    int total = (samples > maxSamples) ? maxSamples : samples;

    static int framedump = 0;
    if (framedump < 5 && total > 0) {
        int nsamp = (info.channels == 2) ? total * 2 : total;
        int nshow = nsamp < 48 ? nsamp : 48;
        Serial.printf("[DEC#%d] %d ch total=%d pairs=%d (sr=%d):", framedump, info.channels, nsamp, (info.channels == 2) ? total : total, info.hz);
        for (int i = 0; i < nshow; i++) Serial.printf(" %d", output_buf_[i]);
        Serial.println();
        framedump++;
    }

    size_t written = 0;
    if (info.channels == 2) {
        // minimp3 returns samples-per-channel. For stereo this means
        // output_buf_ has total*2 interleaved L,R pairs. Downmix to mono.
        int pairs = total;
        for (int i = 0; i < pairs; i++) {
            int32_t mixed = ((int32_t)output_buf_[i * 2] + (int32_t)output_buf_[i * 2 + 1]) / 2;
            if (rb_.write((int16_t)mixed)) {
                written++;
            } else {
                break;
            }
        }
    } else {
        for (int i = 0; i < total; i++) {
            if (rb_.write(output_buf_[i])) {
                written++;
            } else {
                break;
            }
        }
    }
    return written;
}

void Decoder::reset() {
    mp3dec_init(&dec_);
    input_len_ = 0;
    input_pos_ = 0;
    sample_rate_  = 0;
    num_channels_ = 0;
    bitrate_      = 0;
}

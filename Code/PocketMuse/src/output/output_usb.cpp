#include "output_usb.h"
#include <Arduino.h>

USBStreamOutput::USBStreamOutput(RingBuffer& rb, int muxPin)
    : rb_(rb)
    , usb_(nullptr)
    , mux_pin_(muxPin)
    , running_(false)
    , spk_rate_(48000)
    , spk_channels_(2)
    , feed_handle_(nullptr)
{
}

USBStreamOutput::~USBStreamOutput() {
    stop();
}

bool USBStreamOutput::begin(int sampleRate) {
    if (running_) return true;
    running_ = true;

    BaseType_t ret = xTaskCreatePinnedToCore(
        feedTask, "usb_feed", 8192, this, 6, &feed_handle_, 0
    );
    if (ret != pdPASS) {
        running_ = false;
        return false;
    }
    return true;
}

bool USBStreamOutput::stop() {
    if (!running_) return false;
    running_ = false;

    if (feed_handle_) {
        vTaskDelay(pdMS_TO_TICKS(50));
        vTaskDelete(feed_handle_);
        feed_handle_ = nullptr;
    }

    if (usb_) {
        usb_->stop();
        delete usb_;
        usb_ = nullptr;
    }

    return true;
}

bool USBStreamOutput::isRunning() const {
    return running_;
}

size_t USBStreamOutput::write(const int16_t* data, size_t samples) {
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

size_t USBStreamOutput::availableForWrite() const {
    return rb_.freeSpace();
}

// SRC: 44.1kHz (source) → spk_rate_ (USB speaker)
// step is the input advance per output sample in Q16.16
// step = source_rate / spk_rate * 2^16
static uint32_t src_step(int srcRate, int spkRate) {
    uint64_t s = (uint64_t)srcRate << 16;
    return (uint32_t)(s / spkRate);
}

void USBStreamOutput::feedTask(void* arg) {
    USBStreamOutput* self = (USBStreamOutput*)arg;

    pinMode(self->mux_pin_, OUTPUT);
    digitalWrite(self->mux_pin_, HIGH);
    delay(100);

    self->usb_ = new USB_STREAM();

    self->usb_->uacConfiguration(
        0, 0, 0, 0,
        UAC_CH_ANY, UAC_BITS_ANY, UAC_FREQUENCY_ANY, 6400
    );

    self->usb_->start();
    self->usb_->connectWait(10000);

    uac_frame_size_t fsize;
    if (self->usb_->uacSpkGetFrameSize(&fsize) != nullptr &&
        fsize.samples_frequence > 0) {
        self->spk_rate_    = fsize.samples_frequence;
        self->spk_channels_ = fsize.ch_num > 0 ? fsize.ch_num : 2;
    } else {
        self->spk_rate_    = 48000;
        self->spk_channels_ = 2;
    }

    uint32_t step = src_step(44100, self->spk_rate_);
    uint32_t pos  = 0;

    int16_t s0 = 0, s1 = 0;
    bool havePair = self->rb_.read(s0);
    if (havePair) havePair = self->rb_.read(s1);
    if (!havePair) s1 = s0;

    while (self->running_) {
        int outSamps = self->spk_rate_ * kFeedIntervalMs / 1000;

        for (int i = 0; i < outSamps; i++) {
            uint32_t frac = pos;

            int32_t interp = (int32_t)s0 * (int32_t)(0x10000 - frac) +
                             (int32_t)s1 * (int32_t)frac;
            interp >>= 16;

            if (interp < -32768) interp = -32768;
            if (interp > 32767)  interp = 32767;

            uint16_t us = (uint16_t)(interp + 32768);
            self->feed_buf_[i * 2]     = us;
            self->feed_buf_[i * 2 + 1] = us;

            pos += step;
            while (pos >= 0x10000) {
                s0 = s1;
                if (!self->rb_.read(s1)) s1 = s0;
                pos -= 0x10000;
            }
        }

        size_t bytes = (size_t)outSamps * self->spk_channels_ * sizeof(uint16_t);
        self->usb_->uacWriteSpk(self->feed_buf_, bytes, 100);
    }

    self->usb_->stop();
    delete self->usb_;
    self->usb_ = nullptr;

    digitalWrite(self->mux_pin_, LOW);

    vTaskDelete(NULL);
}

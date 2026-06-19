#pragma once
#include <stdint.h>
#include <stddef.h>

class RingBuffer {
public:
    static constexpr size_t kSize = 8192;
    static constexpr size_t kMask = kSize - 1;

    RingBuffer();

    bool write(int16_t sample);
    bool read(int16_t& sample);
    size_t available() const;
    size_t freeSpace() const;
    void reset();

private:
    int16_t buf_[kSize];
    volatile size_t head_;
    volatile size_t tail_;
};

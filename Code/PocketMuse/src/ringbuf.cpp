#include "ringbuf.h"
#include "muse.h"

RingBuffer::RingBuffer() : head_(0), tail_(0) {}

bool RingBuffer::write(int16_t sample) {
    size_t next = (head_ + 1) & kMask;
    if (next == tail_) {
        return false;
    }
    buf_[head_] = sample;
    head_ = next;
    return true;
}

bool RingBuffer::read(int16_t& sample) {
    if (head_ == tail_) {
        return false;
    }
    
    sample = buf_[tail_];
    tail_ = (tail_ + 1) & kMask;
    return true;
}

size_t RingBuffer::available() const {
    return (head_ - tail_) & kMask;
}

size_t RingBuffer::freeSpace() const {
    return kSize - 1 - available();
}

void RingBuffer::reset() {
    head_ = 0;
    tail_ = 0;
}

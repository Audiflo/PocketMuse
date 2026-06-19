#pragma once
#include <stdint.h>
#include <stddef.h>

class AudioOutput {
public:
    virtual ~AudioOutput() = default;

    virtual bool begin() = 0;
    virtual bool stop() = 0;
    virtual bool isRunning() const = 0;

    virtual size_t write(const int16_t* data, size_t samples) = 0;
    virtual size_t availableForWrite() const = 0;

protected:
    AudioOutput() = default;
};

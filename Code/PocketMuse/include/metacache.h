#pragma once
#include <Arduino.h>
#include <FS.h>
#include <stdint.h>

class MetadataCache {
public:
    bool load(const char* path, char* title, size_t tSize,
              char* artist, size_t aSize, char* album, size_t lSize);
    void save(const char* path, const char* title, const char* artist, const char* album);
    void clear();

private:
    static constexpr const char* kPath = "/music/.muse/metadata.bin";
    static constexpr const char* kTmp  = "/music/.muse/.meta_tmp";
    static constexpr uint32_t kMagic = 0x4D455441;

    bool readStr(File& f, char* buf, size_t bufSize);
    bool skipStr(File& f);
};

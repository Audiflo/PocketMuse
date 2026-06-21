#pragma once
#include <stdint.h>
#include <stddef.h>
#include <FS.h>

class AlbumArt {
public:
    static constexpr int kMaxSize = 120;

    enum Format { kNone, kJPEG, kPNG };

    AlbumArt();
    ~AlbumArt();

    bool load(const char* mp3Path);
    void clear();

    bool   hasArt() const { return format_ != kNone; }
    Format format() const { return format_; }
    bool   isJPEG() const { return format_ == kJPEG; }
    bool   isPNG()  const { return format_ == kPNG; }

    int  width() const  { return img_w_; }
    int  height() const { return img_h_; }

    bool render1Bit(uint8_t* einkBuf, int bufW, int bufH,
                    int outX, int outY, int maxSize = kMaxSize);

private:
    static constexpr const char* kCachePath = "/music/.muse/cache_art.jpg";

    bool ensureMuseDir_();
    bool extractAPIC_(const char* mp3Path);
    bool detectFormat_();
    bool renderJPEG_(File& file, uint8_t* einkBuf, int bufW, int bufH,
                     int outX, int outY, int maxSize);
    bool renderPNG_(File& file, uint8_t* einkBuf, int bufW, int bufH,
                    int outX, int outY, int maxSize);

    Format format_;
    int  img_w_;
    int  img_h_;
};

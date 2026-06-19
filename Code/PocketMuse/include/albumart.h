#pragma once
#include <stdint.h>
#include <stddef.h>

class AlbumArt {
public:
    static constexpr int kMaxSize = 120;

    AlbumArt();
    ~AlbumArt();

    bool load(const char* mp3Path);
    void clear();

    bool hasArt() const { return has_art_; }
    bool isJPEG() const  { return is_jpeg_; }

    int  width() const  { return img_w_; }
    int  height() const { return img_h_; }

    bool render1Bit(uint8_t* einkBuf, int bufW, int bufH,
                    int outX, int outY, int maxSize = kMaxSize);

private:
    static constexpr const char* kCachePath = "/music/.muse/cache_art.jpg";

    bool ensureMuseDir_();
    bool extractAPIC_(const char* mp3Path);

    bool has_art_;
    bool is_jpeg_;
    int  img_w_;
    int  img_h_;
};

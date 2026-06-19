#include "albumart.h"
#include <globals.h>
#include <FS.h>
#include <JPEGDEC.h>
#include <cstring>

AlbumArt::AlbumArt()
    : has_art_(false), is_jpeg_(false), img_w_(0), img_h_(0)
{
}

AlbumArt::~AlbumArt() {
    clear();
}

bool AlbumArt::ensureMuseDir_() {
    if (!global_fs) return false;
    global_fs->mkdir("/music/.muse");
    return true;
}

static uint32_t readSyncsafeInt(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 21) |
           ((uint32_t)buf[1] << 14) |
           ((uint32_t)buf[2] <<  7) |
           (uint32_t)buf[3];
}

bool AlbumArt::extractAPIC_(const char* mp3Path) {
    if (!global_fs) return false;

    File mp3 = global_fs->open(mp3Path, "r");
    if (!mp3) return false;

    uint8_t hdr[10];
    if (mp3.read(hdr, 10) != 10) { mp3.close(); return false; }
    if (hdr[0] != 'I' || hdr[1] != 'D' || hdr[2] != '3') { mp3.close(); return false; }

    uint8_t  ver     = hdr[3];
    uint8_t  flags   = hdr[5];
    uint32_t tagSize = readSyncsafeInt(hdr + 6);

    if (ver < 3 || ver > 4) { mp3.close(); return false; }

    // Skip extended header
    if (flags & 0x40) {
        uint8_t ext[4];
        if (mp3.read(ext, 4) != 4) { mp3.close(); return false; }
        uint32_t extSize = (ver == 4) ? readSyncsafeInt(ext)
            : ((uint32_t)ext[0] << 24 | (uint32_t)ext[1] << 16 |
               (uint32_t)ext[2] <<  8 | ext[3]);
        mp3.seek(extSize - 4, SeekCur);
    }

    bool footer = (ver == 4) && (flags & 0x10);
    size_t remain = tagSize - (footer ? 10 : 0);

    global_fs->remove(kCachePath);
    File tmp = global_fs->open(kCachePath, "w");
    if (!tmp) { mp3.close(); return false; }

    bool found = false;

    while (remain >= 10) {
        uint8_t fhdr[10];
        if (mp3.read(fhdr, 10) != 10) break;
        remain -= 10;

        char id[5] = {};
        memcpy(id, fhdr, 4);
        if (id[0] == '\0') break;

        uint32_t fsize = ((uint32_t)fhdr[4] << 24) |
                         ((uint32_t)fhdr[5] << 16) |
                         ((uint32_t)fhdr[6] <<  8) |
                         fhdr[7];
        if (fsize == 0 || fsize > remain) break;

        if (strcmp(id, "APIC") != 0) {
            mp3.seek(fsize, SeekCur);
            remain -= fsize;
            continue;
        }

        // APIC body: encoding(1) + MIME(null-term) + type(1) + desc(null-term) + image_data
        uint8_t encoding;
        if (mp3.read(&encoding, 1) != 1) break;
        fsize--; remain--;

        for (;;) {
            uint8_t b;
            if (mp3.read(&b, 1) != 1 || fsize == 0) break;
            fsize--; remain--;
            if (b == 0) break;
        }

        is_jpeg_ = true;

        if (fsize == 0) break;
        mp3.seek(1, SeekCur);
        fsize--; remain--;

        if (encoding == 0x00 || encoding == 0x03) {
            for (;;) {
                uint8_t b;
                if (mp3.read(&b, 1) != 1 || fsize == 0) break;
                fsize--; remain--;
                if (b == 0) break;
            }
        } else {
            for (;;) {
                uint8_t b0, b1;
                if (mp3.read(&b0, 1) != 1 || mp3.read(&b1, 1) != 1 || fsize < 2) break;
                fsize -= 2; remain -= 2;
                if (b0 == 0 && b1 == 0) break;
            }
        }

        uint8_t buf[512];
        while (fsize > 0) {
            size_t chunk = (fsize > sizeof(buf)) ? sizeof(buf) : fsize;
            size_t rd = mp3.read(buf, chunk);
            if (rd == 0) break;
            tmp.write(buf, rd);
            fsize -= rd;
            remain -= rd;
        }

        found = true;
        break;
    }

    tmp.close();
    mp3.close();

    if (found) {
        has_art_ = true;
    } else {
        global_fs->remove(kCachePath);
    }

    return found;
}

bool AlbumArt::load(const char* mp3Path) {
    if (has_art_) clear();
    if (!ensureMuseDir_()) return false;
    return extractAPIC_(mp3Path);
}

void AlbumArt::clear() {
    if (has_art_ && global_fs) {
        global_fs->remove(kCachePath);
    }
    has_art_ = false;
    is_jpeg_ = false;
    img_w_ = img_h_ = 0;
}

// Context passed to the JPEGDEC draw callback
struct RenderCtx {
    uint8_t* einkBuf;
    int bufW, bufH;
    int outX, outY;
    int fullW, fullH; // full-res decoded dimensions
    int outW, outH;   // target output dimensions
};

// Callback receives packed 1-bit data (Floyd-Steinberg dithered by JPEGDEC).
// Maps full-resolution pixels to target dimensions via nearest-neighbor.
static int artDrawCb(JPEGDRAW* pDraw) {
    RenderCtx* ctx = (RenderCtx*)pDraw->pUser;
    if (!ctx || !ctx->einkBuf) return 0;

    uint8_t* src = (uint8_t*)pDraw->pPixels;
    int srcW = pDraw->iWidth;
    int srcH = pDraw->iHeight;
    int srcRowBytes = (srcW + 7) / 8;
    int srcBaseY = pDraw->y;

    int dstRowBytes = (ctx->bufW + 7) / 8;

    for (int sy = 0; sy < srcH; sy++) {
        int fy = srcBaseY + sy;  // row in the full-res decoded image
        if (fy >= ctx->fullH) break;

        int oy = fy * ctx->outH / ctx->fullH;
        if (oy >= ctx->outH) continue;
        int einkY = ctx->outY + oy;
        if (einkY >= ctx->bufH) continue;

        for (int sx = 0; sx < srcW; sx++) {
            int fx = pDraw->x + sx;
            if (fx >= ctx->fullW) break;

            int ox = fx * ctx->outW / ctx->fullW;
            if (ox >= ctx->outW) continue;
            int einkX = ctx->outX + ox;
            if (einkX >= ctx->bufW) continue;

            // Read 1-bit pixel from packed source (MSB first)
            int bit = (src[sy * srcRowBytes + (sx >> 3)] >> (7 - (sx & 7))) & 1;

            // Write to E-Ink 1bpp buffer (also MSB first)
            int idx = einkY * dstRowBytes + (einkX >> 3);
            if (idx < ctx->bufH * dstRowBytes) {
            if (!bit) {
                ctx->einkBuf[idx] |= (1 << (7 - (einkX & 7)));
            } else {
                ctx->einkBuf[idx] &= ~(1 << (7 - (einkX & 7)));
            }
            }
        }
    }

    return 1;
}

bool AlbumArt::render1Bit(uint8_t* einkBuf, int bufW, int bufH,
                          int outX, int outY, int maxSize)
{
    if (!has_art_ || !global_fs) return false;

    File file = global_fs->open(kCachePath, "r");
    if (!file) return false;

    JPEGDEC jpeg;
    if (!jpeg.open(file, artDrawCb)) {
        file.close();
        return false;
    }

    int imgW = jpeg.getWidth();
    int imgH = jpeg.getHeight();
    if (imgW <= 0 || imgH <= 0) {
        jpeg.close(); file.close();
        return false;
    }

    // Determine subsampling to compute MCU dimensions
    int ss  = jpeg.getSubSample();
    int mcuCX, mcuCY;
    switch (ss) {
        case 0x12: mcuCX = 8;  mcuCY = 16; break;
        case 0x21: mcuCX = 16; mcuCY = 8;  break;
        case 0x22: mcuCX = 16; mcuCY = 16; break;
        default:   mcuCX = 8;  mcuCY = 8;  break; // 0x00/0x11
    }

    float scale = (float)maxSize / (imgW > imgH ? imgW : imgH);
    int outW = (int)(imgW * scale);
    int outH = (int)(imgH * scale);
    if (outW < 1) outW = 1;
    if (outH < 1) outH = 1;

    img_w_ = outW;
    img_h_ = outH;

    // Decode at full resolution into strips of ~128 pixels wide
    int stripMCUs = 128 / mcuCX;
    if (stripMCUs < 1) stripMCUs = 1;

    // ONE_BIT_DITHERED mode overrides iMaxMCUs to the full image MCU width
    // so the dither buffer must hold the grayscale
    // output for the entire image row, not just the strip.
    int ditherW = ((imgW + mcuCX - 1) / mcuCX) * mcuCX;
    int ditherSize = ditherW * mcuCY;
    uint8_t* ditherBuf = new (std::nothrow) uint8_t[ditherSize];
    if (!ditherBuf) {
        jpeg.close(); file.close();
        return false;
    }

    // Output context for the callback
    RenderCtx ctx;
    ctx.einkBuf = einkBuf;
    ctx.bufW  = bufW;
    ctx.bufH  = bufH;
    ctx.outX  = outX;
    ctx.outY  = outY;
    ctx.fullW = imgW;
    ctx.fullH = imgH;
    ctx.outW  = outW;
    ctx.outH  = outH;

    jpeg.setPixelType(ONE_BIT_DITHERED);
    jpeg.setMaxOutputSize(stripMCUs);
    jpeg.setUserPointer(&ctx);

    bool ok = jpeg.decodeDither(0, 0, ditherBuf, 0);

    jpeg.close();
    file.close();
    delete[] ditherBuf;

    return ok;
}

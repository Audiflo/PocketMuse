#include "albumart.h"
#include <globals.h>
#include <FS.h>
#include <JPEGDEC.h>
#include <PNGdec.h>
#include <cstring>

// Helpers
static uint32_t readSyncsafeInt(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 21) |
           ((uint32_t)buf[1] << 14) |
           ((uint32_t)buf[2] <<  7) |
           (uint32_t)buf[3];
}

// PNGdec file callbacks
static void* pngOpenCb(const char* filename, int32_t* pSize) {
    File* f = new File(global_fs->open(filename, "r"));
    if (!*f) { delete f; return nullptr; }
    *pSize = f->size();
    return f;
}

static void pngCloseCb(void* pHandle) {
    if (!pHandle) return;
    delete static_cast<File*>(pHandle);
}

static int32_t pngReadCb(PNGFILE* pFile, uint8_t* buf, int32_t iLen) {
    auto* f = static_cast<File*>(pFile->fHandle);
    return f ? f->read(buf, iLen) : 0;
}

static int32_t pngSeekCb(PNGFILE* pFile, int32_t iPos) {
    auto* f = static_cast<File*>(pFile->fHandle);
    if (!f) return -1;
    return f->seek(iPos) ? iPos : -1;
}

// Render context shared by JPEG and PNG code paths
struct RenderCtx {
    uint8_t* einkBuf;
    int bufW, bufH;
    int outX, outY;
    int fullW, fullH;
    int outW, outH;
};

// PNG-specific extension: Floyd–Steinberg dither state
struct PngRenderCtx : RenderCtx {
    int pixelType;          // PNG_PIXEL_xxx
    int16_t* err;           // current-row error (size fullW + 2)
    int16_t* nextErr;       // next-row accumulation (size fullW + 2)
};

// Grayscale extraction (single pixel from PNGDRAW)
static int pixelToGray(PNGDRAW* pDraw, int x) {
    // pDraw->pPalette is always non-null (points to ucPalette[] in the PNGIMAGE
    // struct), even for non-palette images where it contains only zeros.
    // Must check the pixel type, not the pointer.
    uint8_t* p = pDraw->pPixels;
    PngRenderCtx* ctx = (PngRenderCtx*)pDraw->pUser;
    int type = ctx->pixelType;

    if (type == PNG_PIXEL_INDEXED) {
        uint8_t* pal = pDraw->pPalette;
        int idx = p[x];
        int r = pal[idx * 3];
        int g = pal[idx * 3 + 1];
        int b = pal[idx * 3 + 2];
        return (r * 77 + g * 150 + b * 29) >> 8;
    }
    if (type == PNG_PIXEL_TRUECOLOR || type == PNG_PIXEL_TRUECOLOR_ALPHA) {
        int i = x * (type == PNG_PIXEL_TRUECOLOR_ALPHA ? 4 : 3);
        int r = p[i];
        int g = p[i + 1];
        int b = p[i + 2];
        return (r * 77 + g * 150 + b * 29) >> 8;
    }
    // Grayscale / grayscale+alpha
    if (type == PNG_PIXEL_GRAY_ALPHA)
        return p[x * 2];
    return p[x];
}

// PNG draw callback: Floyd–Steinberg dither + nearest-neighbour scale
static int pngDrawCb(PNGDRAW* pDraw) {
    PngRenderCtx* ctx = (PngRenderCtx*)pDraw->pUser;
    if (!ctx || !ctx->einkBuf) return 0;

    int y        = pDraw->y;
    int w        = pDraw->iWidth;
    int fullW    = ctx->fullW;
    int outH     = ctx->outH;
    int outW     = ctx->outW;
    int16_t* err = ctx->err;
    int16_t* nxt = ctx->nextErr;

    int oy = y * outH / ctx->fullH;
    bool writeOutput = (oy < outH);

    int dstRowBytes = (ctx->bufW + 7) / 8;
    int prevOx = -1;

    for (int x = 0; x < w; x++) {
        int gray = pixelToGray(pDraw, x);
        int v = gray + err[x + 1];
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        int bit = v > 127 ? 1 : 0;         // 1 = white, 0 = black
        int qErr = v - (bit ? 255 : 0);     // always ≤ 0 with this threshold

        // Floyd–Steinberg error diffusion
        err[x + 2]    += (qErr * 7) / 16;   // right
        nxt[x]        += (qErr * 5) / 16;   // bottom
        nxt[x + 1]    += (qErr * 1) / 16;   // bottom-right
        if (x > 0)
            nxt[x - 1] += (qErr * 3) / 16;  // bottom-left

        if (!writeOutput) continue;

        // Nearest-neighbour downscale
        int ox = x * outW / fullW;
        if (ox == prevOx) continue;
        prevOx = ox;
        if (ox >= outW) continue;

        int einkX = ctx->outX + ox;
        int einkY = ctx->outY + oy;
        if (einkX >= ctx->bufW || einkY >= ctx->bufH) continue;

        int idx = einkY * dstRowBytes + (einkX >> 3);
        if (idx >= ctx->bufH * dstRowBytes) continue;

        if (!bit) {
            ctx->einkBuf[idx] |=  (1 << (7 - (einkX & 7)));
        } else {
            ctx->einkBuf[idx] &= ~(1 << (7 - (einkX & 7)));
        }
    }

    // Swap error buffers for next row
    ctx->err     = nxt;
    ctx->nextErr = err;
    memset(ctx->nextErr, 0, (fullW + 2) * sizeof(int16_t));
    return 1;
}

// JPEG draw callback
static int jpegDrawCb(JPEGDRAW* pDraw) {
    RenderCtx* ctx = (RenderCtx*)pDraw->pUser;
    if (!ctx || !ctx->einkBuf) return 0;

    uint8_t* src  = (uint8_t*)pDraw->pPixels;
    int srcW      = pDraw->iWidth;
    int srcH      = pDraw->iHeight;
    int srcRowB   = (srcW + 7) / 8;
    int srcBaseY  = pDraw->y;

    int dstRowB   = (ctx->bufW + 7) / 8;

    for (int sy = 0; sy < srcH; sy++) {
        int fy = srcBaseY + sy;
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

            int bit = (src[sy * srcRowB + (sx >> 3)] >> (7 - (sx & 7))) & 1;

            int idx = einkY * dstRowB + (einkX >> 3);
            if (idx < ctx->bufH * dstRowB) {
                if (!bit) {
                    ctx->einkBuf[idx] |=  (1 << (7 - (einkX & 7)));
                } else {
                    ctx->einkBuf[idx] &= ~(1 << (7 - (einkX & 7)));
                }
            }
        }
    }
    return 1;
}

// AlbumArt implementation
AlbumArt::AlbumArt()
    : format_(kNone), img_w_(0), img_h_(0)
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

// ID3v2 APIC extraction
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

    if (flags & 0x40) {
        uint8_t ext[4];
        if (mp3.read(ext, 4) != 4) { mp3.close(); return false; }
        uint32_t extSize = (ver == 4) ? readSyncsafeInt(ext)
            : ((uint32_t)ext[0] << 24 | (uint32_t)ext[1] << 16 |
               (uint32_t)ext[2] <<  8 | ext[3]);
        mp3.seek(extSize, SeekCur);
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

        uint32_t fsize = (ver == 4)
            ? readSyncsafeInt(fhdr + 4)
            : ((uint32_t)fhdr[4] << 24 | (uint32_t)fhdr[5] << 16 |
               (uint32_t)fhdr[6] <<  8 | fhdr[7]);
        if (fsize == 0 || fsize > remain) break;

        if (strcmp(id, "APIC") != 0) {
            mp3.seek(fsize, SeekCur);
            remain -= fsize;
            continue;
        }

        // APIC body
        uint8_t encoding;
        if (mp3.read(&encoding, 1) != 1) break;
        fsize--; remain--;

        for (;;) {
            uint8_t b;
            if (mp3.read(&b, 1) != 1 || fsize == 0) break;
            fsize--; remain--;
            if (b == 0) break;
        }

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
        format_ = kJPEG;   // tentative, detectFormat_() will correct
        if (!detectFormat_()) {
            global_fs->remove(kCachePath);
            format_ = kNone;
            return false;
        }
    } else {
        global_fs->remove(kCachePath);
    }

    return found;
}

// File-format probing (JPEG vs PNG)
bool AlbumArt::detectFormat_() {
    if (!global_fs) return false;
    File f = global_fs->open(kCachePath, "r");
    if (!f) return false;

    uint8_t magic[4];
    int n = f.read(magic, 4);
    f.close();
    if (n < 4) return false;

    if (magic[0] == 0xFF && magic[1] == 0xD8) {
        format_ = kJPEG;
        return true;
    }
    if (magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
        format_ = kPNG;
        return true;
    }
    return false;   // unknown format
}

// Lifecycle
bool AlbumArt::load(const char* mp3Path) {
    if (format_ != kNone) clear();
    if (!ensureMuseDir_()) return false;
    return extractAPIC_(mp3Path);
}

void AlbumArt::clear() {
    if (format_ != kNone && global_fs) {
        global_fs->remove(kCachePath);
    }
    format_ = kNone;
    img_w_ = img_h_ = 0;
}

// Render to 1-bit e-ink buffer
bool AlbumArt::render1Bit(uint8_t* einkBuf, int bufW, int bufH,
                          int outX, int outY, int maxSize)
{
    if (format_ == kNone || !global_fs) return false;

    File file = global_fs->open(kCachePath, "r");
    if (!file) return false;

    if (format_ == kJPEG)
        return renderJPEG_(file, einkBuf, bufW, bufH, outX, outY, maxSize);
    if (format_ == kPNG)
        return renderPNG_(file, einkBuf, bufW, bufH, outX, outY, maxSize);

    file.close();
    return false;
}

// JPEG render path (delegates to JPEGDEC with ONE_BIT_DITHERED)
bool AlbumArt::renderJPEG_(File& file, uint8_t* einkBuf, int bufW, int bufH,
                           int outX, int outY, int maxSize)
{
    JPEGDEC jpeg;
    if (!jpeg.open(file, jpegDrawCb)) {
        file.close();
        return false;
    }

    int imgW = jpeg.getWidth();
    int imgH = jpeg.getHeight();
    if (imgW <= 0 || imgH <= 0) {
        jpeg.close(); file.close();
        return false;
    }

    int ss  = jpeg.getSubSample();
    int mcuCX, mcuCY;
    switch (ss) {
        case 0x12: mcuCX = 8;  mcuCY = 16; break;
        case 0x21: mcuCX = 16; mcuCY = 8;  break;
        case 0x22: mcuCX = 16; mcuCY = 16; break;
        default:   mcuCX = 8;  mcuCY = 8;  break;
    }

    float scale = (float)maxSize / (imgW > imgH ? imgW : imgH);
    int outW = (int)(imgW * scale);
    int outH = (int)(imgH * scale);
    if (outW < 1) outW = 1;
    if (outH < 1) outH = 1;

    img_w_ = outW;
    img_h_ = outH;

    int stripMCUs = 128 / mcuCX;
    if (stripMCUs < 1) stripMCUs = 1;

    int ditherW = ((imgW + mcuCX - 1) / mcuCX) * mcuCX;
    int ditherSize = ditherW * mcuCY;
    uint8_t* ditherBuf = new (std::nothrow) uint8_t[ditherSize];
    if (!ditherBuf) {
        jpeg.close(); file.close();
        return false;
    }

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

// PNG render path
bool AlbumArt::renderPNG_(File& file, uint8_t* einkBuf, int bufW, int bufH,
                          int outX, int outY, int maxSize)
{
    // PNGIMAGE embeds a 32KB zlib buffer so it must be heap-allocated to avoid
    // stack overflow on the ~4 KB appTask stack.
    auto* png = new (std::nothrow) PNG;
    if (!png) { file.close(); return false; }

    if (png->open(kCachePath, pngOpenCb, pngCloseCb, pngReadCb, pngSeekCb, pngDrawCb)
        != PNG_SUCCESS) {
        delete png; file.close();
        return false;
    }

    int imgW = png->getWidth();
    int imgH = png->getHeight();
    if (imgW <= 0 || imgH <= 0) {
        png->close(); delete png; file.close();
        return false;
    }

    float scale = (float)maxSize / (imgW > imgH ? imgW : imgH);
    int outW = (int)(imgW * scale);
    int outH = (int)(imgH * scale);
    if (outW < 1) outW = 1;
    if (outH < 1) outH = 1;

    img_w_ = outW;
    img_h_ = outH;

    // Allocate Floyd–Steinberg error buffers (full width + margin)
    int errSize = (imgW + 2) * sizeof(int16_t);
    auto* err0 = new (std::nothrow) int16_t[imgW + 2];
    auto* err1 = new (std::nothrow) int16_t[imgW + 2];
    if (!err0 || !err1) {
        delete[] err0; delete[] err1;
        png->close(); delete png; file.close();
        return false;
    }
    memset(err0, 0, errSize);
    memset(err1, 0, errSize);

    PngRenderCtx ctx;
    ctx.einkBuf   = einkBuf;
    ctx.bufW      = bufW;
    ctx.bufH      = bufH;
    ctx.outX      = outX;
    ctx.outY      = outY;
    ctx.fullW     = imgW;
    ctx.fullH     = imgH;
    ctx.outW      = outW;
    ctx.outH      = outH;
    ctx.pixelType = png->getPixelType();
    ctx.err       = err0;
    ctx.nextErr   = err1;

    int rc = png->decode(&ctx, PNG_FAST_PALETTE);
    png->close();
    delete png;
    file.close();

    delete[] err0;
    delete[] err1;

    return rc == PNG_SUCCESS;
}

#include "metadata.h"
#include <cstring>
#include <cstdlib>

static uint32_t readSyncsafeInt(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 21) |
           ((uint32_t)buf[1] << 14) |
           ((uint32_t)buf[2] <<  7) |
           (uint32_t)buf[3];
}

static String readTextFrame(const uint8_t* data, size_t size) {
    if (size < 2) return "";
    uint8_t enc = data[0];
    size_t len = size - 1;
    if (len == 0) return "";
    if (enc == 0x00 || enc == 0x03) {
        return String((const char*)data + 1, len);
    }
    return "";
}

bool parseID3v2(File& file, SongMetadata& meta) {
    uint8_t hdr[10];
    if (file.read(hdr, sizeof(hdr)) != sizeof(hdr)) {
        return false;
    }
    if (hdr[0] != 'I' || hdr[1] != 'D' || hdr[2] != '3') {
        file.seek(0, SeekSet);
        return false;
    }

    uint8_t  ver     = hdr[3];
    uint8_t  flags   = hdr[5];
    uint32_t tagSize = readSyncsafeInt(hdr + 6);

    if (ver < 3 || ver > 4) {
        file.seek(tagSize, SeekCur);
        return false;
    }

    if (flags & 0x40) {
        uint8_t ext[4];
        if (file.read(ext, 4) != 4) return false;
        uint32_t extSize = (ver == 4) ? readSyncsafeInt(ext)
            : ((uint32_t)ext[0] << 24 | (uint32_t)ext[1] << 16 |
               (uint32_t)ext[2] <<  8 | ext[3]);
        file.seek(extSize - 4, SeekCur);
    }

    bool footer = (ver == 4) && (flags & 0x10);
    size_t remain = tagSize - (footer ? 10 : 0);
    bool found = false;

    while (remain >= 10) {
        uint8_t fhdr[10];
        if (file.read(fhdr, 10) != 10) break;
        remain -= 10;

        char id[5] = {};
        memcpy(id, fhdr, 4);
        if (id[0] == '\0') break;

        uint32_t fsize = ((uint32_t)fhdr[4] << 24) |
                         ((uint32_t)fhdr[5] << 16) |
                         ((uint32_t)fhdr[6] <<  8) |
                         fhdr[7];
        if (fsize == 0 || fsize > remain) break;

        if (fsize > 4096) {
            file.seek(fsize, SeekCur);
            remain -= fsize;
            continue;
        }

        uint8_t fdata[4096];
        if (file.read(fdata, fsize) != fsize) break;
        remain -= fsize;

        if      (strcmp(id, "TIT2") == 0) { meta.title  = readTextFrame(fdata, fsize); found = true; }
        else if (strcmp(id, "TPE1") == 0) { meta.artist = readTextFrame(fdata, fsize); found = true; }
        else if (strcmp(id, "TALB") == 0) { meta.album  = readTextFrame(fdata, fsize); found = true; }
    }

    meta.valid = found;
    return found;
}

bool parseID3v1(File& file, SongMetadata& meta) {
    size_t sz = file.size();
    if (sz < 128) return false;

    file.seek(sz - 128, SeekSet);
    uint8_t tag[128];
    if (file.read(tag, 128) != 128) return false;

    if (tag[0] != 'T' || tag[1] != 'A' || tag[2] != 'G') return false;

    auto read30 = [&](int off) -> String {
        char buf[31];
        memcpy(buf, tag + off, 30);
        buf[30] = '\0';
        String s(buf);
        s.trim();
        return s;
    };

    meta.title  = read30(3);
    meta.artist = read30(33);
    meta.album  = read30(63);

    char ybuf[5];
    memcpy(ybuf, tag + 93, 4);
    ybuf[4] = '\0';
    meta.year = atoi(ybuf);

    meta.valid = true;
    return true;
}

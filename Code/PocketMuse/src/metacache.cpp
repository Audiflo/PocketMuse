#include "metacache.h"
#include <globals.h>
#include <cstring>

bool MetadataCache::readStr(File& f, char* buf, size_t bufSize) {
    if (!f || bufSize == 0) return false;
    buf[0] = '\0';
    uint16_t len;
    if (f.read((uint8_t*)&len, sizeof(len)) != sizeof(len)) return false;
    if (len == 0) return true;
    size_t toRead = len < bufSize - 1 ? len : bufSize - 1;
    if (f.read((uint8_t*)buf, toRead) != toRead) return false;
    buf[toRead] = '\0';
    if (toRead < len) f.seek(f.position() + (len - toRead));
    return true;
}

bool MetadataCache::skipStr(File& f) {
    uint16_t len;
    if (f.read((uint8_t*)&len, sizeof(len)) != sizeof(len)) return false;
    if (len > 0) f.seek(f.position() + len);
    return true;
}

bool MetadataCache::load(const char* path, char* title, size_t tSize,
                          char* artist, size_t aSize, char* album, size_t lSize) {
    if (!path || !path[0] || !global_fs) return false;

    File f = global_fs->open(kPath, "r");
    if (!f) return false;

    uint32_t magic, count;
    if (f.read((uint8_t*)&magic, sizeof(magic)) != sizeof(magic) || magic != kMagic) {
        f.close(); return false;
    }
    if (f.read((uint8_t*)&count, sizeof(count)) != sizeof(count)) {
        f.close(); return false;
    }

    for (uint32_t i = 0; i < count; i++) {
        uint16_t pathLen;
        if (f.read((uint8_t*)&pathLen, sizeof(pathLen)) != sizeof(pathLen)) break;

        uint32_t entryStart = f.position();

        // Check if this entry's path matches
        bool match = false;
        if (pathLen > 0) {
            size_t pathStrLen = strlen(path);
            if (pathLen == pathStrLen + 1) {
                char* p = new char[pathLen];
                if (f.read((uint8_t*)p, pathLen) == pathLen) {
                    match = (strcmp(p, path) == 0);
                }
                delete[] p;
            } else {
                f.seek(f.position() + pathLen);
            }
        }

        if (match) {
            readStr(f, title, tSize);
            readStr(f, artist, aSize);
            readStr(f, album, lSize);
            f.close();
            return true;
        }

        f.seek(entryStart + pathLen);
        skipStr(f); skipStr(f); skipStr(f);
    }

    f.close();
    return false;
}

void MetadataCache::save(const char* path, const char* title,
                          const char* artist, const char* album) {
    if (!path || !path[0] || !global_fs) return;

    global_fs->mkdir("/music/.muse");

    // Read existing entries into temp file, updating the matching one
    File old = global_fs->open(kPath, "r");
    File tmp = global_fs->open(kTmp, "w");
    if (!tmp) { if (old) old.close(); return; }

    uint32_t magic = kMagic;
    tmp.write((uint8_t*)&magic, sizeof(magic));

    if (old) {
        uint32_t oldMagic, count;
        if (old.read((uint8_t*)&oldMagic, sizeof(oldMagic)) == sizeof(oldMagic) &&
            oldMagic == kMagic &&
            old.read((uint8_t*)&count, sizeof(count)) == sizeof(count)) {

            // Write count placeholder, will update at end
            uint32_t countPos = tmp.position();
            tmp.write((uint8_t*)&count, sizeof(count));

            uint32_t newCount = 0;
            bool updated = false;
            size_t pathLen = strlen(path) + 1;

            for (uint32_t i = 0; i < count; i++) {
                uint16_t epLen;
                if (old.read((uint8_t*)&epLen, sizeof(epLen)) != sizeof(epLen)) break;

                uint32_t entryStart = old.position();
                char* epBuf = new char[epLen];
                if (old.read((uint8_t*)epBuf, epLen) != epLen) {
                    delete[] epBuf; break;
                }

                bool isMatch = !updated && (size_t)epLen == pathLen &&
                               strcmp(epBuf, path) == 0;

                if (isMatch) {
                    // Replace with new data
                    uint16_t len = pathLen;
                    tmp.write((uint8_t*)&len, sizeof(len));
                    tmp.write((uint8_t*)path, len);

                    auto writeStr = [&](const char* s) {
                        uint16_t sl = s ? strlen(s) + 1 : 0;
                        tmp.write((uint8_t*)&sl, sizeof(sl));
                        if (sl > 0) tmp.write((uint8_t*)s, sl);
                    };
                    writeStr(title);
                    writeStr(artist);
                    writeStr(album);
                    updated = true;
                    newCount++;
                } else {
                    // Copy entry unchanged
                    uint16_t len = epLen;
                    tmp.write((uint8_t*)&len, sizeof(len));
                    tmp.write((uint8_t*)epBuf, epLen);

                    // Copy title, artist, album
                    for (int j = 0; j < 3; j++) {
                        uint16_t sl;
                        if (old.read((uint8_t*)&sl, sizeof(sl)) != sizeof(sl)) {
                            delete[] epBuf; old.close(); tmp.close();
                            global_fs->remove(kTmp);
                            return;
                        }
                        tmp.write((uint8_t*)&sl, sizeof(sl));
                        if (sl > 0) {
                            uint8_t* sbuf = new uint8_t[sl];
                            old.read(sbuf, sl);
                            tmp.write(sbuf, sl);
                            delete[] sbuf;
                        }
                    }
                    newCount++;
                }
                delete[] epBuf;
            }

            // If not updated (new entry), append it
            if (!updated) {
                uint16_t len = pathLen;
                tmp.write((uint8_t*)&len, sizeof(len));
                tmp.write((uint8_t*)path, len);

                auto writeStr = [&](const char* s) {
                    uint16_t sl = s ? strlen(s) + 1 : 0;
                    tmp.write((uint8_t*)&sl, sizeof(sl));
                    if (sl > 0) tmp.write((uint8_t*)s, sl);
                };
                writeStr(title); writeStr(artist); writeStr(album);
                newCount++;
            }

            // Update count
            uint32_t endPos = tmp.position();
            tmp.seek(countPos);
            tmp.write((uint8_t*)&newCount, sizeof(newCount));
            tmp.seek(endPos);

            old.close();
        } else {
            old.close();
            // Corrupted file, start fresh
            tmp.seek(0);
            tmp.write((uint8_t*)&magic, sizeof(magic));
            goto writeNew;
        }
    } else {
        writeNew:;
        uint32_t one = 1;
        tmp.write((uint8_t*)&one, sizeof(one));
        uint16_t len = strlen(path) + 1;
        tmp.write((uint8_t*)&len, sizeof(len));
        tmp.write((uint8_t*)path, len);

        auto writeStr = [&](const char* s) {
            uint16_t sl = s ? strlen(s) + 1 : 0;
            tmp.write((uint8_t*)&sl, sizeof(sl));
            if (sl > 0) tmp.write((uint8_t*)s, sl);
        };
        writeStr(title); writeStr(artist); writeStr(album);
    }

    tmp.close();

    global_fs->remove(kPath);
    global_fs->rename(kTmp, kPath);
}

void MetadataCache::clear() {
    if (global_fs) global_fs->remove(kPath);
}

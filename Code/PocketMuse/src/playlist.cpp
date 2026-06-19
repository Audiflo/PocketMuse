#include "playlist.h"
#include <globals.h>
#include <cstring>

// helpers

// Read one line (including trailing \n) from a File. buf is always null-terminated.
// Returns false only on immediate EOF/error (no bytes read).
static bool readLine(File& f, char* buf, size_t bufSize) {
    if (!f || f.available() <= 0) return false;
    int i = 0;
    while (i < (int)bufSize - 1) {
        int c = f.read();
        if (c < 0) break;
        buf[i++] = (char)c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return i > 0;
}

// Trim trailing \r and \n in-place.
static void trimCRLF(char* buf) {
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\r' || buf[len - 1] == '\n'))
        buf[--len] = '\0';
}

// PlaylistCursor

bool PlaylistCursor::load(const char* m3uPath, bool isFav) {
    unload();
    if (!global_fs) return false;

    File f = global_fs->open(m3uPath, "r");
    if (!f) return false;

    bool ok = isFav ? parseFavs_(f) : parseM3U_(f);
    f.close();

    if (!ok || count_ == 0) { unload(); return false; }

    strncpy(path_, m3uPath, sizeof(path_) - 1);
    path_[sizeof(path_) - 1] = '\0';
    file_size_ = f.size();
    return true;
}

void PlaylistCursor::unload() {
    delete[] offsets_;
    offsets_ = nullptr;
    count_ = 0;
    file_size_ = 0;
}

bool PlaylistCursor::getEntry(int index, char* pathBuf, size_t bufSize) {
    if (!valid() || index < 0 || index >= count_ || !global_fs) return false;

    File f = global_fs->open(path_, "r");
    if (!f) return false;

    f.seek(offsets_[index]);
    bool ok = readLine(f, pathBuf, bufSize);
    f.close();

    if (ok) trimCRLF(pathBuf);
    return ok && strlen(pathBuf) > 0;
}

bool PlaylistCursor::readPage(int startIndex, int count, std::vector<String>& out) {
    out.clear();
    if (!valid() || startIndex < 0 || count <= 0 || !global_fs) return false;

    int endIndex = startIndex + count;
    if (endIndex > count_) endIndex = count_;

    File f = global_fs->open(path_, "r");
    if (!f) return false;

    for (int i = startIndex; i < endIndex; i++) {
        f.seek(offsets_[i]);
        char buf[256];
        if (readLine(f, buf, sizeof(buf))) {
            trimCRLF(buf);
            if (strlen(buf) > 0) out.emplace_back(buf);
        }
    }

    f.close();
    return !out.empty();
}

bool PlaylistCursor::parseM3U_(File& f) {
    int cap = 128;
    uint32_t* off = new uint32_t[cap];
    int n = 0;
    char buf[kScanBufSize];

    while (f.available()) {
        uint32_t lineStart = f.position();
        if (!readLine(f, buf, sizeof(buf))) break;

        trimCRLF(buf);
        if (strlen(buf) == 0) continue;

        // Skip comments (#EXTM3U, #EXTINF, #)
        if (buf[0] == '#') continue;

        if (n >= cap) {
            cap *= 2;
            uint32_t* nf = new uint32_t[cap];
            memcpy(nf, off, n * sizeof(uint32_t));
            delete[] off;
            off = nf;
        }
        off[n++] = lineStart;
    }

    if (n == 0) { delete[] off; return false; }
    count_ = n;
    offsets_ = off;
    return true;
}

bool PlaylistCursor::parseFavs_(File& f) {
    int cap = 128;
    uint32_t* off = new uint32_t[cap];
    int n = 0;
    char buf[kScanBufSize];

    while (f.available()) {
        uint32_t lineStart = f.position();
        if (!readLine(f, buf, sizeof(buf))) break;

        trimCRLF(buf);
        if (strlen(buf) == 0) continue;

        if (n >= cap) {
            cap *= 2;
            uint32_t* nf = new uint32_t[cap];
            memcpy(nf, off, n * sizeof(uint32_t));
            delete[] off;
            off = nf;
        }
        off[n++] = lineStart;
    }

    if (n == 0) { delete[] off; return false; }
    count_ = n;
    offsets_ = off;
    return true;
}

// PlaylistManager

PlaylistManager::PlaylistManager() : source_(PlaySource::Library), pl_index_(-1) {}

bool PlaylistManager::begin() {
    if (!global_fs) return false;
    global_fs->mkdir(kMuseDir);
    global_fs->mkdir(kPlDir);
    scanPlaylists_();
    return true;
}

const PlaylistMeta& PlaylistManager::playlistMeta(int idx) const {
    static PlaylistMeta empty;
    if (idx < 0 || idx >= (int)playlists_.size()) return empty;
    return playlists_[idx];
}

int PlaylistManager::findPlaylist(const char* name) const {
    for (int i = 0; i < (int)playlists_.size(); i++) {
        if (playlists_[i].name == name) return i;
    }
    return -1;
}

bool PlaylistManager::createPlaylist(const char* name) {
    if (!global_fs || findPlaylist(name) >= 0) return false;

    String fpath = String(kPlDir) + "/" + name + ".m3u";
    File f = global_fs->open(fpath, "w");
    if (!f) return false;
    f.println("#EXTM3U");
    f.close();

    scanPlaylists_();
    return true;
}

bool PlaylistManager::deletePlaylist(int idx) {
    if (!global_fs || idx < 0 || idx >= (int)playlists_.size()) return false;

    if (source_ == PlaySource::Playlist && pl_index_ == idx) {
        cursor_.unload();
        source_ = PlaySource::Library;
        pl_index_ = -1;
    }

    bool ok = global_fs->remove(playlists_[idx].filename.c_str());
    if (ok) scanPlaylists_();
    return ok;
}

bool PlaylistManager::playLibrary() {
    cursor_.unload();
    source_ = PlaySource::Library;
    pl_index_ = -1;
    return true;
}

bool PlaylistManager::playFavorites() {
    cursor_.unload();
    if (!cursor_.load(kFavFile, true)) return false;
    source_ = PlaySource::Favorites;
    pl_index_ = -1;
    return true;
}

bool PlaylistManager::playPlaylist(int idx) {
    if (idx < 0 || idx >= (int)playlists_.size()) return false;

    cursor_.unload();
    if (!cursor_.load(playlists_[idx].filename.c_str(), false)) {
        playlists_.erase(playlists_.begin() + idx);
        return false;
    }

    source_ = PlaySource::Playlist;
    playlists_[idx].count = cursor_.count();
    pl_index_ = idx;
    return true;
}

int PlaylistManager::entryCount() const {
    return cursor_.count();
}

bool PlaylistManager::readPage(int start, int count, std::vector<String>& out) {
    return cursor_.readPage(start, count, out);
}

bool PlaylistManager::getEntry(int index, char* pathBuf, size_t bufSize) {
    return cursor_.getEntry(index, pathBuf, bufSize);
}

// Favorites

bool PlaylistManager::toggleFavorite(const char* path) {
    if (!global_fs) return false;

    bool wasFav = isFavorite(path);
    if (wasFav) {
        if (!removeLine_(kFavFile, path)) return true;   // unchanged, still fav
    } else {
        File f = global_fs->open(kFavFile, "a");
        if (!f) return false;  // unchanged, still not fav
        f.println(path);
        f.close();
    }

    if (source_ == PlaySource::Favorites) {
        cursor_.unload();
        cursor_.load(kFavFile, true);
    }
    return !wasFav;
}

bool PlaylistManager::isFavorite(const char* path) {
    if (!global_fs) return false;
    File f = global_fs->open(kFavFile, "r");
    if (!f) return false;

    char buf[256];
    while (f.available()) {
        int i = 0;
        while (i < 255) {
            int c = f.read();
            if (c < 0 || c == '\n') break;
            buf[i++] = (char)c;
        }
        buf[i] = '\0';
        if (strcmp(buf, path) == 0) { f.close(); return true; }
    }

    f.close();
    return false;
}

int PlaylistManager::favoriteCount() {
    if (!global_fs) return 0;
    File f = global_fs->open(kFavFile, "r");
    if (!f) return 0;

    int count = 0;
    char buf[256];
    while (f.available()) {
        int i = 0;
        while (i < 255) {
            int c = f.read();
            if (c < 0 || c == '\n') break;
            buf[i++] = (char)c;
        }
        buf[i] = '\0';
        if (i > 0) count++;
    }

    f.close();
    return count;
}

// Playlist content management

bool PlaylistManager::addToPlaylist(int plIdx, const char* songPath) {
    if (!global_fs || plIdx < 0 || plIdx >= (int)playlists_.size()) return false;

    File f = global_fs->open(playlists_[plIdx].filename.c_str(), "a");
    if (!f) return false;
    f.println(songPath);
    f.close();

    if (source_ == PlaySource::Playlist && pl_index_ == plIdx) {
        cursor_.unload();
        cursor_.load(playlists_[plIdx].filename.c_str(), false);
        playlists_[plIdx].count = cursor_.count();
    } else {
        playlists_[plIdx].count = -1;
    }

    return true;
}

bool PlaylistManager::removeFromPlaylist(int plIdx, int entryIndex) {
    if (!global_fs || plIdx < 0 || plIdx >= (int)playlists_.size()) return false;

    PlaylistCursor tmp;
    if (!tmp.load(playlists_[plIdx].filename.c_str(), false)) return false;
    if (entryIndex < 0 || entryIndex >= tmp.count()) { tmp.unload(); return false; }

    const char* tmpPath = "/music/.muse/.pl_tmp";
    File out = global_fs->open(tmpPath, "w");
    if (!out) { tmp.unload(); return false; }

    out.println("#EXTM3U");
    char p[256];
    for (int i = 0; i < tmp.count(); i++) {
        if (i == entryIndex) continue;
        if (tmp.getEntry(i, p, sizeof(p))) out.println(p);
    }

    tmp.unload();
    out.close();

    global_fs->remove(playlists_[plIdx].filename.c_str());
    global_fs->rename(tmpPath, playlists_[plIdx].filename.c_str());

    if (source_ == PlaySource::Playlist && pl_index_ == plIdx) {
        cursor_.unload();
        cursor_.load(playlists_[plIdx].filename.c_str(), false);
        playlists_[plIdx].count = cursor_.count();
    } else {
        playlists_[plIdx].count = -1;
    }

    return true;
}

// Private helpers

void PlaylistManager::scanPlaylists_() {
    playlists_.clear();
    if (!global_fs) return;

    File dir = global_fs->open(kPlDir);
    if (!dir || !dir.isDirectory()) return;

    File entry;
    while ((entry = dir.openNextFile())) {
        if (entry.isDirectory()) { entry.close(); continue; }

        String fname = String(entry.name());
        if (!fname.endsWith(".m3u")) { entry.close(); continue; }

        // Strip .m3u and directory prefix for display name
        String dname = fname.substring(0, fname.length() - 4);
        int slash = dname.lastIndexOf('/');
        if (slash >= 0) dname = dname.substring(slash + 1);

        PlaylistMeta meta;
        meta.filename = fname;
        meta.name     = dname;
        meta.count    = -1;
        playlists_.push_back(meta);
        entry.close();
    }

    dir.close();
}

bool PlaylistManager::removeLine_(const char* filePath, const char* line) {
    if (!global_fs) return false;

    File f = global_fs->open(filePath, "r");
    if (!f) return false;

    const char* tmpPath = "/music/.muse/.fav_tmp";
    File out = global_fs->open(tmpPath, "w");
    if (!out) { f.close(); return false; }

    char buf[256];
    bool found = false;
    while (f.available()) {
        int i = 0;
        while (i < 255) {
            int c = f.read();
            if (c < 0 || c == '\n') break;
            buf[i++] = (char)c;
        }
        buf[i] = '\0';

        if (!found && strcmp(buf, line) == 0) {
            found = true;
            continue;
        }
        out.println(buf);
    }

    f.close();
    out.close();

    global_fs->remove(filePath);
    global_fs->rename(tmpPath, filePath);

    if (source_ == PlaySource::Favorites) {
        cursor_.unload();
        cursor_.load(kFavFile, true);
    }

    return found;
}

#pragma once
#include <Arduino.h>
#include <FS.h>
#include <stdint.h>
#include <stddef.h>
#include <vector>

// Plays directory
static constexpr const char* kMuseDir     = "/music/.muse";
static constexpr const char* kPlDir       = "/music/.muse/playlists";
static constexpr const char* kFavFile     = "/music/.muse/favs.txt";

enum class PlaySource : uint8_t {
    Library,
    Favorites,
    Playlist,
};

struct PlaylistMeta {
    String filename;   // full path to .m3u file
    String name;       // display name (filename without .m3u)
    int    count;      // total entries (-1 if not loaded)
};

// Lightweight cursor into a playlist for streaming access.
// Builds a small offset table in RAM (~4 bytes/entry) so paging
// is a single seek + read per page, not a full re-scan.
class PlaylistCursor {
public:
    PlaylistCursor() : file_size_(0), count_(0), offsets_(nullptr) {}

    bool load(const char* m3uPath, bool isFav);
    void unload();

    int  count() const { return count_; }
    bool valid() const { return offsets_ != nullptr; }

    // Read entry at index into pathBuf. Returns true if found.
    bool getEntry(int index, char* pathBuf, size_t bufSize);

    // Read a page of counts into the given vector. Clears vec first.
    bool readPage(int startIndex, int count, std::vector<String>& out);

private:
    bool parseM3U_(File& f);
    bool parseFavs_(File& f);
    bool readLine_(File& f, char* buf, size_t bufSize);

    uint32_t   file_size_;
    int        count_;
    char       path_[256];
    uint32_t*  offsets_; // heap-allocated byte offsets for each entry

    static constexpr int kScanBufSize = 512;
};

class PlaylistManager {
public:
    PlaylistManager();

    // Scan the playlists directory. Must be called at least once.
    bool begin();

    // Playlist listing
    int  playlistCount() const { return (int)playlists_.size(); }
    const PlaylistMeta& playlistMeta(int idx) const;
    int  findPlaylist(const char* name) const;

    // Playlist CRUD
    bool createPlaylist(const char* name);
    bool deletePlaylist(int idx);

    // Current source
    PlaySource source() const { return source_; }
    const PlaylistCursor& cursor() const { return cursor_; }
    int  playlistIndex() const { return pl_index_; }

    // Source switching
    bool playLibrary();           // switch to full library as source
    bool playFavorites();         // switch to favorites
    bool playPlaylist(int idx);   // switch to a named playlist

    // Active source helpers
    int  entryCount() const;
    bool readPage(int start, int count, std::vector<String>& out);
    bool getEntry(int index, char* pathBuf, size_t bufSize);

    // Favorites
    bool toggleFavorite(const char* path);
    bool isFavorite(const char* path);
    int  favoriteCount();

    // Playlist content management
    bool addToPlaylist(int plIdx, const char* songPath);
    bool removeFromPlaylist(int plIdx, int entryIndex);

private:
    void scanPlaylists_();
    bool removeLine_(const char* filePath, const char* line);

    PlaySource      source_;
    PlaylistCursor  cursor_;
    int             pl_index_;
    std::vector<PlaylistMeta> playlists_;

    static constexpr int kMaxFavs = 256;
};

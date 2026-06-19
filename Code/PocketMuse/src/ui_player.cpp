#include <globals.h>
#include "muse.h"
#include "metadata.h"
#include <cstring>

const char* get_track_path(int index) {
    static char buf[256];
    buf[0] = '\0';

    switch (g_playlistMgr.source()) {
    case PlaySource::Library:
        if (index >= 0 && index < g_library.count()) {
            const String& p = g_library.path(index);
            strncpy(buf, p.c_str(), sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
        }
        break;
    case PlaySource::Favorites:
    case PlaySource::Playlist:
        g_playlistMgr.getEntry(index, buf, sizeof(buf));
        break;
    }
    return buf;
}

void player_play_index(int index) {
    const char* path = get_track_path(index);
    if (!path || !path[0]) return;

    player.stop();
    player.play(path);

    g_nowTitle[0] = '\0';
    g_nowArtist[0] = '\0';
    g_nowAlbum[0] = '\0';
    g_nowDuration = 0;

    if (global_fs) {
        File f = global_fs->open(path, "r");
        if (f) {
            SongMetadata meta;
            if (parseID3v2(f, meta) || parseID3v1(f, meta)) {
                strncpy(g_nowTitle, meta.title.c_str(), sizeof(g_nowTitle) - 1);
                strncpy(g_nowArtist, meta.artist.c_str(), sizeof(g_nowArtist) - 1);
                strncpy(g_nowAlbum, meta.album.c_str(), sizeof(g_nowAlbum) - 1);
            }
            f.close();
        }
    }

    if (g_nowTitle[0] == '\0') {
        get_display_name(path, g_nowTitle, sizeof(g_nowTitle));
    }

    strncpy(g_nowPath, path, sizeof(g_nowPath) - 1);
    g_nowPath[sizeof(g_nowPath) - 1] = '\0';

    g_nowTrackIndex = index;
    g_nowDuration = player.durationSec();
    g_nowProgress = 0.0f;
    g_isFavorite = g_playlistMgr.isFavorite(path);
    g_playState = PlayerState::Playing;
    g_loopMode = player.loopMode();
    g_shuffleEnabled = player.isShuffle();
    g_needsRedraw = true;
    g_appMode = MODE_NOWPLAYING;
}

void player_next_track() {
    int n = g_trackCount > 0 ? g_trackCount : 1;
    int next = g_nowTrackIndex + 1;

    if (g_shuffleEnabled) {
        next = rand() % n;
    } else if (g_loopMode == LoopMode::All && next >= n) {
        next = 0;
    }

    if (next >= n) {
        player.stop();
        g_playState = PlayerState::Stopped;
        g_nowProgress = 0.0f;
        g_needsRedraw = true;
        return;
    }

    player_play_index(next);
}

void player_prev_track() {
    int n = g_trackCount > 0 ? g_trackCount : 1;
    int prev = g_nowTrackIndex - 1;

    if (prev < 0) {
        if (g_loopMode == LoopMode::All) {
            prev = n - 1;
        } else {
            prev = 0;
        }
    }

    if (prev >= 0 && prev < n) {
        player_play_index(prev);
    }
}

void player_cycle_source() {
    PlaySource cur = g_playlistMgr.source();

    switch (cur) {
    case PlaySource::Library:
        if (g_playlistMgr.playFavorites()) {
            return;
        }
        g_playlistMgr.playLibrary();
        break;

    case PlaySource::Favorites: {
        int plCount = g_playlistMgr.playlistCount();
        if (plCount > 0) {
            g_playlistMgr.playPlaylist(0);
        } else {
            g_playlistMgr.playLibrary();
        }
        break;
    }
    case PlaySource::Playlist: {
        int next = g_playlistMgr.playlistIndex() + 1;
        if (next < g_playlistMgr.playlistCount()) {
            g_playlistMgr.playPlaylist(next);
        } else {
            g_playlistMgr.playLibrary();
        }
        break;
    }
    }

    g_trackCount = g_playlistMgr.entryCount();
    if (g_playlistMgr.source() == PlaySource::Library) {
        g_trackCount = g_library.count();
    }
}

void player_toggle_favorite() {
    if (g_nowPath[0]) {
        g_playlistMgr.toggleFavorite(g_nowPath);
        g_isFavorite = g_playlistMgr.isFavorite(g_nowPath);
    }
}

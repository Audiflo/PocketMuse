#include <globals.h>
#include "muse.h"
#include "albumart.h"
#include "metacache.h"
#include "AudioTools/Disk/WAVFileInfo.h"
#include <cstring>
#include <vector>
#include <algorithm>

using namespace audio_tools;

// Fisher-Yates shuffle state
static std::vector<int> s_shuffleIdx;
static int s_shufflePos = 0;

static void build_shuffle() {
    int n = g_trackCount;
    if (n <= 0) { s_shuffleIdx.clear(); return; }
    s_shuffleIdx.resize(n);
    for (int i = 0; i < n; i++) s_shuffleIdx[i] = i;
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(s_shuffleIdx[i], s_shuffleIdx[j]);
    }
    s_shufflePos = 0;
}

bool get_track_path(int index, char* buf, size_t bufSize) {
    if (!buf || bufSize == 0) return false;
    buf[0] = '\0';

    switch (g_playlistMgr.source()) {
    case PlaySource::Library:
        if (index >= 0 && index < g_library.count()) {
            const String& p = g_library.path(index);
            strncpy(buf, p.c_str(), bufSize - 1);
            buf[bufSize - 1] = '\0';
        }
        break;
    case PlaySource::Favorites:
    case PlaySource::Playlist:
        g_playlistMgr.getEntry(index, buf, bufSize);
        break;
    }
    return buf[0] != '\0';
}

// Compute approximate duration from file size + format header.
uint32_t compute_duration(const char* path) {
    if (!path || !path[0] || !global_fs) return 0;

    File f = global_fs->open(path, "r");
    if (!f) return 0;

    size_t fileSize = f.size();
    if (fileSize < 128) { f.close(); return 0; }

    uint32_t duration = 0;

    // Check for WAV using WAVFileInfo
    WAVFileInfo wav;
    WAVAudioInfo info;
    if (wav.getInfo(f, info) && info.data_length > 0) {
        uint32_t bytesPerSec = info.sample_rate * info.channels * (info.bits_per_sample / 8);
        if (bytesPerSec > 0) {
            duration = info.data_length / bytesPerSec;
        }
    }

    // If not WAV, try MPEG frame header for bitrate
    if (duration == 0) {
        f.seek(0);
        uint8_t buf[4];
        size_t pos = 0;
        while (pos < fileSize - 1) {
            int b = f.read();
            if (b < 0) break;
            pos++;
            if (b != 0xFF) continue;

            int b2 = f.read();
            if (b2 < 0) break;
            pos++;
            if ((b2 & 0xE0) != 0xE0) continue;

            buf[0] = 0xFF;
            buf[1] = b2;
            if (f.read(buf + 2, 2) != 2) break;
            pos += 2;

            int version = (buf[1] >> 3) & 3;
            int layer = (buf[1] >> 1) & 3;
            int bitrateIdx = buf[2] >> 4;

            if (version == 1 || layer == 0 || bitrateIdx == 0 || bitrateIdx == 15) {
                continue;
            }

            static const int bitrate_mp3[15] = {
                0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320
            };

            int bitrate = 0;
            if (version == 3 && layer == 1) {
                bitrate = bitrate_mp3[bitrateIdx];
            } else if (version == 3 && layer == 2) {
                static const int table_l2[15] = {
                    0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384
                };
                bitrate = table_l2[bitrateIdx];
            } else if (version == 3 && layer == 3) {
                static const int table_l1[15] = {
                    0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448
                };
                bitrate = table_l1[bitrateIdx];
            } else if (version == 2 || version == 0) {
                static const int table_v2_l3[15] = {
                    0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160
                };
                bitrate = table_v2_l3[bitrateIdx];
            }

            if (bitrate > 0) {
                uint64_t bits = (uint64_t)fileSize * 8;
                duration = bits / ((uint64_t)bitrate * 1000);
            }
            break;
        }
    }

    f.close();
    return duration;
}

void player_play_index(int index) {
    char path[256];
    if (!get_track_path(index, path, sizeof(path)) || !path[0]) return;

    music_stop();

    g_nowTitle[0] = '\0';
    g_nowArtist[0] = '\0';
    g_nowAlbum[0] = '\0';
    g_nowDuration = 0;

    get_display_name(path, g_nowTitle, sizeof(g_nowTitle));

    strncpy(g_nowPath, path, sizeof(g_nowPath) - 1);
    g_nowPath[sizeof(g_nowPath) - 1] = '\0';

    g_nowTrackIndex = index;
    g_nowDuration = compute_duration(path);
    g_nowProgress = 0.0f;
    g_isFavorite = g_playlistMgr.isFavorite(path);
    g_playState = PlayerState::Playing;
    nowplaying_cache_art(path);

    music_play(path);

    g_needsRedraw = true;
    g_appMode = MODE_NOWPLAYING;
}

void player_next_track() {
    int n = g_trackCount > 0 ? g_trackCount : 1;
    int next;

    if (g_shuffleEnabled) {
        if (s_shuffleIdx.empty() || (int)s_shuffleIdx.size() != n) build_shuffle();
        s_shufflePos++;
        if (s_shufflePos >= n) {
            build_shuffle();
            s_shufflePos = 0;
        }
        next = s_shuffleIdx[s_shufflePos];
    } else {
        next = g_nowTrackIndex + 1;
        if (g_loopMode == LoopMode::All && next >= n) next = 0;
    }

    if (next >= 0 && next < n) {
        player_play_index(next);
    } else {
        music_stop();
        g_playState = PlayerState::Stopped;
        g_nowProgress = 0.0f;
        g_needsRedraw = true;
    }
}

void player_prev_track() {
    int n = g_trackCount > 0 ? g_trackCount : 1;
    int prev;

    if (g_shuffleEnabled) {
        if (s_shuffleIdx.empty() || (int)s_shuffleIdx.size() != n) build_shuffle();
        s_shufflePos--;
        if (s_shufflePos < 0) s_shufflePos = n - 1;
        prev = s_shuffleIdx[s_shufflePos];
    } else {
        prev = g_nowTrackIndex - 1;
        if (prev < 0) {
            prev = (g_loopMode == LoopMode::All) ? n - 1 : 0;
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
        g_isFavorite = g_playlistMgr.toggleFavorite(g_nowPath);
    }
}

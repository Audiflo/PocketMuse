#include <globals.h>
#include "muse.h"

static constexpr int kItemH     = 10;
static constexpr int kListTop   = 32;
static constexpr int kListBot   = 222;
static constexpr int kMaxVis    = (kListBot - kListTop) / kItemH;

static int g_scrollOffset = 0;

static const char* source_label() {
    switch (g_playlistMgr.source()) {
        case PlaySource::Library:   return "Library";
        case PlaySource::Favorites: return "Favorites";
        case PlaySource::Playlist: {
            int pi = g_playlistMgr.playlistIndex();
            if (pi >= 0) return g_playlistMgr.playlistMeta(pi).name.c_str();
            return "Playlist";
        }
    }
    return "Library";
}

static int track_count() {
    switch (g_playlistMgr.source()) {
        case PlaySource::Library:   return g_library.count();
        case PlaySource::Favorites:
        case PlaySource::Playlist:  return g_playlistMgr.entryCount();
    }
    return 0;
}

void browser_init() {
    g_appMode = MODE_BROWSER;
    g_selIndex = 0;
    g_scrollOffset = 0;
    g_trackCount = track_count();
    g_needsRedraw = true;
}

void browser_process_key(char ch) {
    int n = track_count();
    int page = kMaxVis;

    switch (ch) {
    case 19: // LEFT - prev track
        if (g_selIndex > 0) {
            g_selIndex--;
            if (g_selIndex < g_scrollOffset) g_scrollOffset = g_selIndex;
            g_needsRedraw = true;
        }
        break;
    case 21: // RIGHT - next track
        if (g_selIndex < n - 1) {
            g_selIndex++;
            if (g_selIndex >= g_scrollOffset + kMaxVis) g_scrollOffset = g_selIndex - kMaxVis + 1;
            g_needsRedraw = true;
        }
        break;
    case 32: case 13: // Space or Enter
        if (n > 0 && g_selIndex < n) {
            player_play_index(g_selIndex);
        }
        break;
    case 'p': case 'P':
        player_cycle_source();
        g_selIndex = 0;
        g_scrollOffset = 0;
        g_trackCount = track_count();
        g_needsRedraw = true;
        break;
    case 'f': case 'F':
        if (n > 0 && g_selIndex < n) {
            char path[256];
            if (get_track_path(g_selIndex, path, sizeof(path)) && path[0]) {
                g_isFavorite = g_playlistMgr.toggleFavorite(path);
            }
        }
        g_needsRedraw = true;
        break;
    case 's': case 'S':
        g_shuffleEnabled = !g_shuffleEnabled;
        g_needsRedraw = true;
        break;
    case 'l': case 'L': {
        switch (g_loopMode) {
            case LoopMode::None: g_loopMode = LoopMode::One; break;
            case LoopMode::One:  g_loopMode = LoopMode::All; break;
            case LoopMode::All:  g_loopMode = LoopMode::None; break;
        }
        g_needsRedraw = true;
        break;
    }
    case 'd': case 'D':
        if (g_playlistMgr.source() == PlaySource::Playlist && n > 0) {
            int pi = g_playlistMgr.playlistIndex();
            int idx = g_selIndex;
            if (idx < g_trackCount) {
                g_playlistMgr.removeFromPlaylist(pi, idx);
                g_trackCount = track_count();
                if (g_selIndex >= g_trackCount && g_selIndex > 0) g_selIndex--;
                g_needsRedraw = true;
            }
        }
        break;
    case 'b': case 'B':
        if (g_playState != PlayerState::Stopped) {
            g_appMode = MODE_NOWPLAYING;
            g_needsRedraw = true;
        }
        break;
    case '?':
        g_prevMode = g_appMode;
        g_appMode = MODE_HELP;
        g_needsRedraw = true;
        break;
    }
}

void browser_render() {
    display.fillScreen(GxEPD_WHITE);

    char hdr[64];
    snprintf(hdr, sizeof(hdr), "%s  [%d]", source_label(), g_trackCount);
    draw_header(hdr);

    int n = track_count();
    if (n == 0) {
        display.setFont(&Font5x7Fixed);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(8, 80);
        display.print("No tracks found");
        display.setCursor(8, 94);
        display.print("Place .mp3 files in /music/ on SD");
        EINK().refresh();
        return;
    }

    if (g_scrollOffset > n - kMaxVis) g_scrollOffset = n - kMaxVis;
    if (g_scrollOffset < 0) g_scrollOffset = 0;

    int y = kListTop;
    int end = g_scrollOffset + kMaxVis;
    if (end > n) end = n;

    for (int i = g_scrollOffset; i < end; i++) {
        if (i == g_selIndex) {
            display.fillRect(0, y - 7, display.width(), kItemH, GxEPD_BLACK);
            display.setTextColor(GxEPD_WHITE);
        } else {
            display.setTextColor(GxEPD_BLACK);
        }

        display.setFont(&Font5x7Fixed);
        display.setCursor(8, y);

        char path[256];
        char label[64];
        get_track_path(i, path, sizeof(path));
        get_display_name(path, label, sizeof(label));

        uint16_t maxChars = 54;
        if ((int)strlen(label) > maxChars) {
            label[maxChars - 2] = '.';
            label[maxChars - 1] = '.';
            label[maxChars] = '\0';
        }
        display.print(label);

        y += kItemH;
    }

    draw_scrollbar(n, kMaxVis, g_scrollOffset);

    char footer[64] = {};
    if (g_playlistMgr.source() == PlaySource::Playlist) {
        snprintf(footer, sizeof(footer),
            "L/R:nav SPC:play P:src F:fav D:del ?:help");
    } else {
        snprintf(footer, sizeof(footer),
            "L/R:nav SPC:play P:src F:fav L:loop ?:help");
    }
    draw_footer(footer);

    EINK().refresh();
}

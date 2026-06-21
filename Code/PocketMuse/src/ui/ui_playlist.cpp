#include <globals.h>
#include "muse.h"

static constexpr int kItemH   = 10;
static constexpr int kListTop = 32;
static constexpr int kListBot = 222;
static constexpr int kMaxVis  = (kListBot - kListTop) / kItemH;

static int g_plScroll = 0;

void playlist_process_key(char ch) {
    int n = g_trackCount;

    switch (ch) {
    case 19: // LEFT
        if (g_selIndex > 0) {
            g_selIndex--;
            if (g_selIndex < g_plScroll) g_plScroll = g_selIndex;
            g_needsRedraw = true;
        }
        break;
    case 21: // RIGHT
        if (g_selIndex < n - 1) {
            g_selIndex++;
            if (g_selIndex >= g_plScroll + kMaxVis) g_plScroll = g_selIndex - kMaxVis + 1;
            g_needsRedraw = true;
        }
        break;
    case 32: case 13: // Space or Enter
        if (n > 0 && g_selIndex < n) {
            player_play_index(g_selIndex);
        }
        break;
    case 'd': case 'D':
        if (g_playlistMgr.source() == PlaySource::Playlist) {
            int pi = g_playlistMgr.playlistIndex();
            int idx = g_selIndex;
            if (idx < g_trackCount) {
                g_playlistMgr.removeFromPlaylist(pi, idx);
                g_trackCount = g_playlistMgr.entryCount();
                if (g_selIndex >= g_trackCount && g_selIndex > 0) g_selIndex--;
                g_needsRedraw = true;
            }
        }
        break;
    case 'b': case 'B':
        if (g_playState != PlayerState::Stopped) {
            g_appMode = MODE_NOWPLAYING;
        } else {
            g_appMode = MODE_BROWSER;
        }
        g_needsRedraw = true;
        break;
    case '?':
        g_prevMode = g_appMode;
        g_appMode = MODE_HELP;
        g_needsRedraw = true;
        break;
    }
}

void playlist_render() {
    display.fillScreen(GxEPD_WHITE);

    const char* srcName = "";
    if (g_playlistMgr.source() == PlaySource::Playlist) {
        int pi = g_playlistMgr.playlistIndex();
        if (pi >= 0) srcName = g_playlistMgr.playlistMeta(pi).name.c_str();
    }

    char hdr[64];
    snprintf(hdr, sizeof(hdr), "Playlist: %s  [%d]", srcName, g_trackCount);
    draw_header(hdr);

    int n = g_trackCount;
    if (n == 0) {
        display.setFont(&Font5x7Fixed);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(8, 80);
        display.print("Playlist is empty");
        draw_footer("B:back ?:help");
        EINK().refresh();
        return;
    }

    if (g_plScroll > n - kMaxVis) g_plScroll = n - kMaxVis;
    if (g_plScroll < 0) g_plScroll = 0;

    int y = kListTop;
    int end = g_plScroll + kMaxVis;
    if (end > n) end = n;

    for (int i = g_plScroll; i < end; i++) {
        if (i == g_selIndex) {
            display.fillRect(0, y - 7, display.width(), kItemH, GxEPD_BLACK);
            display.setTextColor(GxEPD_WHITE);
        } else if (i == g_nowTrackIndex) {
            display.fillRect(0, y - 7, display.width(), kItemH, GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);
            display.drawFastHLine(0, y - 7, display.width(), GxEPD_BLACK);
            display.drawFastHLine(0, y + kItemH - 8, display.width(), GxEPD_BLACK);
        } else {
            display.setTextColor(GxEPD_BLACK);
        }

        display.setFont(&Font5x7Fixed);
        display.setCursor(8, y);

        // Track number
        char line[72];
        snprintf(line, sizeof(line), "%2d. ", i + 1);
        display.print(line);

        // Track name
        char path[256];
        char label[56];
        get_track_path(i, path, sizeof(path));
        get_display_name(path, label, sizeof(label));

        if ((int)strlen(label) > 46) {
            label[44] = '.';
            label[45] = '.';
            label[46] = '\0';
        }
        display.print(label);

        // Now playing indicator
        if (i == g_nowTrackIndex && g_playState != PlayerState::Stopped) {
            display.setCursor(display.width() - 30, y);
            display.print(">");
        }

        y += kItemH;
    }

    draw_scrollbar(n, kMaxVis, g_plScroll);

    char footer[64];
    if (g_playlistMgr.source() == PlaySource::Playlist) {
        snprintf(footer, sizeof(footer),
            "L/R:nav SPC:play D:del B:back ?:help");
    } else {
        snprintf(footer, sizeof(footer),
            "L/R:nav SPC:play B:back ?:help");
    }
    draw_footer(footer);

    EINK().refresh();
}

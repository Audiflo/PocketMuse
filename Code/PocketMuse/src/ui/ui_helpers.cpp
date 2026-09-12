#include <globals.h>
#include "muse.h"

static constexpr int kLineH = 10;

void draw_header(const char* title) {
    u8g2f.setForegroundColor(GxEPD_BLACK);
    FontEngine::drawText(DisplayTarget::EINK, 8, 16,
                         title ? title : "", FontStyle::Body);
    display.drawFastHLine(0, 22, display.width(), GxEPD_BLACK);
}

void draw_footer(const char* hints) {
    EINK().drawStatusBar(hints ? hints : "");
}

void draw_scrollbar(int total, int visible, int position) {
    if (total <= visible) return;
    drawScrollbar(total, visible, position, -1, 0, kEinkContentH, 3,
                  false, GxEPD_BLACK, GxEPD_WHITE);
}

void format_time(char* buf, size_t bufSize, uint32_t seconds) {
    uint32_t m = seconds / 60;
    uint32_t s = seconds % 60;
    snprintf(buf, bufSize, "%d:%02d", m, s);
}

void get_display_name(const char* path, char* out, size_t maxLen) {
    const char* name = strrchr(path, '/');
    if (name) name++; else name = path;
    strncpy(out, name, maxLen - 1);
    out[maxLen - 1] = '\0';
    char* dot = strrchr(out, '.');
    if (dot) {
        char* p = dot;
        while (*p) { *p = '\0'; p++; }
    }
}

void ui_update_oled() {
    u8g2.clearBuffer();

    // Top line: play state + now playing
    String now = "   ";
    if (g_playState == PlayerState::Playing) {
        now = ">> ";
        now += g_nowTitle;
        if (g_nowArtist[0]) {
            now += " - ";
            now += g_nowArtist;
        }
    } else if (g_playState == PlayerState::Paused) {
        now = "|| ";
        now += g_nowTitle;
    } else if (g_nowTitle[0]) {
        now += g_nowTitle;
    } else {
        now += "PocketMuse";
    }
    FontEngine::drawText(DisplayTarget::OLED, 0, 8, now, FontStyle::Tiny);

    uint32_t elapsed = (uint32_t)(g_nowProgress * g_nowDuration);
    char timeStr[32];
    char elStr[16], durStr[16];
    format_time(elStr, sizeof(elStr), elapsed);
    format_time(durStr, sizeof(durStr), g_nowDuration);
    snprintf(timeStr, sizeof(timeStr), "%s / %s", elStr, durStr);

    // Progress bar outline
    u8g2.drawFrame(0, 14, 144, 7);
    if (g_nowProgress > 0.0f) {
        int filled = (int)(142 * g_nowProgress);
        if (filled > 0) u8g2.drawBox(1, 15, filled, 5);
    }

    // Time on right side
    FontEngine::drawText(DisplayTarget::OLED, 152, 20, timeStr, FontStyle::Tiny);

    // Bottom line: source + count
    String src;
    switch (g_playlistMgr.source()) {
        case PlaySource::Library:   src = "Library"; break;
        case PlaySource::Favorites: src = "Favorites"; break;
        case PlaySource::Playlist:  src = "Playlist"; break;
    }
    char srcBuf[32];
    snprintf(srcBuf, sizeof(srcBuf), "%s [%d]", src.c_str(), g_trackCount);
    FontEngine::drawText(DisplayTarget::OLED, 0, 30, srcBuf, FontStyle::Tiny);

    // Volume bar on right side
    int volPct = (g_volume * 100 + 127) / 255;
    char volBuf[16];
    snprintf(volBuf, sizeof(volBuf), "V:%3d%%", volPct);
    FontEngine::drawText(DisplayTarget::OLED, 152, 30, volBuf, FontStyle::Tiny);

    u8g2.sendBuffer();
}

void ui_show_help() {
    beginEinkScreen();

    FontEngine::drawText(DisplayTarget::EINK, 8, 18,
                         "PocketMuse Controls", FontStyle::Body);
    display.drawFastHLine(0, 22, display.width(), GxEPD_BLACK);

    struct Row { const char* key; const char* desc; };
    Row rows[] = {
        { "LEFT/RIGHT","Navigate / skip track"       },
        { "UP/DOWN",  "Volume (Now Playing)"         },
        { "SPACE",    "Play / pause"                },
        { "ENTER",    "Select / play"               },
        { "P",        "Cycle source (Lib/Fav/Pl)"    },
        { "F",        "Toggle favorite"              },
        { "S",        "Toggle shuffle"               },
        { "L",        "Cycle loop (None/One/All)"  },
        { "D",        "Delete from playlist"         },
        { "B",        "Back to browser"              },
        { "?",        "Show this screen"             },
        { "ESC / A",  "Exit to OS"                   },
    };

    int y = 38;
    for (auto& r : rows) {
        FontEngine::drawText(DisplayTarget::EINK, 8, y, r.key, FontStyle::Tiny);
        FontEngine::drawText(DisplayTarget::EINK, 82, y, r.desc, FontStyle::Tiny);
        y += 12;
    }

    display.drawFastHLine(0, y + 4, display.width(), GxEPD_BLACK);
    FontEngine::drawText(DisplayTarget::EINK, 8, y + 16,
                         "Press any key to close", FontStyle::Tiny);

    draw_footer("PocketMuse help");
    EINK().refresh();
}
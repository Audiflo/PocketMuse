#include <globals.h>
#include "muse.h"

static constexpr int kLineH = 10;

void draw_header(const char* title) {
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 16);
    display.print(title);
    display.drawFastHLine(0, 22, display.width(), GxEPD_BLACK);
}

void draw_footer(const char* hints) {
    display.drawFastHLine(0, display.height() - 14, display.width(), GxEPD_BLACK);
    display.setFont(&Font5x7Fixed);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(4, display.height() - 4);
    display.print(hints);
}

void draw_scrollbar(int total, int visible, int position) {
    if (total <= visible) return;

    int barTop = 24;
    int barH = display.height() - 14 - barTop - 2;
    int thumbH = (barH * visible) / total;
    if (thumbH < 6) thumbH = 6;

    int maxPos = total - visible;
    if (maxPos <= 0) maxPos = 1;
    int thumbY = barTop + ((barH - thumbH) * position) / maxPos;

    display.drawFastVLine(display.width() - 4, barTop, barH, GxEPD_BLACK);
    display.fillRect(display.width() - 6, thumbY, 4, thumbH, GxEPD_BLACK);
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
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(0, 8);

    if (g_playState == PlayerState::Playing) {
        u8g2.print(">> ");
        u8g2.print(g_nowTitle);
        if (g_nowArtist[0]) {
            u8g2.print(" - ");
            u8g2.print(g_nowArtist);
        }
    } else if (g_playState == PlayerState::Paused) {
        u8g2.print("|| ");
        u8g2.print(g_nowTitle);
    } else if (g_nowTitle[0]) {
        u8g2.print("   ");
        u8g2.print(g_nowTitle);
    } else {
        u8g2.print("   PocketMuse");
    }

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
    u8g2.setCursor(152, 20);
    u8g2.print(timeStr);

    // Bottom line: source + count + volume
    u8g2.setCursor(0, 30);
    switch (g_playlistMgr.source()) {
        case PlaySource::Library:   u8g2.print("Library"); break;
        case PlaySource::Favorites: u8g2.print("Favorites"); break;
        case PlaySource::Playlist:  u8g2.print("Playlist"); break;
    }
    u8g2.printf(" [%d]", g_trackCount);

    // Volume bar on right side
    int volPct = (g_volume * 100 + 127) / 255;
    u8g2.setCursor(152, 30);
    u8g2.printf("V:%3d%%", volPct);

    u8g2.sendBuffer();
}

void ui_show_help() {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 18);
    display.print("PocketMuse Controls");
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

    display.setFont(&Font5x7Fixed);
    int y = 38;
    for (auto& r : rows) {
        display.setCursor(8, y);   display.print(r.key);
        display.setCursor(82, y);  display.print(r.desc);
        y += 12;
    }

    display.drawFastHLine(0, y + 4, display.width(), GxEPD_BLACK);
    display.setCursor(8, y + 16);
    display.print("Press any key to close");

    EINK().refresh();
}

#include <globals.h>
#include "muse.h"
#include "albumart.h"

static constexpr int kArtX = 8;
static constexpr int kArtY = 28;
static constexpr int kArtSize = 120;
static constexpr int kInfoX = 136;
static constexpr int kInfoW = 176;

void nowplaying_init() {
    g_appMode = MODE_NOWPLAYING;
    g_needsRedraw = true;
}

static void truncate_text(const char* src, char* dst, size_t maxDst) {
    strncpy(dst, src, maxDst - 1);
    dst[maxDst - 1] = '\0';
}

void nowplaying_process_key(char ch) {
    int n = g_trackCount > 0 ? g_trackCount : 1;

    switch (ch) {
    case 32: case 13: // Space or Enter
        if (g_playState == PlayerState::Playing) {
            music_pause();
            g_playState = PlayerState::Paused;
        } else if (g_playState == PlayerState::Paused) {
            music_resume();
            g_playState = PlayerState::Playing;
        } else {
            if (g_nowTrackIndex >= 0 && g_nowTrackIndex < n) {
                player_play_index(g_nowTrackIndex);
            }
        }
        g_needsRedraw = true;
        break;
    case 19: // LEFT - prev track
        if (n > 0) player_prev_track();
        g_needsRedraw = true;
        break;
    case 21: // RIGHT - next track
        if (n > 0) player_next_track();
        g_needsRedraw = true;
        break;
    case 24: // UP - volume up
        if (g_volume < 245) g_volume += 10;
        else g_volume = 255;
        g_needsRedraw = true;
        break;
    case 25: // DOWN - volume down
        if (g_volume > 10) g_volume -= 10;
        else g_volume = 0;
        g_needsRedraw = true;
        break;
    case 'b': case 'B':
        g_appMode = MODE_BROWSER;
        g_needsRedraw = true;
        break;
    case 'p': case 'P':
        if (g_trackCount > 0) {
            g_appMode = MODE_PLAYLIST;
            g_needsRedraw = true;
        }
        break;
    case 'f': case 'F': {
        if (g_nowPath[0]) {
            g_isFavorite = g_playlistMgr.toggleFavorite(g_nowPath);
        }
        g_needsRedraw = true;
        break;
    }
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
    case '?':
        g_prevMode = g_appMode;
        g_appMode = MODE_HELP;
        g_needsRedraw = true;
        break;
    }
}

static uint8_t  s_artCache[(kArtSize * kArtSize) / 8];
static char     s_artCachePath[256] = {};
static int      s_cachedW = 0, s_cachedH = 0;

void nowplaying_cache_art(const char* path) {
    s_cachedW = 0;
    s_cachedH = 0;
    s_artCachePath[0] = '\0';

    if (!path || !path[0]) return;

    AlbumArt art;
    if (!art.load(path)) return;

    if (art.render1Bit(s_artCache, kArtSize, kArtSize, 0, 0, kArtSize)) {
        s_cachedW = art.width();
        s_cachedH = art.height();
        strncpy(s_artCachePath, path, sizeof(s_artCachePath) - 1);
        s_artCachePath[sizeof(s_artCachePath) - 1] = '\0';
    }
}

void nowplaying_render() {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);

    if (g_nowTitle[0] == '\0' && g_nowPath[0] == '\0') {
        draw_header("Now Playing");
        display.setFont(&Font5x7Fixed);
        display.setCursor(8, 80);
        display.print("Nothing playing");
        display.setCursor(8, 94);
        display.print("Select a track from the browser");
        draw_footer("B:browser ?:help");
        EINK().refresh();
        return;
    }

    // Header
    char hdr[32];
    if (g_playState == PlayerState::Playing) {
        snprintf(hdr, sizeof(hdr), "Now Playing");
    } else if (g_playState == PlayerState::Paused) {
        snprintf(hdr, sizeof(hdr), "Paused");
    } else {
        snprintf(hdr, sizeof(hdr), "Stopped");
    }
    draw_header(hdr);

    bool artDrawn = false;
    if (s_cachedW > 0 && s_cachedH > 0) {
        display.drawBitmap(kArtX, kArtY, s_artCache,
                           s_cachedW, s_cachedH, GxEPD_BLACK);
        artDrawn = true;
    }

    // If no art, draw a placeholder frame
    if (!artDrawn) {
        display.drawRect(kArtX, kArtY, kArtSize, kArtSize, GxEPD_BLACK);
        display.setFont(&Font5x7Fixed);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(kArtX + 32, kArtY + 64);
        display.print("No Art");
    }

    // Track info
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);

    char buf[128];

    // Title
    truncate_text(g_nowTitle, buf, sizeof(buf));
    display.setCursor(kInfoX, kArtY + 18);
    display.print(buf);

    // Artist
    truncate_text(g_nowArtist, buf, sizeof(buf));
    if (buf[0]) {
        display.setCursor(kInfoX, kArtY + 40);
        display.print(buf);
    }

    // Album
    truncate_text(g_nowAlbum, buf, sizeof(buf));
    if (buf[0]) {
        display.setCursor(kInfoX, kArtY + 60);
        display.print(buf);
    }

    // Status line
    display.setFont(&Font5x7Fixed);
    int statusY = kArtY + 82;

    int col = kInfoX;
    if (g_isFavorite) {
        display.setCursor(col, statusY);
        display.print("Fav");
        col += 40;
    }

    display.setCursor(col, statusY);
    switch (g_loopMode) {
        case LoopMode::None: display.print(""); break;
        case LoopMode::One:  display.print("R1"); break;
        case LoopMode::All:  display.print("RA"); break;
    }
    if (g_loopMode != LoopMode::None) col += 35;

    display.setCursor(col, statusY);
    if (g_shuffleEnabled) {
        display.print("Sf");
        col += 30;
    }

    // Volume
    display.setCursor(col, statusY);
    display.printf("V:%d", (g_volume * 100 + 127) / 255);

    // Play/pause status
    int playStatusY = statusY + 14;
    display.setCursor(kInfoX, playStatusY);
    if (g_playState == PlayerState::Playing) {
        display.print("> Playing");
    } else if (g_playState == PlayerState::Paused) {
        display.print("|| Paused");
    } else {
        display.print("[] Stopped");
    }

    // Progress bar
    int barY = kArtY + kArtSize + 14;
    int barW = display.width() - 16;
    display.drawRect(8, barY, barW, 8, GxEPD_BLACK);
    if (g_nowProgress > 0.0f && g_nowProgress <= 1.0f) {
        int filled = (int)((barW - 2) * g_nowProgress);
        if (filled > 0) display.fillRect(9, barY + 1, filled, 6, GxEPD_BLACK);
    }

    // Time
    char timeBuf[32];
    char elapsedStr[16];
    char durationStr[16];
    uint32_t elapsed = (uint32_t)(g_nowProgress * g_nowDuration);
    format_time(elapsedStr, sizeof(elapsedStr), elapsed);
    format_time(durationStr, sizeof(durationStr), g_nowDuration);
    snprintf(timeBuf, sizeof(timeBuf), "%s / %s", elapsedStr, durationStr);

    display.setFont(&Font5x7Fixed);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, barY + 24);
    display.print(timeBuf);

    // Footer
    draw_footer("SPC:pause < >:skip /\\/:vol B:browser P:plist ?:help");

    EINK().refresh();
}

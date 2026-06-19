#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "player.h"
#include "library.h"
#include "playlist.h"

// App modes
enum AppMode : uint8_t {
    MODE_BROWSER,
    MODE_NOWPLAYING,
    MODE_PLAYLIST,
    MODE_HELP,
};

// App globals
extern AppMode  g_appMode;
extern AppMode  g_prevMode;
extern bool     g_needsRedraw;
extern int      g_selIndex;

// Library / source
extern Library         g_library;
extern PlaylistManager g_playlistMgr;
extern int             g_trackCount;

// Now playing
extern char     g_nowPath[256];
extern char     g_nowTitle[128];
extern char     g_nowArtist[128];
extern char     g_nowAlbum[128];
extern uint32_t g_nowDuration;
extern float    g_nowProgress;
extern int      g_nowTrackIndex;
extern bool     g_isFavorite;
extern PlayerState g_playState;

// Playback
extern LoopMode g_loopMode;
extern bool     g_shuffleEnabled;

// Player instance (defined in main.cpp)
extern Player player;

// UI prototypes
void browser_init();
void browser_process_key(char ch);
void browser_render();

void nowplaying_init();
void nowplaying_process_key(char ch);
void nowplaying_render();
void nowplaying_cache_art(const char* path);

void playlist_process_key(char ch);
void playlist_render();

void ui_update_oled();
void ui_show_help();

// Shared UI helpers
void draw_header(const char* title);
void draw_footer(const char* hints);
void draw_scrollbar(int total, int visible, int position);
void format_time(char* buf, size_t bufSize, uint32_t seconds);
void get_display_name(const char* path, char* out, size_t maxLen);

// Player control helpers
void player_play_index(int index);
void player_next_track();
void player_prev_track();
void player_cycle_source();
void player_toggle_favorite();
const char* get_track_path(int index);

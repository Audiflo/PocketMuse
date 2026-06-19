#include "muse.h"

AppMode  g_appMode       = MODE_BROWSER;
AppMode  g_prevMode      = MODE_BROWSER;
bool     g_needsRedraw   = false;
int      g_selIndex      = 0;

Library         g_library;
PlaylistManager g_playlistMgr;
int             g_trackCount = 0;

char     g_nowPath[256]    = {};
char     g_nowTitle[128]   = {};
char     g_nowArtist[128]  = {};
char     g_nowAlbum[128]   = {};
uint32_t g_nowDuration     = 0;
float    g_nowProgress     = 0.0f;
int      g_nowTrackIndex   = -1;
bool     g_isFavorite      = false;
PlayerState g_playState    = PlayerState::Stopped;

LoopMode g_loopMode        = LoopMode::None;
bool     g_shuffleEnabled  = false;

#include <globals.h>
#include "AudioTools.h"
#include "AudioTools/CoreAudio/AudioPlayer.h"
#include "AudioTools/AudioCodecs/CodecHelix.h"
#include "AudioTools/Disk/AudioSource.h"
#include "muse.h"
#include "metacache.h"

#if OTA_APP

using namespace audio_tools;

// Audio pipeline components (global so callbacks can reach them)
static AudioSourceCallback    s_source;       // callbacks open files from SD
static DecoderHelix           s_decoder;      // MP3/AAC/WAV auto-detect
static I2SStream              s_i2sOut;       // I2S DMA output on GPIO 17
AudioPlayer                   s_player(s_source, s_i2sOut, s_decoder);

// Currently-open audio file and its size (updated by callbacks)
static File s_audioFile;
static size_t s_fileSize = 0;

// FileLoop for LoopMode::One (wraps current file for endless replay)
static FileLoop s_fileLoop;
static bool s_useLoop = false;

// Metadata cache
static MetadataCache s_metaCache;
static int s_metaFields = 0;

// Metadata callback (fires during s_player.copy())
static void onMetadata(MetaDataType type, const char* str, int len) {
    switch (type) {
        case Title:
            strncpy(g_nowTitle, str, sizeof(g_nowTitle) - 1);
            g_nowTitle[sizeof(g_nowTitle) - 1] = '\0';
            s_metaFields |= 1;
            break;
        case Artist:
            strncpy(g_nowArtist, str, sizeof(g_nowArtist) - 1);
            g_nowArtist[sizeof(g_nowArtist) - 1] = '\0';
            s_metaFields |= 2;
            break;
        case Album:
            strncpy(g_nowAlbum, str, sizeof(g_nowAlbum) - 1);
            g_nowAlbum[sizeof(g_nowAlbum) - 1] = '\0';
            s_metaFields |= 4;
            break;
        default: break;
    }
    // Cache when all three fields are received
    if (s_metaFields == 7 && g_nowPath[0]) {
        s_metaCache.save(g_nowPath, g_nowTitle, g_nowArtist, g_nowAlbum);
    }
}

// Pre-read metadata from file (faster than waiting for streaming callback)
static void pre_read_metadata(const char* path) {
    if (!path || !path[0] || !global_fs) return;

    g_nowTitle[0] = '\0';
    g_nowArtist[0] = '\0';
    g_nowAlbum[0] = '\0';
    s_metaFields = 0;

    // Check cache first
    if (s_metaCache.load(path, g_nowTitle, sizeof(g_nowTitle),
                         g_nowArtist, sizeof(g_nowArtist),
                         g_nowAlbum, sizeof(g_nowAlbum))) {
        return;
    }

    // Cache miss — quick pre-read of first 10 KB
    File f = global_fs->open(path, "r");
    if (!f) return;

    uint8_t buf[10240];
    size_t n = f.read(buf, sizeof(buf));
    f.close();
    if (n == 0) return;

    MetaDataID3 meta;
    meta.setCallback(onMetadata);
    meta.begin();
    meta.write(buf, n);
    meta.end();

    if (g_nowTitle[0] || g_nowArtist[0] || g_nowAlbum[0]) {
        s_metaCache.save(path, g_nowTitle, g_nowArtist, g_nowAlbum);
    }
}

// AudioSourceCallbacks
static Stream* openStream(const char* path, size_t& fileSize) {
    if (!path || !path[0]) return nullptr;
    File f = global_fs->open(path, "r");
    if (!f) return nullptr;
    fileSize = f.size();

    if (g_loopMode == LoopMode::One) {
        s_fileLoop.end();
        s_fileLoop.setFile(f);
        s_fileLoop.setLoopCount(-1);
        s_fileLoop.begin();
        s_useLoop = true;
        return &s_fileLoop;
    }

    s_audioFile = f;
    s_useLoop = false;
    return &s_audioFile;
}

static Stream* onSelectStream(int index) {
    s_fileLoop.end();
    if (s_audioFile) s_audioFile.close();

    if (index == -1) {
        return openStream(s_source.getPath(), s_fileSize);
    }

    char path[256];
    if (g_playlistMgr.source() == PlaySource::Library) {
        if (index < 0 || index >= g_library.count()) return nullptr;
        strncpy(path, g_library.path(index).c_str(), sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    } else {
        if (!g_playlistMgr.getEntry(index, path, sizeof(path))) return nullptr;
    }
    return openStream(path, s_fileSize);
}

// Called by AudioPlayer::next() on EOF for auto-advance
static Stream* onNextStream(int offset) {
    if (offset <= 0) return nullptr;

    if (g_loopMode == LoopMode::One) {
        // FileLoop handles endless replay; this should never be called
        return nullptr;
    }

    if (g_loopMode == LoopMode::All) {
        int next = (g_nowTrackIndex + 1) % g_trackCount;
        char path[256];
        if (!get_track_path(next, path, sizeof(path))) return nullptr;
        s_fileLoop.end();
        if (s_audioFile) s_audioFile.close();

        File f = global_fs->open(path, "r");
        if (!f) return nullptr;
        s_fileSize = f.size();
        s_audioFile = f;
        s_useLoop = false;

        g_nowTrackIndex = next;
        strncpy(g_nowPath, path, sizeof(g_nowPath) - 1);
        g_nowPath[sizeof(g_nowPath) - 1] = '\0';
        g_nowDuration = compute_duration(path);
        g_nowProgress = 0.0f;
        pre_read_metadata(path);
        g_needsRedraw = true;
        return &s_audioFile;
    }

    return nullptr;
}

// Playback control helpers (called from ui_*.cpp)
void music_stop() {
    s_player.setActive(false);
}

void music_play(const char* path) {
    pre_read_metadata(path);
    s_player.setAutoFade(true);
    s_player.setPath(path);
    s_player.play();
}

void music_pause() {
    s_player.setAutoFade(false);
    s_player.setActive(false);
}

void music_resume() {
    s_player.play();
}

// appTask: UI, progress, volume
static void appTask(void*) {
    vTaskDelay(pdMS_TO_TICKS(200));
    for (;;) {
        processKB_APP();

        // Touch slider: volume
        {
            static unsigned long lastTouchMs = 0;
            unsigned long now = millis();
            int scroll = 0;
            if (now - lastTouchMs >= 200) {
                lastTouchMs = now;
                scroll = TOUCH().getScrollVector();
            }
            if (scroll > 0 && g_volume < 245) {
                g_volume += 10;
                s_player.setVolume(g_volume / 255.0f);
                g_needsRedraw = true;
            } else if (scroll < 0 && g_volume > 10) {
                g_volume -= 10;
                s_player.setVolume(g_volume / 255.0f);
                g_needsRedraw = true;
            }
        }

        // Progress from file position
        if (s_useLoop) {
            if (s_fileLoop.file() && s_fileSize > 0) {
                g_nowProgress = (float)s_fileLoop.file().position() / s_fileSize;
            }
        } else if (s_audioFile && s_fileSize > 0) {
            g_nowProgress = (float)s_audioFile.position() / s_fileSize;
        }

        // Update global play state
        if (s_player.isActive()) {
            g_playState = PlayerState::Playing;
        } else if (g_nowTrackIndex >= 0 && g_nowPath[0]) {
            g_playState = PlayerState::Paused;
        } else {
            g_playState = PlayerState::Stopped;
        }

        ui_update_oled();
        vTaskDelay(pdMS_TO_TICKS(50));
        yield();
    }
}

// Application entry points (called from PocketMage framework)
void APP_INIT() {
    // Restore volume
    prefs.begin("PocketMuse", true);
    g_volume = prefs.getUChar("volume", 200);
    prefs.end();

    // Configure I2S output on BZ_PIN (GPIO 17).
    // Single-pin mode: BCK and WS clocks generated internally, only DATA pin used.
    // A simple RC low-pass filter on the pin converts the bitstream to analog audio.
    auto cfg = s_i2sOut.defaultConfig();
    cfg.pin_data = BZ_PIN;
    cfg.pin_bck = -1;
    cfg.sample_rate = 44100;
    cfg.channels = 2;
    cfg.bits_per_sample = 16;
    cfg.signal_type = PDM;
    s_i2sOut.begin(cfg);

    // Set up AudioSourceCallback
    s_source.setCallbackSelectStream(onSelectStream);

    // Initialize the pipeline (setupFade, volume_out.begin, etc.)
    // without selecting a stream or activating playback.
    s_player.begin(-1, false);

    // Enable auto-next for gapless track advancement
    s_player.setAutoNext(true);
    s_source.setCallbackNextStream(onNextStream);

    // Set up metadata callback (fires during copy() via built-in MetaDataID3)
    s_player.setMetadataCallback(onMetadata);

    // Set volume
    s_player.setVolume(g_volume / 255.0f);

    // Scan library
    if (global_fs) {
        g_library.scan();
        g_playlistMgr.begin();
    }
    g_trackCount = g_library.count();
    browser_init();
}

void processKB_APP() {
    char ch = KB().updateKeypress();
    if (!ch) return;

    if (ch == 18) {
        KB().setKeyboardState(KB().getKeyboardState() == FUNC ? NORMAL : FUNC);
        return;
    }
    if (ch == 17) {
        KB().setKeyboardState(KB().getKeyboardState() == SHIFT ? NORMAL : SHIFT);
        return;
    }
    if (ch == 12 || ch == 27 || ch == 65) {
        music_stop();
        prefs.begin("PocketMuse", false);
        prefs.putUChar("volume", g_volume);
        prefs.end();
        KB().setKeyboardState(NORMAL);
        rebootToPocketMage();
        return;
    }

    switch (g_appMode) {
        case MODE_BROWSER:     browser_process_key(ch);     break;
        case MODE_NOWPLAYING:  nowplaying_process_key(ch);  break;
        case MODE_PLAYLIST:    playlist_process_key(ch);    break;
        case MODE_HELP:        g_appMode = g_prevMode; g_needsRedraw = true; break;
    }

    ui_update_oled();
}

void einkHandler_APP() {
    if (!g_needsRedraw) return;

    g_needsRedraw = false;

    switch (g_appMode) {
        case MODE_HELP:       ui_show_help();       break;
        case MODE_BROWSER:    browser_render();     break;
        case MODE_NOWPLAYING: nowplaying_render();  break;
        case MODE_PLAYLIST:   playlist_render();    break;
    }
}

void applicationEinkHandler() {
    einkHandler_APP();
}

void einkHandler(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(250));
    UBaseType_t minStack = UINT32_MAX;
    for (;;) {
        applicationEinkHandler();
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        if (hwm < minStack) {
            minStack = hwm;
            Serial.printf("einkHandlerTask HWM: %u\n", hwm);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        yield();
    }
}

void setup() {
    PocketMage_INIT();
    xTaskCreatePinnedToCore(appTask, "appTask", 32768, NULL, 2, NULL, 1);
    APP_INIT();
}

void loop() {
    s_player.copy();
}

#endif

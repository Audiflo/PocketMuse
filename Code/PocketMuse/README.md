# PocketMuse

Music player app for the PocketMage PDA. Streams MP3, (AAC and WAV soon)
from SD card over I2S single-pin PDM output through the piezo buzzer GPIO.
Supports library browsing, now-playing with album art, playlist management,
shuffle, and loop modes. Runs as an OTA application slot on the PocketMage OS.

## Build

Requires PlatformIO, `just`, and the standard ESP32-S3 toolchain.

```sh
just build
```

This runs two scripts in sequence:

1. `scripts/build_firmware.py` - run `pio run -e OTA_APP`.
2. `scripts/package_ota.py` - wrap `firmware.bin` into `PocketMuse.tar` for
   OTA distribution.

The default environment is `OTA_APP`. Set `env` in the `Justfile` or pass
`ENV=<name>` to build a different PlatformIO environment.

```sh
just upload      # build + upload via USB
just monitor     # build + upload + serial monitor
```

## Key bindings

| Input        | Action                     |
|--------------|----------------------------|
| LEFT / RIGHT | Navigate list / skip track |
| UP / DOWN    | Volume (Now Playing)       |
| SPACE        | Play / pause               |
| ENTER        | Select / play              |
| P            | Cycle source (Lib/Fav/Pl)  |
| F            | Toggle favorite            |
| S            | Toggle shuffle             |
| L            | Cycle loop (None/One/All)  |
| D            | Delete from playlist       |
| B            | Back to browser            |
| ?            | Help overlay               |
| ESC / A      | Exit to PocketMage OS      |

Touch scroll on the slider controls volume in all modes.

## Dependencies

- **PlatformIO** (`espressif32` platform, Arduino framework)
- **arduino-audio-tools** - audio pipeline (I2S, source callbacks, fading)
- **arduino-libhelix** - MP3/AAC decode
- **GxEPD2** (fork:
  [`ashtf8/GxEPD2_Editable_useFastFullUpdate`](https://github.com/ashtf8/GxEPD2_Editable_useFastFullUpdate))
- **JPEGDEC** - album art decode
- **PNGdec** - album art decode
- **U8g2** - OLED display
- Adafruit GFX fonts (FreeSans, FreeMono, Font5x7Fixed, etc.)

## Project layout

```text
src/
   main.cpp             audio pipeline, app entry, setup/loop
   muse.cpp             app-wide globals
   library.cpp          SD card track scanner (/music/)
   playlist.cpp         playlist manager (favorites, .m3u)
   metacache.cpp        ID3 metadata cache on SD
   albumart.cpp         ID3v2 APIC extraction, JPEG/PNG decode, 1-bit render
   ui_player.cpp        playback control, shuffle, duration compute
   ui/
      ui_browser.cpp    library/favorites/playlist browser
      ui_nowplaying.cpp now-playing screen with album art
      ui_playlist.cpp   playlist queue view
      ui_helpers.cpp    header/footer/scrollbar/OLED/time formatting
include/                public headers
lib/
   PocketMage/          shared PocketMage PDA framework (submodule)
scripts/
   build_firmware.py     PlatformIO build helper
   package_ota.py       firmware -> tar for OTA
```

## License

Apache-2.0. Same as the PocketMage project.

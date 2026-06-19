#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <FS.h>

struct SongMetadata {
    String title;
    String artist;
    String album;
    int year  = 0;
    int track = 0;
    bool valid = false;
};

// Parse ID3v2 header at current file position (expected at offset 0).
// File position after return is at the end of the tag (start of MPEG data).
bool parseID3v2(File& file, SongMetadata& meta);

// Parse ID3v1 tag at the end of the file.
bool parseID3v1(File& file, SongMetadata& meta);

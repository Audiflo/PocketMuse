#include "library.h"
#include <globals.h>
#include <algorithm>

Library::Library() {}

void Library::clear() {
    tracks_.clear();
}

int Library::scan() {
    clear();

    if (!global_fs) {
        return -1;
    }

    File root = global_fs->open("/music");
    if (!root || !root.isDirectory()) {
        return -1;
    }
    root.close();

    scanDir_("/music", 0);

    std::sort(tracks_.begin(), tracks_.end(),
              [](const String& a, const String& b) {
                  return strcasecmp(a.c_str(), b.c_str()) < 0;
              });

    return count();
}

const String& Library::path(int index) const {
    return tracks_[index];
}

void Library::scanDir_(const char* dirname, int depth) {
    File dir = global_fs->open(dirname);
    if (!dir || !dir.isDirectory()) {
        return;
    }

    File entry = dir.openNextFile();
    while (entry) {
        if (entry.isDirectory()) {
            if (depth + 1 < kMaxDepth) {
                String sub = String(dirname) + "/" + entry.name();
                scanDir_(sub.c_str(), depth + 1);
            }
        } else {
            String name = entry.name();
            if (name.endsWith(".mp3") || name.endsWith(".MP3")) {
                String full = String(dirname) + "/" + name;
                tracks_.push_back(full);
            }
        }
        entry = dir.openNextFile();
    }

    dir.close();
}

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <FS.h>

class Decoder;
class RingBuffer;

enum class PlayerState : uint8_t {
    Stopped,
    Playing,
    Paused,
};

enum class LoopMode : uint8_t {
    None,
    One,
    All,
};

class Player {
public:
    Player(Decoder& dec, RingBuffer& rb);

    void play(const char* path);
    void pause();
    void resume();
    void stop();

    PlayerState state() const { return state_; }
    const char*  currentPath() const { return current_path_; }

    void tick();

    void seek(float fraction);
    float progress() const;

    void setLoopMode(LoopMode mode) { loop_mode_ = mode; }
    LoopMode loopMode() const       { return loop_mode_; }

    void toggleShuffle();
    bool isShuffle() const          { return shuffle_; }

    uint32_t fileSize() const       { return file_size_; }
    uint32_t durationSec() const    { return duration_sec_; }

private:
    static constexpr size_t kReadBufSize = 4096;

    void closeFile_();
    void openFile_(const char* path);
    void handleSeek_();
    void updateDuration_();

    Decoder&    dec_;
    RingBuffer& rb_;
    volatile PlayerState state_;
    File        file_;
    uint8_t     read_buf_[kReadBufSize];
    char        current_path_[256];
    uint32_t    file_size_;
    uint32_t    duration_sec_;

    bool     seek_pending_;
    uint32_t seek_pos_;

    LoopMode loop_mode_;
    bool     shuffle_;
};

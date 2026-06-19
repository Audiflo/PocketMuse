#include "player.h"
#include "decoder.h"
#include "ringbuf.h"
#include <cstring>
#include <cstdlib>
#include <globals.h>

Player::Player(Decoder& dec, RingBuffer& rb)
    : dec_(dec)
    , rb_(rb)
    , state_(PlayerState::Stopped)
    , file_size_(0)
    , duration_sec_(0)
    , seek_pending_(false)
    , seek_pos_(0)
    , loop_mode_(LoopMode::None)
    , shuffle_(false)
{
    current_path_[0] = '\0';
}

void Player::closeFile_() {
    if (file_) {
        file_.close();
    }
}

void Player::openFile_(const char* path) {
    closeFile_();
    if (global_fs) {
        file_ = global_fs->open(path, "r");
    }
    if (file_) {
        dec_.reset();
        rb_.reset();
        file_size_ = file_.size();
        strncpy(current_path_, path, sizeof(current_path_) - 1);
        current_path_[sizeof(current_path_) - 1] = '\0';
        seek_pending_ = false;
        seek_pos_ = 0;
        updateDuration_();
    }
}

void Player::updateDuration_() {
    // Xing/Info header gives exact duration for VBR, but for now
    // estimate from file size and bitrate once the first frame decodes
    duration_sec_ = 0;
}

void Player::play(const char* path) {
    openFile_(path);
    if (file_) {
        state_ = PlayerState::Playing;
    }
}

void Player::pause() {
    if (state_ == PlayerState::Playing) {
        state_ = PlayerState::Paused;
    }
}

void Player::resume() {
    if (state_ == PlayerState::Paused) {
        state_ = PlayerState::Playing;
    }
}

void Player::stop() {
    closeFile_();
    dec_.reset();
    rb_.reset();
    seek_pending_ = false;
    state_ = PlayerState::Stopped;
}

void Player::seek(float fraction) {
    if (!file_ || state_ == PlayerState::Stopped) return;
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    seek_pos_ = (uint32_t)(file_size_ * fraction);
    seek_pending_ = true;
}

float Player::progress() const {
    if (file_size_ == 0 || !file_) return 0.0f;
    size_t pos = file_.position();
    return (float)pos / (float)file_size_;
}

void Player::toggleShuffle() {
    shuffle_ = !shuffle_;
}

void Player::handleSeek_() {
    if (!seek_pending_) return;
    dec_.reset();
    rb_.reset();
    file_.seek(seek_pos_, SeekSet);
    seek_pending_ = false;
}

void Player::tick() {
    handleSeek_();

    if (state_ != PlayerState::Playing) {
        return;
    }

    if (!file_) {
        state_ = PlayerState::Stopped;
        return;
    }

    size_t bytes_read = file_.read(read_buf_, kReadBufSize);
    if (bytes_read == 0) {
        closeFile_();

        if (loop_mode_ == LoopMode::One && current_path_[0]) {
            play(current_path_);
        } else {
            state_ = PlayerState::Stopped;
        }
        return;
    }

    dec_.input(read_buf_, bytes_read);

    int ret;
    do {
        ret = dec_.process();
    } while (ret > 0);

    // Update duration estimate after first decode
    if (duration_sec_ == 0 && dec_.sampleRate() > 0) {
        int bps = dec_.bitrate();
        if (bps > 0 && file_size_ > 0) {
            duration_sec_ = (file_size_ * 8) / bps;
        }
    }
}

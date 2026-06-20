#include "player.h"
#include "decoder.h"
#include "ringbuf.h"
#include "output.h"
#include <cstring>
#include <cstdlib>
#include <globals.h>

static uint32_t rd32be(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

Player::Player(Decoder& dec, RingBuffer& rb)
    : dec_(dec)
    , rb_(rb)
    , state_(PlayerState::Stopped)
    , file_size_(0)
    , duration_sec_(0)
    , paused_(false)
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
    Serial.printf("[player] open('%s') -> %s, size=%u\n",
        path, file_ ? "OK" : "FAIL", file_ ? file_.size() : 0);
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
    duration_sec_ = 0;
    if (!file_ || !global_fs) return;

    size_t savedPos = file_.position();
    file_.seek(0, SeekSet);

    uint8_t buf[8192];
    size_t n = file_.read(buf, sizeof(buf));
    file_.seek(savedPos, SeekSet);

    if (n < 10) return;

    // Find the first MP3 sync word
    for (size_t i = 0; i < n - 3; i++) {
        if (buf[i] != 0xFF || (buf[i+1] & 0xE0) != 0xE0) continue;

        int ver    = (buf[i+1] >> 3) & 3; // 3=MPEG1, 2=MPEG2, 0=MPEG2.5
        int layer  = (buf[i+1] >> 1) & 3; // 1=Layer III
        int prot   =  buf[i+1]       & 1; // 0=CRC present
        int br_idx =  buf[i+2] >> 4;
        int sr_idx = (buf[i+2] >> 2) & 3;
        int mode   =  buf[i+3] >> 6;

        if (layer != 1) continue;                // must be Layer III
        if (br_idx == 0 || br_idx == 15) continue;
        if (sr_idx == 3) continue;

        static const int kSR[4] = {44100, 48000, 32000, 0};
        static const int kSR25[4] = {22050, 24000, 16000, 0};
        int sr = (ver == 0) ? kSR25[sr_idx] : kSR[sr_idx];
        if (sr == 0) continue;

        int spf = (ver == 3) ? 1152 : 576;

        // Xing header offset past frame header + optional CRC + side info
        int ssize;
        if (ver == 3)      ssize = (mode == 3) ? 17 : 32;
        else               ssize = (mode == 3) ?  9 : 17;
        int xoff = i + 4 + (prot ? 0 : 2) + ssize;
        if (xoff + 12 > (int)n) continue;

        if (memcmp(buf + xoff, "Xing", 4) != 0 && memcmp(buf + xoff, "Info", 4) != 0)
            continue;

        uint32_t flags = rd32be(buf + xoff + 4);
        if (!(flags & 1)) continue; // frames flag not set

        uint32_t frames = rd32be(buf + xoff + 8);
        if (frames == 0) continue;

        duration_sec_ = (uint32_t)((uint64_t)frames * spf / sr);
        Serial.printf("[player] Xing frames=%u, spf=%d, sr=%d => duration=%u\n",
            frames, spf, sr, duration_sec_);
        return;
    }
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
        paused_ = true;
        if (output_) output_->pause();
    }
}

void Player::resume() {
    if (state_ == PlayerState::Paused) {
        state_ = PlayerState::Playing;
        paused_ = false;
        if (output_) output_->resume();
    }
}

void Player::stop() {
    closeFile_();
    dec_.reset();
    rb_.reset();
    if (output_) output_->stop();
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

    if (paused_ || !file_) {
        if (!file_) state_ = PlayerState::Stopped;
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

    // Ensure output runs at the decoded sample rate.  begin() handles
    // both first-start and on-the-fly reconfiguration when the rate
    // changes (e.g.  following track with different sample rate).
    if (output_ && dec_.sampleRate() > 0) {
        int sr = dec_.sampleRate();
        if (!output_->isRunning() || output_->sampleRate() != sr) {
            output_->begin(sr);
        }
    }

    // Update duration estimate after first decode
    if (duration_sec_ == 0 && dec_.sampleRate() > 0) {
        int bps = dec_.bitrate();
        if (bps > 0 && file_size_ > 0) {
            duration_sec_ = (uint32_t)((uint64_t)file_size_ * 8 / bps);
        }
    }
}

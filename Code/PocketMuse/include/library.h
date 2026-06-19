#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <Arduino.h>

class Library {
public:
    Library();

    int scan();
    void clear();

    int count() const { return (int)tracks_.size(); }
    const String& path(int index) const;

private:
    void scanDir_(const char* dirname, int depth);

    static constexpr int kMaxDepth = 2;

    std::vector<String> tracks_;
};

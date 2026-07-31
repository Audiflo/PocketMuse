#pragma once
#include <Arduino.h>
#include <vector>
#include <pocketmage_font/pocketmage_font.h>

// Find the longest prefix of s[0..n) that fits within maxTextWidth pixels
// using the given e-ink style. Word-aware: breaks at spaces.
// Returns the number of characters that fit (0 if none fit, or 1+ for newline).
size_t sliceThatFits(const char* s, size_t n, int maxTextWidth, FontStyle style);

// Truncate text to fit within maxWidthPx using the given e-ink style.
// Appends "..." when truncated and returns the shortened string.
String truncateWithEllipsis(const String& text, int maxWidthPx, FontStyle style);

// Word-wrap text to fit within maxWidthPx using the given font style.
// Returns one string per wrapped line.
std::vector<String> wordWrap(const String& text, int maxWidthPx, FontStyle style);

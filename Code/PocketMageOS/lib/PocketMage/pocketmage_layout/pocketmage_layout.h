#pragma once
#include <Arduino.h>

// Find the longest prefix of s[0..n) that fits within maxTextWidth pixels
// using FontEngine's current e-ink style. Word-aware: breaks at spaces.
// Returns the number of characters that fit (0 if none fit, or 1+ for newline).
size_t sliceThatFits(const char* s, size_t n, int maxTextWidth);

// Truncate text to fit within maxWidthPx using the current e-ink FontEngine
// style. Appends "..." when truncated and returns the shortened string.
String truncateWithEllipsis(const String& text, int maxWidthPx);

// Draw a vertical scrollbar on the global E-Ink display.
//   totalLines    - total number of lines in the document
//   visibleLines  - number of lines visible in the viewport
//   scrollIndex   - current scroll position (0 = top)
//   barWidth      - width of the scrollbar handle in pixels (default 3)
void drawScrollbar(int totalLines, int visibleLines, int scrollIndex, int barWidth = 3);

#include "pocketmage_layout.h"
#include <vector>

size_t sliceThatFits(const char* s, size_t n, int maxTextWidth, FontStyle style) {
  if (!s || n == 0) return 0;

  static char buf[256];
  const size_t cap = sizeof(buf) - 1;

  size_t best = 0, lastSpace = SIZE_MAX;
  size_t i = 0;
  size_t len = 0;

  while (i < n && len < cap) {
    char c = s[i];

    if (c == '\n' || c == '\r') return (best > 0) ? best : 1;

    if (c == ' ') lastSpace = i;

    buf[len++] = c;
    buf[len] = '\0';

    int w = FontEngine::textWidth(DisplayTarget::EINK, buf, style);
    if (w > maxTextWidth) break;

    best = i + 1;
    ++i;
  }

  const bool overflowed = (i < n) || (len >= cap);
  if (best == 0) return (n ? 1 : 0);

  if (overflowed && lastSpace != SIZE_MAX && lastSpace + 1 <= best) {
    return lastSpace + 1;
  }
  return best;
}

String truncateWithEllipsis(const String& text, int maxWidthPx, FontStyle style,
                            DisplayTarget target) {
  int w = FontEngine::textWidth(target, text, style);
  if (w <= maxWidthPx) return text;

  String dots("...");
  int dotsW = FontEngine::textWidth(target, dots, style);
  int avail = maxWidthPx - dotsW;
  if (avail <= 0) return dots;

  int lo = 0, hi = text.length();
  while (lo < hi) {
    int mid = (lo + hi + 1) / 2;
    if (FontEngine::textWidth(target, text.substring(0, mid), style) <= avail)
      lo = mid;
    else
      hi = mid - 1;
  }

  // Binary search can cut inside a UTF-8 multi-byte sequence; back off any
  // continuation byte (0x80..0xBF) so the prefix ends on a char boundary.
  while (lo > 0 && (static_cast<uint8_t>(text[lo]) & 0xC0) == 0x80) lo--;

  return text.substring(0, lo) + dots;
}

std::vector<String> wordWrap(const String& text, int maxWidthPx, FontStyle style) {
  std::vector<String> lines;
  const char* s = text.c_str();
  size_t len = text.length();
  size_t pos = 0;
  while (pos < len) {
    size_t n = sliceThatFits(s + pos, len - pos, maxWidthPx, style);
    if (n == 0) n = 1;
    String line(s + pos, n);
    line.trim();
    lines.push_back(line);
    pos += n;
  }
  return lines;
}

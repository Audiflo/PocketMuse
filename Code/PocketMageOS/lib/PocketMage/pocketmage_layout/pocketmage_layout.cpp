#include "pocketmage_layout.h"
#include <pocketmage_font/pocketmage_font.h>
#include <GxEPD2_BW.h>

// Forward-declare the global E-Ink display (declared in pocketmage_eink.h)
using PanelT   = GxEPD2_310_GDEQ031T10;
using DisplayT = GxEPD2_BW<PanelT, PanelT::HEIGHT>;
extern DisplayT display;

size_t sliceThatFits(const char* s, size_t n, int maxTextWidth) {
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

    int w = FontEngine::einkTextWidth(buf);
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

String truncateWithEllipsis(const String& text, int maxWidthPx) {
  int w = FontEngine::einkTextWidth(text);
  if (w <= maxWidthPx) return text;

  String dots("...");
  int dotsW = FontEngine::einkTextWidth(dots);
  int avail = maxWidthPx - dotsW;
  if (avail <= 0) return dots;

  int lo = 0, hi = text.length();
  while (lo < hi) {
    int mid = (lo + hi + 1) / 2;
    if (FontEngine::einkTextWidth(text.substring(0, mid)) <= avail)
      lo = mid;
    else
      hi = mid - 1;
  }

  return text.substring(0, lo) + dots;
}

void drawScrollbar(int totalLines, int visibleLines, int scrollIndex, int barWidth) {
  int maxScroll = totalLines - visibleLines;
  if (maxScroll <= 0) return;

  int barX = display.width() - barWidth;

  float visibleRatio = (float)visibleLines / totalLines;
  int handleHeight = max((int)(display.height() * visibleRatio), 15);

  float scrollFraction = (float)scrollIndex / maxScroll;
  if (scrollFraction > 1.0f) scrollFraction = 1.0f;
  int handleY = scrollFraction * (display.height() - handleHeight);

  display.fillRect(barX, handleY, barWidth, handleHeight, GxEPD_BLACK);

  display.drawFastHLine(barX, display.height() - 1, barWidth, GxEPD_WHITE);
  display.drawFastHLine(barX, 0, barWidth, GxEPD_WHITE);
}

#include "pocketmage_ui.h"
#include <pocketmage_eink/pocketmage_eink.h>
#include <pocketmage_layout/pocketmage_layout.h>

void drawScrollbar(int total, int visible, int index,
                   int barX, int barY, int barH, int barW, bool clearBg,
                   uint16_t fg, uint16_t bg) {
  int maxScroll = total - visible;
  if (maxScroll <= 0) return;

  if (barX < 0) barX = display.width() - barW;
  if (barH < 0) barH = display.height();

  float visibleRatio = min((float)visible / total, 1.0f);
  int handleHeight = max((int)(barH * visibleRatio), 15);

  int clamped = index < 0 ? 0 : (index > maxScroll ? maxScroll : index);
  float scrollFraction = (float)clamped / maxScroll;
  int handleY = barY + (int)(scrollFraction * (barH - handleHeight));

  if (clearBg) {
    display.fillRect(barX - 1, barY, barW + 1, barH, bg);
  }

  display.fillRect(barX, handleY, barW, handleHeight, fg);

  display.drawFastHLine(barX, display.height() - 1, barW, bg);
  display.drawFastHLine(barX, 0, barW, bg);
}

void beginEinkScreen(bool preserveBg) {
  EINK().resetDisplay(!preserveBg);
}

void endEinkScreen(const char* statusText, EinkRefresh mode) {
  if (!statusText) statusText = "";
  EINK().drawStatusBar(statusText);
  switch (mode) {
    case EinkRefresh::Normal:
      EINK().refresh();
      break;
    case EinkRefresh::ForceFull:
      EINK().forceSlowFullUpdate(true);
      EINK().refresh();
      break;
    case EinkRefresh::MultiPass1:
      EINK().multiPassRefresh(1);
      break;
    case EinkRefresh::MultiPass2:
      EINK().multiPassRefresh(2);
      break;
  }
}

void drawListItem(int x, int y, const String& text, int maxWidth) {
  FontEngine::setEinkStyle(FontStyle::Body);
  if (maxWidth > 0) {
    String s = text;
    int w = FontEngine::einkTextWidth(s);
    if (w > maxWidth) {
      s = truncateWithEllipsis(s, maxWidth);
    }
    FontEngine::einkDraw(x, y, s);
  } else {
    FontEngine::einkDraw(x, y, text);
  }
}

void drawAppIcon(int x, int y, const uint8_t* bitmap, const String& name) {
  display.fillRect(x, y, 40, 40, GxEPD_WHITE);
  if (bitmap) {
    display.drawBitmap(x, y, bitmap, 40, 40, GxEPD_BLACK);
  }
  FontEngine::setEinkStyle(FontStyle::Body);
  int w = FontEngine::einkTextWidth(name);
  FontEngine::einkDraw(x + (40 - w) / 2, y + 53, name);
}

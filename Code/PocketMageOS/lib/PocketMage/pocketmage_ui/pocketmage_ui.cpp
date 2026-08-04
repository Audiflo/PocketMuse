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
  }
}

void drawListItem(int x, int y, const String& text, int maxWidth) {
  if (maxWidth > 0) {
    String s = text;
    int w = FontEngine::textWidth(DisplayTarget::EINK, s, FontStyle::Body);
    if (w > maxWidth) {
      s = truncateWithEllipsis(s, maxWidth, FontStyle::Body);
    }
    FontEngine::drawText(DisplayTarget::EINK, x, y, s, FontStyle::Body);
  } else {
    FontEngine::drawText(DisplayTarget::EINK, x, y, text, FontStyle::Body);
  }
}

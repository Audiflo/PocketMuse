//   .oooooo.    ooooo        oooooooooooo oooooooooo.    //
//  d8P'  `Y8b  `888'        `888'     `8 `888'   `Y8b   //
//  888      888  888         888          888      888  //
//  888      888  888         888oooo8     888      888  //
//  888      888  888         888    "     888      888  //
//  `88b    d88'  888       o  888       o  888     d88'  //
//   `Y8bood8P'  o888ooooood8 o888ooooood8 o888bood8P'    //

#include <pocketmage.h>

static constexpr const char* tag = "OLED";

// Initialization of oled display class
static PocketmageOled pm_oled(u8g2);

// 256x32 SPI OLED display object
U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2(U8G2_R2, OLED_CS, OLED_DC, OLED_RST);

struct FontSlot { FontStyle style; int yBias; };

static void drawKeyboardModifier(bool centered) {
  int state = KB().getKeyboardState();
  if (state < 1 || state > 3) return;
  const uint16_t dw = u8g2.getDisplayWidth();
  const uint16_t dh = u8g2.getDisplayHeight();
  static const char* labels[] = { "SHIFT", "FN", "FN+SHIFT" };
  const char* label = labels[state - 1];
  int tw = FontEngine::textWidth(DisplayTarget::OLED, label, FontStyle::Tiny);
  FontEngine::drawText(DisplayTarget::OLED, centered ? (dw - tw) / 2 : dw - tw, dh, label, FontStyle::Tiny);
}

static FontStyle pickFont(const String& text, int maxWidth, const FontSlot* table, int count, int& y, int& x) {
  const uint16_t dw = u8g2.getDisplayWidth();
  for (int i = 0; i < count; i++) {
    if (FontEngine::textWidth(DisplayTarget::OLED, text, table[i].style) < maxWidth) {
      y = 16 + table[i].yBias;
      x = (dw - FontEngine::textWidth(DisplayTarget::OLED, text, table[i].style)) / 2;
      return table[i].style;
    }
  }
  y = 16 + table[count - 1].yBias;
  x = dw - FontEngine::textWidth(DisplayTarget::OLED, text, table[count - 1].style);
  return table[count - 1].style;
}

// Setup for Oled Class
void setupOled() {
  u8g2.begin();
  u8g2.setBusClock(10000000);
  u8g2.setPowerSave(0);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

// oled object reference for other apps
PocketmageOled& OLED() { return pm_oled; }

// ===================== public functions =====================
void PocketmageOled::oledWord(String word, bool allowLarge, bool showInfo, String bottomText) {
  u8g2_.clearBuffer();
  const uint16_t dw = u8g2_.getDisplayWidth();
  const uint16_t dh = u8g2_.getDisplayHeight();

  if (showInfo && bottomText == "") infoBar();
  else if (bottomText != "") {
    FontEngine::drawText(DisplayTarget::OLED, (dw - FontEngine::textWidth(DisplayTarget::OLED, bottomText, FontStyle::Tiny)) / 2, dh, bottomText, FontStyle::Tiny);
  }

  if (allowLarge) {
    if (FontEngine::textWidth(DisplayTarget::OLED, word, FontStyle::Heading2) < dw) {
      FontEngine::drawText(DisplayTarget::OLED, (dw - FontEngine::textWidth(DisplayTarget::OLED, word, FontStyle::Heading2)) / 2, 16 + 5, word, FontStyle::Heading2);
      u8g2_.sendBuffer();
      return;
    }
  }

  static const FontSlot cascade[] = {
    {FontStyle::OledWord, 3},
    {FontStyle::Heading3, 2},
    {FontStyle::BodyBold, 1},
    {FontStyle::Status,   0},
  };

  int y = 16, x = 0;
  FontStyle s = pickFont(word, dw, cascade, 4, y, x);
  FontEngine::drawText(DisplayTarget::OLED, x, y, word, s);
  u8g2_.sendBuffer();
}

void PocketmageOled::sysMessage(String msg, int showTime) {
  pocketmage::setCpuSpeed(240);

  u8g2_.clearBuffer();
  const uint16_t dw = u8g2_.getDisplayWidth();
  const uint16_t dh = u8g2_.getDisplayHeight();

  static const FontSlot cascade[] = {
    {FontStyle::Heading2, 3 + 5},
    {FontStyle::OledWord, 2 + 5},
    {FontStyle::BodyBold, 1 + 5},
    {FontStyle::Status,   5},
  };

  int y_offset = 16, x_offset = 0;
  FontStyle s = pickFont(msg, dw - 8, cascade, 4, y_offset, x_offset);

  // --- Raise message animation ---
  for (int y = dh; y > 0; y -= 2) {
    u8g2_.clearBuffer();
    FontEngine::drawText(DisplayTarget::OLED, x_offset, y + y_offset, msg, s);
    u8g2_.drawRFrame(0, y, dw, dh + 16, 10);
    u8g2_.sendBuffer();
    delay(5);
  }

  // --- Hold ---
  vTaskDelay(pdMS_TO_TICKS(showTime));

  // --- Lower message animation ---
  for (int y = 0; y <= dh; y += 2) {
    u8g2_.clearBuffer();
    FontEngine::drawText(DisplayTarget::OLED, x_offset, y + y_offset, msg, s);
    u8g2_.drawRFrame(0, y, dw, dh + 16, 10);
    u8g2_.sendBuffer();
    delay(5);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void PocketmageOled::oledLine(String line, int input_pos, bool doProgressBar, String bottomMsg) {
  u8g2_.setDrawColor(1);
  const uint16_t dw = u8g2_.getDisplayWidth();
  const uint16_t dh = u8g2_.getDisplayHeight();

  String left = "";
  u8g2_.clearBuffer();

  //PROGRESS BAR
  if (doProgressBar && line.length() > 0) {
    const uint16_t charWidth = FontEngine::textWidth(DisplayTarget::OLED, line, FontStyle::Body);

    const uint16_t progress = map(charWidth, 0, display.width() - 5, 0, dw);

    u8g2_.drawVLine(dw - 1, 0, 2);
    u8g2_.drawVLine(0, 0, 2);

    u8g2_.drawHLine(0, 0, progress);
    u8g2_.drawHLine(0, 1, progress);

    if (charWidth > ((display.width() - 5) * 0.8)) {
      if ((millis() / 400) % 2 == 0) {
        u8g2_.drawVLine(dw - 1, 8, 32 - 16);
        u8g2_.drawLine(dw - 1, 15, dw - 4, 12);
        u8g2_.drawLine(dw - 1, 15, dw - 4, 18);
      }
    }
  }

  // No bottom msg, show infobar
  if (bottomMsg.length() == 0) {
    infoBar();
  }
  // Display bottomMsg
  else {
    FontEngine::drawText(DisplayTarget::OLED, 0, dh, bottomMsg, FontStyle::Tiny);

    drawKeyboardModifier(false);
  }

  // DRAW LINE TEXT
  int lineWidth = FontEngine::textWidth(DisplayTarget::OLED, line, FontStyle::Heading2);

  if (lineWidth < (dw - 5)) {
    if (line.length() > 0) {
      if (input_pos == 0) {
        FontEngine::drawText(DisplayTarget::OLED, 0, 20, line, FontStyle::Heading2);
        u8g2_.drawVLine(0, 1, 22);
      } else if (input_pos == line.length()) {
        FontEngine::drawText(DisplayTarget::OLED, 0, 20, line, FontStyle::Heading2);
        u8g2_.drawVLine(lineWidth + 2, 1, 22);
      } else {
        left = line.substring(0, input_pos);
        FontEngine::drawText(DisplayTarget::OLED, 0, 20, line, FontStyle::Heading2);
        u8g2_.drawVLine(FontEngine::textWidth(DisplayTarget::OLED, left, FontStyle::Heading2), 1, 22);
      }
    } else {
      FontEngine::drawText(DisplayTarget::OLED, 0, 20, line, FontStyle::Heading2);
      u8g2_.drawVLine(0, 1, 22);
    }
  } else {
    if (input_pos == 0) {
      FontEngine::drawText(DisplayTarget::OLED, 0, 20, line, FontStyle::Heading2);
      u8g2_.drawVLine(0, 1, 22);
    } else if (input_pos == line.length()) {
      FontEngine::drawText(DisplayTarget::OLED, dw - 8 - lineWidth, 20, line, FontStyle::Heading2);
      u8g2_.drawVLine(dw - 6, 1, 22);
    } else {
      left = line.substring(0, input_pos);
      int cursor_offset = FontEngine::textWidth(DisplayTarget::OLED, left, FontStyle::Heading2);
      int line_start = 0;

      if (cursor_offset > (dw - 8) / 2) {
        line_start += ((dw - 8) / 2) - cursor_offset;
        if (line_start + lineWidth < dw - 8) {
          line_start += dw - 8 - (line_start + lineWidth);
        }
        cursor_offset += line_start;
      }
      FontEngine::drawText(DisplayTarget::OLED, line_start, 20, line, FontStyle::Heading2);
      u8g2_.drawVLine(cursor_offset, 1, 22);
    }
  }

  u8g2_.sendBuffer();
}

void PocketmageOled::infoBar() {
  const uint16_t dw = u8g2_.getDisplayWidth();
  const uint16_t dh = u8g2_.getDisplayHeight();

  drawKeyboardModifier(true);

  int infoWidth = 16;

  // Battery Indicator
  int maxIconIndex = sizeof(batt_allArray) / sizeof(batt_allArray[0]) - 1;
  int state_ = battState;
  state_ = (int)constrain(state_, 0, maxIconIndex);
  u8g2_.drawXBMP(0, dh - 6, 10, 6, batt_allArray[state_]);

  // CLOCK
  if (SYSTEM_CLOCK) {
    DateTime now = CLOCK().nowDT();

    String timeString = String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute());
    FontEngine::drawText(DisplayTarget::OLED, infoWidth, dh, timeString, FontStyle::Tiny);

    String day3Char = String(daysOfTheWeek[now.dayOfTheWeek()]);
    day3Char = day3Char.substring(0, 3);
    if (SHOW_YEAR) day3Char += (" " + String(now.month()) + "/" + String(now.day()) + "/" + String(now.year()).substring(2, 4));
    else           day3Char += (" " + String(now.month()) + "/" + String(now.day()));
    FontEngine::drawText(DisplayTarget::OLED, dw - FontEngine::textWidth(DisplayTarget::OLED, day3Char, FontStyle::Tiny), dh, day3Char, FontStyle::Tiny);

    infoWidth += (FontEngine::textWidth(DisplayTarget::OLED, timeString, FontStyle::Tiny) + 6);
  }

  // MSC Indicator
  if (mscEnabled) {
    FontEngine::drawText(DisplayTarget::OLED, infoWidth, dh, "USB", FontStyle::Tiny);
    infoWidth += (FontEngine::textWidth(DisplayTarget::OLED, "USB", FontStyle::Tiny) + 6);
  }

  // Sink Indicator
  if (sinkEnabled) {
    FontEngine::drawText(DisplayTarget::OLED, infoWidth, dh, "SNK", FontStyle::Tiny);
    infoWidth += (FontEngine::textWidth(DisplayTarget::OLED, "SNK", FontStyle::Tiny) + 6);
  }

  // SD Indicator
  if (SDActive) {
    FontEngine::drawText(DisplayTarget::OLED, infoWidth, dh, "SD", FontStyle::Tiny);
    infoWidth += (FontEngine::textWidth(DisplayTarget::OLED, "SD", FontStyle::Tiny) + 6);
  }
}

void PocketmageOled::oledScroll() {
  // CLEAR DISPLAY
  u8g2_.clearBuffer();

  // DRAW BACKGROUND
  if (scrolloled0) u8g2_.drawXBMP(0, 0, 128, 32, scrolloled0);

  // DRAW LINES PREVIEW
  const long count = allLines.size();
  const long startIndex = max((long)(count - TOUCH().getDynamicScroll()), 0L);
  const long endIndex   = max((long)(count - TOUCH().getDynamicScroll() - 9), 0L);

  // CHECK IF LINE STARTS WITH A TAB
  for (long i = startIndex; i > endIndex && i >= 0; --i) {
    if (i >= count) continue;

    const bool tabbed = (allLines)[i].startsWith("    ");
    const String& s   = tabbed ? (allLines)[i].substring(4) : (allLines)[i];

    // Measure on OLED (not E-Ink) with a consistent tiny font for bar proportionality
    const uint16_t w  = FontEngine::textWidth(DisplayTarget::OLED, s, FontStyle::Tiny);

    const int refMax  = tabbed ? 49 : 56;
    const int lineW   = constrain(map((int)w, 0, display.width(), 0, refMax), 0, refMax);
    const int y       = 28 - (4 * (startIndex - i));
    const int x       = tabbed ? 68 : 61;

    u8g2_.drawBox(x, y, lineW, 2);
  }

  // PRINT CURRENT LINE — using _tf (full Unicode) fonts throughout
  String lineNumStr = String(startIndex) + "/" + String(count);
  FontEngine::drawText(DisplayTarget::OLED, 0, 12, "Line:", FontStyle::Status);
  FontEngine::drawText(DisplayTarget::OLED, 0, 24, lineNumStr, FontStyle::Status);

  // PRINT LINE PREVIEW
  if (startIndex >= 0 && (size_t)startIndex < allLines.size()) {
    const String& line = (allLines)[startIndex];
    if (line.length() > 0) {
      FontEngine::drawText(DisplayTarget::OLED, 140, 24, line, FontStyle::Heading2);
    }
  }

  // SEND BUFFER
  u8g2_.sendBuffer();
}

void PocketmageOled::setPowerSave(bool enable) {
  OLEDPowerSave_ = enable;
  u8g2_.setPowerSave(enable ? 1 : 0);
}

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
  static const StringID labels[] = { STR_KB_SHIFT, STR_KB_FN, STR_KB_FN_SHIFT };
  const char* label = TR(labels[state - 1]);
  int tw = FontEngine::textWidth(DisplayTarget::OLED, label, FontStyle::Tiny);
  FontEngine::drawText(DisplayTarget::OLED, centered ? (dw - tw) / 2 : dw - tw, kOledInfoBaseline, label, FontStyle::Tiny);
}

static FontStyle pickFont(const String& text, int maxWidth, const FontSlot* table, int count, int& y, int& x) {
  const uint16_t dw = u8g2.getDisplayWidth();
  for (int i = 0; i < count; i++) {
    if (FontEngine::textWidth(DisplayTarget::OLED, text, table[i].style) < maxWidth) {
      y = kOledWordBaseline + table[i].yBias;
      x = (dw - FontEngine::textWidth(DisplayTarget::OLED, text, table[i].style)) / 2;
      return table[i].style;
    }
  }
  y = kOledWordBaseline + table[count - 1].yBias;
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

  if (showInfo && bottomText == "") infoBar();
  else if (bottomText != "") {
    FontEngine::drawText(DisplayTarget::OLED, (dw - FontEngine::textWidth(DisplayTarget::OLED, bottomText, FontStyle::Tiny)) / 2, kOledInfoBaseline, bottomText, FontStyle::Tiny);
  }

  if (allowLarge) {
    if (FontEngine::textWidth(DisplayTarget::OLED, word, FontStyle::Heading2) < dw) {
      FontEngine::drawText(DisplayTarget::OLED, (dw - FontEngine::textWidth(DisplayTarget::OLED, word, FontStyle::Heading2)) / 2, kOledWordLargeBaseline, word, FontStyle::Heading2);
      u8g2_.sendBuffer();
      return;
    }
  }

  static const FontSlot cascade[] = {
    {FontStyle::OledWord, 3},
    {FontStyle::Heading3, 2},
    {FontStyle::BodyBold, 1},
    {FontStyle::Caption,   0},
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
    {FontStyle::Heading2, 3 + kOledSysMsgRaise},
    {FontStyle::OledWord, 2 + kOledSysMsgRaise},
    {FontStyle::BodyBold, 1 + kOledSysMsgRaise},
    {FontStyle::Caption,   kOledSysMsgRaise},
  };

  int y_offset = kOledWordBaseline, x_offset = 0;
  FontStyle s = pickFont(msg, dw - kOledSysMsgPadX, cascade, 4, y_offset, x_offset);

  // --- Raise message animation ---
  for (int y = dh; y > 0; y -= 2) {
    u8g2_.clearBuffer();
    FontEngine::drawText(DisplayTarget::OLED, x_offset, y + y_offset, msg, s);
    u8g2_.drawRFrame(0, y, dw, kOledSysMsgFrameH, kOledSysMsgRadius);
    u8g2_.sendBuffer();
    delay(5);
  }

  // --- Hold ---
  vTaskDelay(pdMS_TO_TICKS(showTime));

  // --- Lower message animation ---
  for (int y = 0; y <= dh; y += 2) {
    u8g2_.clearBuffer();
    FontEngine::drawText(DisplayTarget::OLED, x_offset, y + y_offset, msg, s);
    u8g2_.drawRFrame(0, y, dw, kOledSysMsgFrameH, kOledSysMsgRadius);
    u8g2_.sendBuffer();
    delay(5);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void PocketmageOled::oledLine(String line, int input_pos, bool doProgressBar, String bottomMsg, bool deferSend) {
  u8g2_.setDrawColor(1);
  const uint16_t dw = u8g2_.getDisplayWidth();

  String left = "";
  u8g2_.clearBuffer();

  //PROGRESS BAR
  if (doProgressBar && line.length() > 0) {
    const uint16_t charWidth = FontEngine::textWidth(DisplayTarget::OLED, line, FontStyle::Body);

    const uint16_t progress = map(charWidth, 0, kOledProgressMaxW, 0, dw);

    u8g2_.drawVLine(dw - 1, 0, 2);
    u8g2_.drawVLine(0, 0, 2);

    u8g2_.drawHLine(0, 0, progress);
    u8g2_.drawHLine(0, 1, progress);

    if (charWidth > (kOledProgressMaxW * kOledProgressFullFrac)) {
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
    FontEngine::drawText(DisplayTarget::OLED, kOledEditCursorX, kOledInfoBaseline, bottomMsg, FontStyle::Tiny);

    drawKeyboardModifier(false);
  }

  // DRAW LINE TEXT
  int lineWidth = FontEngine::textWidth(DisplayTarget::OLED, line, FontStyle::Heading2);

  if (lineWidth < (dw - 5)) {
    if (line.length() > 0) {
      if (input_pos == 0) {
        FontEngine::drawText(DisplayTarget::OLED, 0, kOledEditBaseline, line, FontStyle::Heading2);
        u8g2_.drawVLine(kOledEditCursorX, kOledEditCursorY, kOledEditCursorH);
      } else if (input_pos == line.length()) {
        FontEngine::drawText(DisplayTarget::OLED, 0, kOledEditBaseline, line, FontStyle::Heading2);
        u8g2_.drawVLine(lineWidth + 2, kOledEditCursorY, kOledEditCursorH);
      } else {
        left = line.substring(0, input_pos);
        FontEngine::drawText(DisplayTarget::OLED, 0, kOledEditBaseline, line, FontStyle::Heading2);
        u8g2_.drawVLine(FontEngine::textWidth(DisplayTarget::OLED, left, FontStyle::Heading2), kOledEditCursorY, kOledEditCursorH);
      }
    } else {
      FontEngine::drawText(DisplayTarget::OLED, 0, kOledEditBaseline, line, FontStyle::Heading2);
      u8g2_.drawVLine(kOledEditCursorX, kOledEditCursorY, kOledEditCursorH);
    }
  } else {
    if (input_pos == 0) {
      FontEngine::drawText(DisplayTarget::OLED, 0, kOledEditBaseline, line, FontStyle::Heading2);
      u8g2_.drawVLine(kOledEditCursorX, kOledEditCursorY, kOledEditCursorH);
    } else if (input_pos == line.length()) {
      FontEngine::drawText(DisplayTarget::OLED, dw - kOledEditRightPad - lineWidth, kOledEditBaseline, line, FontStyle::Heading2);
      u8g2_.drawVLine(dw - 6, kOledEditCursorY, kOledEditCursorH);
    } else {
      left = line.substring(0, input_pos);
      int cursor_offset = FontEngine::textWidth(DisplayTarget::OLED, left, FontStyle::Heading2);
      int line_start = 0;

      if (cursor_offset > (dw - kOledEditRightPad) / 2) {
        line_start += ((dw - kOledEditRightPad) / 2) - cursor_offset;
        if (line_start + lineWidth < dw - kOledEditRightPad) {
          line_start += dw - kOledEditRightPad - (line_start + lineWidth);
        }
        cursor_offset += line_start;
      }
      FontEngine::drawText(DisplayTarget::OLED, line_start, kOledEditBaseline, line, FontStyle::Heading2);
      u8g2_.drawVLine(cursor_offset, kOledEditCursorY, kOledEditCursorH);
    }
  }

  if (!deferSend) u8g2_.sendBuffer();
}

void PocketmageOled::infoBar() {
  const uint16_t dw = u8g2_.getDisplayWidth();

  drawKeyboardModifier(true);

  int infoWidth = kOledInfoFirstX;

  // Battery Indicator
  int maxIconIndex = sizeof(batt_allArray) / sizeof(batt_allArray[0]) - 1;
  int state_ = battState;
  state_ = (int)constrain(state_, 0, maxIconIndex);
  u8g2_.drawXBMP(kOledEditCursorX, kOledInfoBatteryY, kOledInfoBatteryW, kOledInfoBatteryH, batt_allArray[state_]);

  // CLOCK
  if (SYSTEM_CLOCK) {
    DateTime now = CLOCK().nowDT();

    String timeString = String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute());
    FontEngine::drawText(DisplayTarget::OLED, infoWidth, kOledInfoBaseline, timeString, FontStyle::Tiny);

    String day3Char = String(I18n::dayName(now.dayOfTheWeek()));
    day3Char = day3Char.substring(0, 3);
    if (SHOW_YEAR) day3Char += (" " + String(now.month()) + "/" + String(now.day()) + "/" + String(now.year()).substring(2, 4));
    else           day3Char += (" " + String(now.month()) + "/" + String(now.day()));
    FontEngine::drawText(DisplayTarget::OLED, dw - FontEngine::textWidth(DisplayTarget::OLED, day3Char, FontStyle::Tiny), kOledInfoBaseline, day3Char, FontStyle::Tiny);

    infoWidth += (FontEngine::textWidth(DisplayTarget::OLED, timeString, FontStyle::Tiny) + kOledInfoGap);
  }

  // MSC Indicator
  if (mscEnabled) {
    FontEngine::drawText(DisplayTarget::OLED, infoWidth, kOledInfoBaseline, TR(STR_INFO_USB), FontStyle::Tiny);
    infoWidth += (FontEngine::textWidth(DisplayTarget::OLED, TR(STR_INFO_USB), FontStyle::Tiny) + kOledInfoGap);
  }

  // Sink Indicator
  if (sinkEnabled) {
    FontEngine::drawText(DisplayTarget::OLED, infoWidth, kOledInfoBaseline, TR(STR_INFO_SNK), FontStyle::Tiny);
    infoWidth += (FontEngine::textWidth(DisplayTarget::OLED, TR(STR_INFO_SNK), FontStyle::Tiny) + kOledInfoGap);
  }

  // SD Indicator
  if (SDActive) {
    FontEngine::drawText(DisplayTarget::OLED, infoWidth, kOledInfoBaseline, TR(STR_INFO_SD), FontStyle::Tiny);
    infoWidth += (FontEngine::textWidth(DisplayTarget::OLED, TR(STR_INFO_SD), FontStyle::Tiny) + kOledInfoGap);
  }
}

void PocketmageOled::oledScroll() {
  // CLEAR DISPLAY
  u8g2_.clearBuffer();

  // DRAW BACKGROUND
  if (scrolloled0) u8g2_.drawXBMP(0, 0, kOledScrollPreviewW, kOledHeight, scrolloled0);

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

    const int refMax  = tabbed ? kOledScrollTabMaxW : kOledScrollNormMaxW;
    const int lineW   = constrain(map((int)w, 0, kEinkWidth, 0, refMax), 0, refMax);
    const int y       = kOledScrollBaseY - (kOledScrollRowPitch * (startIndex - i));
    const int x       = tabbed ? kOledScrollTabX : kOledScrollNormX;

    u8g2_.drawBox(x, y, lineW, kOledScrollBarH);
  }

  // PRINT CURRENT LINE — using _tf (full Unicode) fonts throughout
  String lineNumStr = String(startIndex) + "/" + String(count);
  FontEngine::drawText(DisplayTarget::OLED, 0, kOledScrollLabelY, TR(STR_OLED_LINE), FontStyle::Caption);
  FontEngine::drawText(DisplayTarget::OLED, 0, kOledScrollValueY, lineNumStr, FontStyle::Caption);

  // PRINT LINE PREVIEW
  if (startIndex >= 0 && (size_t)startIndex < allLines.size()) {
    const String& line = (allLines)[startIndex];
    if (line.length() > 0) {
      FontEngine::drawText(DisplayTarget::OLED, kOledScrollTextX, kOledScrollValueY, line, FontStyle::Heading2);
    }
  }

  // SEND BUFFER
  u8g2_.sendBuffer();
}

void PocketmageOled::setPowerSave(bool enable) {
  OLEDPowerSave_ = enable;
  u8g2_.setPowerSave(enable ? 1 : 0);
}

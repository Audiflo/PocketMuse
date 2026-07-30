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
    FontEngine::setOledStyle(FontStyle::Tiny);
    FontEngine::oledDraw((dw - FontEngine::oledTextWidth(bottomText)) / 2, dh, bottomText);
  }

  if (allowLarge) {
    FontEngine::setOledStyle(FontStyle::Heading2);
    if (FontEngine::oledTextWidth(word) < dw) {
      FontEngine::oledDraw((dw - FontEngine::oledTextWidth(word)) / 2, 16 + 5, word);
      u8g2_.sendBuffer();
      return;
    }
  }

  FontEngine::setOledStyle(FontStyle::OledWord);
  if (FontEngine::oledTextWidth(word) < dw) {
    FontEngine::oledDraw((dw - FontEngine::oledTextWidth(word)) / 2, 16 + 3, word);
    u8g2_.sendBuffer();
    return;
  }

  FontEngine::setOledStyle(FontStyle::Heading3);
  if (FontEngine::oledTextWidth(word) < dw) {
    FontEngine::oledDraw((dw - FontEngine::oledTextWidth(word)) / 2, 16 + 2, word);
    u8g2_.sendBuffer();
    return;
  }

  FontEngine::setOledStyle(FontStyle::BodyBold);
  if (FontEngine::oledTextWidth(word) < dw) {
    FontEngine::oledDraw((dw - FontEngine::oledTextWidth(word)) / 2, 16 + 1, word);
    u8g2_.sendBuffer();
    return;
  }

  FontEngine::setOledStyle(FontStyle::Status);
  if (FontEngine::oledTextWidth(word) < dw) {
    FontEngine::oledDraw((dw - FontEngine::oledTextWidth(word)) / 2, 16, word);
    u8g2_.sendBuffer();
    return;
  } else {
    FontEngine::oledDraw(dw - FontEngine::oledTextWidth(word), 16, word);
    u8g2_.sendBuffer();
    return;
  }
}

void PocketmageOled::sysMessage(String msg, int showTime) {
  pocketmage::setCpuSpeed(240);

  u8g2_.clearBuffer();
  const uint16_t dw = u8g2_.getDisplayWidth();
  const uint16_t dh = u8g2_.getDisplayHeight();

  int y_offset = 0;
  int x_offset = 0;

  // --- 1. Find the largest font that fits and calculate offsets ---
  FontEngine::setOledStyle(FontStyle::Heading2);
  if (FontEngine::oledTextWidth(msg) < dw - 8) {
    y_offset = 16 + 3 + 5;
    x_offset = (dw - FontEngine::oledTextWidth(msg)) / 2;
  } else {
    FontEngine::setOledStyle(FontStyle::OledWord);
    if (FontEngine::oledTextWidth(msg) < dw - 8) {
      y_offset = 16 + 2 + 5;
      x_offset = (dw - FontEngine::oledTextWidth(msg)) / 2;
    } else {
      FontEngine::setOledStyle(FontStyle::BodyBold);
      if (FontEngine::oledTextWidth(msg) < dw - 8) {
        y_offset = 16 + 1 + 5;
        x_offset = (dw - FontEngine::oledTextWidth(msg)) / 2;
      } else {
        FontEngine::setOledStyle(FontStyle::Status);
        if (FontEngine::oledTextWidth(msg) < dw - 8) {
          y_offset = 16 + 5;
          x_offset = (dw - FontEngine::oledTextWidth(msg)) / 2;
        } else {
          y_offset = 16 + 5;
          x_offset = dw - FontEngine::oledTextWidth(msg);
        }
      }
    }
  }

  // --- 2. Raise message animation ---
  for (int y = dh; y > 0; y -= 2) {
    u8g2_.clearBuffer();
    FontEngine::oledDraw(x_offset, y + y_offset, msg);
    u8g2_.drawRFrame(0, y, dw, dh + 16, 10);
    u8g2_.sendBuffer();
    delay(5);
  }

  // --- 3. Hold ---
  vTaskDelay(pdMS_TO_TICKS(showTime));

  // --- 4. Lower message animation ---
  for (int y = 0; y <= dh; y += 2) {
    u8g2_.clearBuffer();
    FontEngine::oledDraw(x_offset, y + y_offset, msg);
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
    FontEngine::setOledStyle(FontStyle::Body);
    const uint16_t charWidth = FontEngine::oledTextWidth(line);

    const uint8_t progress = map(charWidth, 0, display.width() - 5, 0, dw);

    u8g2_.drawVLine(dw, 0, 2);
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
    FontEngine::setOledStyle(FontStyle::Tiny);
    FontEngine::oledDraw(0, dh, bottomMsg);

    // Draw FN/Shift indicator
    int state = KB().getKeyboardState();
    switch (state) {
      case 1:
        FontEngine::setOledStyle(FontStyle::Tiny);
        FontEngine::oledDraw((dw - FontEngine::oledTextWidth("SHIFT")), dh, "SHIFT");
        break;
      case 2:
        FontEngine::setOledStyle(FontStyle::Tiny);
        FontEngine::oledDraw((dw - FontEngine::oledTextWidth("FN")), dh, "FN");
        break;
      case 3:
        FontEngine::setOledStyle(FontStyle::Tiny);
        FontEngine::oledDraw((dw - FontEngine::oledTextWidth("FN+SHIFT")), dh, "FN+SHIFT");
        break;
      default:
        break;
    }
  }

  // DRAW LINE TEXT
  FontEngine::setOledStyle(FontStyle::Heading2);
  int lineWidth = FontEngine::oledTextWidth(line);

  if (lineWidth < (dw - 5)) {
    if (line.length() > 0) {
      if (input_pos == 0) {
        FontEngine::oledDraw(0, 20, line);
        u8g2_.drawVLine(0, 1, 22);
      } else if (input_pos == line.length()) {
        FontEngine::oledDraw(0, 20, line);
        u8g2_.drawVLine(lineWidth + 2, 1, 22);
      } else {
        left = line.substring(0, input_pos);
        FontEngine::oledDraw(0, 20, line);
        u8g2_.drawVLine(FontEngine::oledTextWidth(left), 1, 22);
      }
    } else {
      FontEngine::oledDraw(0, 20, line);
      u8g2_.drawVLine(0, 1, 22);
    }
  } else {
    if (input_pos == 0) {
      FontEngine::oledDraw(0, 20, line);
      u8g2_.drawVLine(0, 1, 22);
    } else if (input_pos == line.length()) {
      FontEngine::oledDraw(dw - 8 - lineWidth, 20, line);
      u8g2_.drawVLine(dw - 6, 1, 22);
    } else {
      left = line.substring(0, input_pos);
      int cursor_offset = FontEngine::oledTextWidth(left);
      int line_start = 0;

      if (cursor_offset > (dw - 8) / 2) {
        line_start += ((dw - 8) / 2) - cursor_offset;
        if (line_start + lineWidth < dw - 8) {
          line_start += dw - 8 - (line_start + lineWidth);
        }
        cursor_offset += line_start;
      }
      FontEngine::oledDraw(line_start, 20, line);
      u8g2_.drawVLine(cursor_offset, 1, 22);
    }
  }

  u8g2_.sendBuffer();
}

void PocketmageOled::infoBar() {
  const uint16_t dw = u8g2_.getDisplayWidth();
  const uint16_t dh = u8g2_.getDisplayHeight();

  FontEngine::setOledStyle(FontStyle::Tiny);
  int state = KB().getKeyboardState();

  switch (state) {
    case 1:
    FontEngine::oledDraw((dw - FontEngine::oledTextWidth("SHIFT")) / 2, dh, "SHIFT");
    break;
    case 2:
    FontEngine::oledDraw((dw - FontEngine::oledTextWidth("FN")) / 2, dh, "FN");
    break;
    case 3:
    FontEngine::oledDraw((dw - FontEngine::oledTextWidth("FN+SHIFT")) / 2, dh, "FN+SHIFT");
    break;
    default:
    break;
  }

  int infoWidth = 16;

  // Battery Indicator
  int maxIconIndex = sizeof(batt_allArray) / sizeof(batt_allArray[0]) - 1;
  int state_ = battState;
  state_ = (int)constrain(state_, 0, maxIconIndex);
  u8g2_.drawXBMP(0, dh - 6, 10, 6, batt_allArray[state_]);

  // CLOCK
  if (SYSTEM_CLOCK) {
    FontEngine::setOledStyle(FontStyle::Tiny);
    DateTime now = CLOCK().nowDT();

    String timeString = String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute());
    FontEngine::oledDraw(infoWidth, dh, timeString);

    String day3Char = String(daysOfTheWeek[now.dayOfTheWeek()]);
    day3Char = day3Char.substring(0, 3);
    if (SHOW_YEAR) day3Char += (" " + String(now.month()) + "/" + String(now.day()) + "/" + String(now.year()).substring(2, 4));
    else           day3Char += (" " + String(now.month()) + "/" + String(now.day()));
    FontEngine::oledDraw(dw - FontEngine::oledTextWidth(day3Char), dh, day3Char);

    infoWidth += (FontEngine::oledTextWidth(timeString) + 6);
  }

  // MSC Indicator
  if (mscEnabled) {
    FontEngine::setOledStyle(FontStyle::Tiny);
    FontEngine::oledDraw(infoWidth, dh, "USB");
    infoWidth += (FontEngine::oledTextWidth("USB") + 6);
  }

  // Sink Indicator
  if (sinkEnabled) {
    FontEngine::setOledStyle(FontStyle::Tiny);
    FontEngine::oledDraw(infoWidth, dh, "SNK");
    infoWidth += (FontEngine::oledTextWidth("SNK") + 6);
  }

  // SD Indicator
  if (SDActive) {
    FontEngine::setOledStyle(FontStyle::Tiny);
    FontEngine::oledDraw(infoWidth, dh, "SD");
    infoWidth += (FontEngine::oledTextWidth("SD") + 6);
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
    FontEngine::setOledStyle(FontStyle::Tiny);
    const uint16_t w  = FontEngine::oledTextWidth(s);

    const int refMax  = tabbed ? 49 : 56;
    const int lineW   = constrain(map((int)w, 0, display.width(), 0, refMax), 0, refMax);
    const int y       = 28 - (4 * (startIndex - i));
    const int x       = tabbed ? 68 : 61;

    u8g2_.drawBox(x, y, lineW, 2);
  }

  // PRINT CURRENT LINE — using _tf (full Unicode) fonts throughout
  FontEngine::setOledStyle(FontStyle::Status);
  String lineNumStr = String(startIndex) + "/" + String(count);
  FontEngine::oledDraw(0, 12, "Line:");
  FontEngine::oledDraw(0, 24, lineNumStr);

  // PRINT LINE PREVIEW
  if (startIndex >= 0 && (size_t)startIndex < allLines.size()) {
    const String& line = (allLines)[startIndex];
    if (line.length() > 0) {
      FontEngine::setOledStyle(FontStyle::Heading2);
      FontEngine::oledDraw(140, 24, line);
    }
  }

  // SEND BUFFER
  u8g2_.sendBuffer();
}

void PocketmageOled::setPowerSave(bool enable) {
  OLEDPowerSave_ = enable;
  u8g2_.setPowerSave(enable ? 1 : 0);
}

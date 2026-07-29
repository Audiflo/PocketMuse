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
    u8g2_.setFont(u8g2_font_5x7_tf);
    u8g2_.drawUTF8((dw - u8g2_.getUTF8Width(bottomText.c_str())) / 2, dh, bottomText.c_str());
  }

  // Changed all _tr fonts to _tf to include the extended UTF-8 character set
  if (allowLarge) {
    u8g2_.setFont(u8g2_font_ncenB18_tf);
    if (u8g2_.getUTF8Width(word.c_str()) < dw) {
      u8g2_.drawUTF8((dw - u8g2_.getUTF8Width(word.c_str()))/2, 16+5, word.c_str());
      u8g2_.sendBuffer();
      return;
    }
  }

  u8g2_.setFont(u8g2_font_ncenB14_tf);
  if (u8g2_.getUTF8Width(word.c_str()) < dw) {
    u8g2_.drawUTF8((dw - u8g2_.getUTF8Width(word.c_str()))/2, 16+3, word.c_str());
    u8g2_.sendBuffer();
    return;
  }

  u8g2_.setFont(u8g2_font_ncenB12_tf);
  if (u8g2_.getUTF8Width(word.c_str()) < dw) {
    u8g2_.drawUTF8((dw - u8g2_.getUTF8Width(word.c_str()))/2, 16+2, word.c_str());
    u8g2_.sendBuffer();
    return;
  }

  u8g2_.setFont(u8g2_font_ncenB10_tf);
  if (u8g2_.getUTF8Width(word.c_str()) < dw) {
    u8g2_.drawUTF8((dw - u8g2_.getUTF8Width(word.c_str()))/2, 16+1, word.c_str());
    u8g2_.sendBuffer();
    return;
  }

  u8g2_.setFont(u8g2_font_ncenB08_tf);
  if (u8g2_.getUTF8Width(word.c_str()) < dw) {
    u8g2_.drawUTF8((dw - u8g2_.getUTF8Width(word.c_str()))/2, 16, word.c_str());
    u8g2_.sendBuffer();
    return;
  } else {
    u8g2_.drawUTF8(dw - u8g2_.getUTF8Width(word.c_str()), 16, word.c_str());
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
  u8g2_.setFont(u8g2_font_ncenB14_tf);
  if (u8g2_.getUTF8Width(msg.c_str()) < dw-8) {
    y_offset = 16 + 3 + 5;
    x_offset = (dw - u8g2_.getUTF8Width(msg.c_str())) / 2;
  } 
  else {
    u8g2_.setFont(u8g2_font_ncenB12_tf);
    if (u8g2_.getUTF8Width(msg.c_str()) < dw-8) {
      y_offset = 16 + 2 + 5;
      x_offset = (dw - u8g2_.getUTF8Width(msg.c_str())) / 2;
    } 
    else {
      u8g2_.setFont(u8g2_font_ncenB10_tf);
      if (u8g2_.getUTF8Width(msg.c_str()) < dw-8) {
        y_offset = 16 + 1 + 5;
        x_offset = (dw - u8g2_.getUTF8Width(msg.c_str())) / 2;
      } 
      else {
        u8g2_.setFont(u8g2_font_ncenB08_tf);
        if (u8g2_.getUTF8Width(msg.c_str()) < dw-8) {
          y_offset = 16 + 5;
          x_offset = (dw - u8g2_.getUTF8Width(msg.c_str())) / 2;
        } 
        else {
          // Fallback: If it's still too long, align it to the right edge
          y_offset = 16 + 5;
          x_offset = dw - u8g2_.getUTF8Width(msg.c_str());
        }
      }
    }
  }

  // --- 2. Raise message animation ---
  for (int y = dh; y > 0; y-=2) {
    u8g2_.clearBuffer();
    u8g2_.drawUTF8(x_offset, y + y_offset, msg.c_str());
    u8g2_.drawRFrame(0, y, dw, dh + 16, 10);
    u8g2_.sendBuffer();
    delay(5);
  }

  // --- 3. Hold ---
  vTaskDelay(pdMS_TO_TICKS(showTime));

  // --- 4. Lower message animation ---
  for (int y = 0; y <= dh; y+=2) {
    u8g2_.clearBuffer();
    u8g2_.drawUTF8(x_offset, y + y_offset, msg.c_str());
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
    // Fixed string measurement
    const uint16_t charWidth = u8g2_.getUTF8Width(line.c_str());

    // Restored global display reference
    const uint8_t progress = map(charWidth, 0, display.width() - 5, 0, dw);

    u8g2_.drawVLine(dw, 0, 2);
    u8g2_.drawVLine(0, 0, 2);

    u8g2_.drawHLine(0, 0, progress);
    u8g2_.drawHLine(0, 1, progress);

    // LINE END WARNING INDICATOR
    if (charWidth > ((display.width() - 5) * 0.8)) {
      if ((millis() / 400) % 2 == 0) {  // ON for 200ms, OFF for 200ms
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
    u8g2_.setFont(u8g2_font_5x7_tf);
    u8g2_.drawUTF8(0, dh, bottomMsg.c_str());

    // Draw FN/Shift indicator (Standard ASCII is fine here)
    int state = KB().getKeyboardState();
    switch (state) {
      case 1: //SHIFT
        u8g2_.drawStr((dw - u8g2_.getStrWidth("SHIFT")), dh, "SHIFT");
        break;
      case 2: //FUNC
        u8g2_.drawStr((dw - u8g2_.getStrWidth("FN")), dh, "FN");
        break;
      case 3: //FN_SHIFT
        u8g2_.drawStr((dw - u8g2_.getStrWidth("FN+SHIFT")), dh, "FN+SHIFT");
        break;
      default:
        break;
    }
  }

  // DRAW LINE TEXT (Upgraded to full UTF-8 Font and Draw routines)
  u8g2_.setFont(u8g2_font_ncenB18_tf);
  int lineWidth = u8g2_.getUTF8Width(line.c_str());

  if (lineWidth < (dw - 5)) {
    if (line.length() > 0) {
      if (input_pos == 0) {
        u8g2_.drawUTF8(0, 20, line.c_str());
        u8g2_.drawVLine(0, 1, 22);
      } else if (input_pos == line.length()) {
        u8g2_.drawUTF8(0, 20, line.c_str());
        u8g2_.drawVLine(lineWidth + 2, 1, 22);
      } else {
        left = line.substring(0, input_pos);
        u8g2_.drawUTF8(0, 20, line.c_str());
        u8g2_.drawVLine(u8g2_.getUTF8Width(left.c_str()), 1, 22);
      }
    } else {
      u8g2_.drawUTF8(0, 20, line.c_str());
      u8g2_.drawVLine(0, 1, 22); // Still draw the cursor when string is empty
    }
  } else {
    if (input_pos == 0) {
      u8g2_.drawUTF8(0, 20, line.c_str());
      u8g2_.drawVLine(0, 1, 22);
    } else if (input_pos == line.length()) {
      //show end of line, input scrolls left
      u8g2_.drawUTF8(dw - 8 - lineWidth, 20, line.c_str());
      u8g2_.drawVLine(dw - 6, 1, 22);
    } else {
      //calc cursor pos using perfect UTF-8 string split math
      left = line.substring(0, input_pos);
      int cursor_offset = u8g2_.getUTF8Width(left.c_str());
      int line_start = 0;
      
      if (cursor_offset > (dw - 8) / 2) {
        //shift left
        line_start += ((dw - 8) / 2) - cursor_offset;
        if (line_start + lineWidth < dw - 8) {
          //shift back right
          line_start += dw - 8 - (line_start + lineWidth);
        }
        cursor_offset += line_start;
      }
      u8g2_.drawUTF8(line_start, 20, line.c_str());
      u8g2_.drawVLine(cursor_offset, 1, 22);
    }
  }

  u8g2_.sendBuffer();
}

void PocketmageOled::infoBar() {
  const uint16_t dw = u8g2_.getDisplayWidth();
  const uint16_t dh = u8g2_.getDisplayHeight();

  // FN/SHIFT indicator centered
  u8g2_.setFont(u8g2_font_5x7_tf);
  
  int state = KB().getKeyboardState();

  switch (state) {
    case 1:
    u8g2_.drawStr((dw - u8g2_.getStrWidth("SHIFT")) / 2, dh, "SHIFT");
    break;
    case 2:
    u8g2_.drawStr((dw - u8g2_.getStrWidth("FN")) / 2, dh, "FN");
    break;
    case 3:
    u8g2_.drawStr((dw - u8g2_.getStrWidth("FN+SHIFT")) / 2, dh, "FN+SHIFT");
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
    u8g2_.setFont(u8g2_font_5x7_tf);
    DateTime now = CLOCK().nowDT();
    
    // shortened time format
    String timeString = String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute());
    u8g2_.drawStr(infoWidth, dh, timeString.c_str());

    String day3Char = String(daysOfTheWeek[now.dayOfTheWeek()]);
    day3Char = day3Char.substring(0, 3);
    if (SHOW_YEAR) day3Char += (" " + String(now.month()) + "/" + String(now.day()) + "/" + String(now.year()).substring(2,4));
    else           day3Char += (" " + String(now.month()) + "/" + String(now.day()));
    u8g2_.drawStr(dw - u8g2_.getStrWidth(day3Char.c_str()), dh, day3Char.c_str());    

    infoWidth += (u8g2_.getStrWidth(timeString.c_str()) + 6);
  }

  // MSC Indicator
  if (mscEnabled) {
    u8g2_.setFont(u8g2_font_5x7_tf);
    u8g2_.drawStr(infoWidth, dh, "USB");

    infoWidth += (u8g2_.getStrWidth("USB") + 6);
  }

  // Sink Indicator
  if (sinkEnabled) {
    u8g2_.setFont(u8g2_font_5x7_tf);
    u8g2_.drawStr(infoWidth, dh, "SNK");

    infoWidth += (u8g2_.getStrWidth("SNK") + 6);
  }

  // SD Indicator 
  if (SDActive) {
    u8g2_.setFont(u8g2_font_5x7_tf);
    u8g2_.drawStr(infoWidth, dh, "SD");

    infoWidth += (u8g2_.getStrWidth("SD") + 6);
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
    if (i >= count) continue;  // Ensure i is within bounds

    // CHECK IF LINE STARTS WITH A TAB
    const bool tabbed = (allLines)[i].startsWith("    ");
    const String& s   = tabbed ? (allLines)[i].substring(4) : (allLines)[i];
    const uint16_t w  = strWidth(s);

    // ADJUST DRAW COORDINATES BASED ON TAB
    const int refMax  = tabbed ? 49 : 56;
    
    // Now dynamically referencing display.width() here as well
    const int lineW   = constrain(map((int)w, 0, display.width(), 0, refMax), 0, refMax); 
    const int y       = 28 - (4 * (startIndex - i));
    const int x       = tabbed ? 68 : 61;

    u8g2_.drawBox(x, y, lineW, 2);
  }

  // PRINT CURRENT LINE
  u8g2_.setFont(u8g2_font_ncenB08_tr);
  String lineNumStr = String(startIndex) + "/" + String(count);
  u8g2_.drawStr(0, 12, "Line:");
  u8g2_.drawStr(0, 24, lineNumStr.c_str());

  // PRINT LINE PREVIEW
  if (startIndex >= 0 && (size_t)startIndex < allLines.size()) {
    const String& line = (allLines)[startIndex];
    if (line.length() > 0) {
      u8g2_.setFont(u8g2_font_ncenB18_tr);
      u8g2_.drawStr(140, 24, line.c_str());
    }
  }

  // SEND BUFFER 
  u8g2_.sendBuffer();
}

void PocketmageOled::setPowerSave(bool enable) {
  OLEDPowerSave_ = enable;
  u8g2_.setPowerSave(enable ? 1 : 0);
}

// ===================== private functions =====================
// COMPUTE STRING WIDTH IN EINK PIXELS
uint16_t PocketmageOled::strWidth(const String& s) const {
  return EINK().getEinkTextWidth(s);
}
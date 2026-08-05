// AUDIT 1

#include <globals.h>
#include <vector>

#if !OTA_APP // POCKETMAGE_OS

enum class SetType { BOOLEAN, INTEGER, ACTION };

// Data-driven struct holding configuration for each setting
struct SettingItem {
  String key;              // Preference key name
  String name;             // Display name in UI
  SetType type;            // Boolean, Integer, or Action
  int* intVal;             // Pointer to global int variable
  bool* boolVal;           // Pointer to global bool variable
  int minVal;              // Min boundary for ints
  int maxVal;              // Max boundary for ints
  void (*onUpdate)();      // Callback executed after variable changes or for Action
};

// Callbacks for setting changes
void updateLumina() { u8g2.setContrast(OLED_BRIGHTNESS); }
void updateFastRef() { if (FAST_REFRESH) EINK().markPanelNeedsFullRefresh(); }
void updateBlank() {}

// Action Callbacks for specialized prompts
void actionSetTime() {
  int newTime = timePrompt();
  if (newTime >= 0) {
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", newTime / 100, newTime % 100);
    CLOCK().setTimeFromString(String(timeBuf));
    OLED().sysMessage(TR(STR_SETTINGS_TIME_UPDATED_TO) + String(timeBuf), 1000);
  }
}

void actionSetDate() {
  String newDate = datePrompt();
  if (newDate != "_EXIT_" && newDate != "_RETURN_" && newDate != "_CENTER_") {
    int day   = newDate.substring(0, 2).toInt();
    int month = newDate.substring(3, 5).toInt();
    int year  = newDate.substring(6, 10).toInt();
    DateTime now = CLOCK().nowDT();
    CLOCK().getRTC().adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
    OLED().sysMessage(TR(STR_SETTINGS_DATE_UPDATED), 1000);
  }
}

void actionSetPin() {
  String pin1 = textPrompt(TR(STR_LOCK_ENTER_PIN), "", true);
  if (pin1 == "_EXIT_" || pin1 == "_RETURN_" || pin1 == "_CENTER_") return;
  String pin2 = textPrompt(TR(STR_LOCK_CONFIRM_PIN), "", true);
  if (pin2 == "_EXIT_" || pin2 == "_RETURN_" || pin2 == "_CENTER_") return;
  if (!lockPinValid(pin1)) {
    OLED().sysMessage(TR(STR_LOCK_PIN_INVALID), 1000);
    return;
  }
  if (pin1 != pin2) {
    OLED().sysMessage(TR(STR_LOCK_MISMATCH), 1000);
    return;
  }
  lockSetPin(pin1);
  OLED().sysMessage(TR(STR_LOCK_PIN_SET), 1000);
}

void actionSetLang() {
  String langPart = textPrompt("Language code (en, es, etc):");
  if (langPart != "_EXIT_" && langPart != "_RETURN_" && langPart != "_CENTER_") {
    langPart.trim();
    langPart.toLowerCase();
    if (I18n::setLanguageByCode(langPart.c_str())) {
      prefs.begin("PocketMage", false);
      prefs.putInt("Language", static_cast<int>(I18n::language()));
      prefs.end();
      OLED().sysMessage(String(TR(STR_TERM_LANG_SET)) + I18n::nativeName(), 1000);
    } else {
      OLED().sysMessage(TR(STR_TERM_HELP_LANG), 1000);
    }
  }
}

// Core array defining the system settings
static std::vector<SettingItem> settingsList = {
  SettingItem{"OLED_BRIGHTNESS", "OLED Brightness", SetType::INTEGER, &OLED_BRIGHTNESS, nullptr, 0, 255, updateLumina},
  SettingItem{"TIMEOUT", "Screen Timeout", SetType::INTEGER, &TIMEOUT, nullptr, 15, 3600, updateBlank},
  SettingItem{"OLED_MAX_FPS", "OLED Max FPS", SetType::INTEGER, &OLED_MAX_FPS, nullptr, 5, 144, updateBlank},
  SettingItem{"MUTE_BUZZER", "Mute Buzzer", SetType::BOOLEAN, nullptr, &MUTE_BUZZER, 0, 0, updateBlank},
  SettingItem{"SYSTEM_CLOCK", "System Clock", SetType::BOOLEAN, nullptr, &SYSTEM_CLOCK, 0, 0, updateBlank},
  SettingItem{"SHOW_YEAR", "Show Year", SetType::BOOLEAN, nullptr, &SHOW_YEAR, 0, 0, updateBlank},
  SettingItem{"SAVE_POWER", "Save Power", SetType::BOOLEAN, nullptr, &SAVE_POWER, 0, 0, updateBlank},
  SettingItem{"FAST_REFRESH", "Fast Refresh (Experimental!)", SetType::BOOLEAN, nullptr, &FAST_REFRESH, 0, 0, updateFastRef},
  SettingItem{"DEBUG_VERBOSE", "Debug Verbose", SetType::BOOLEAN, nullptr, &DEBUG_VERBOSE, 0, 0, updateBlank},
  SettingItem{"HOME_ON_BOOT", "Home on Boot", SetType::BOOLEAN, nullptr, &HOME_ON_BOOT, 0, 0, updateBlank},
  SettingItem{"ALLOW_NO_SD", "Allow No SD", SetType::BOOLEAN, nullptr, &ALLOW_NO_MICROSD, 0, 0, updateBlank},
  SettingItem{"LOCK_ENABLED", "Device Lock", SetType::BOOLEAN, nullptr, (bool*)&deviceLocked, 0, 0, updateBlank},
  SettingItem{"", "Set Lock PIN", SetType::ACTION, nullptr, nullptr, 0, 0, actionSetPin},
  SettingItem{"", "Set Time", SetType::ACTION, nullptr, nullptr, 0, 0, actionSetTime},
  SettingItem{"", "Set Date", SetType::ACTION, nullptr, nullptr, 0, 0, actionSetDate},
  SettingItem{"", "System Language", SetType::ACTION, nullptr, nullptr, 0, 0, actionSetLang}
};

// Simplified state machine
enum SettingsState { SETTINGS_MAIN };
SettingsState CurrentSettingsState = SETTINGS_MAIN;

// State variables for UI Interaction
static ulong settingsScrollIndex = 0;

// Layout constants for the Settings UI Box
constexpr int SETTINGS_BOX_X      = 6;
constexpr int SETTINGS_BOX_Y      = 26;
constexpr int SETTINGS_BOX_W      = 206;
constexpr int SETTINGS_BOX_H      = 186;
constexpr int SETTINGS_TOG_W      = 26;
constexpr int SETTINGS_TOG_H      = 11;
constexpr int SETTINGS_ITEMS_PAGE = 9;
constexpr int SETTINGS_PITCH      = SETTINGS_BOX_H / SETTINGS_ITEMS_PAGE;

void SETTINGS_INIT() {
  // OPEN SETTINGS
  CurrentAppState = SETTINGS;
  CurrentSettingsState = SETTINGS_MAIN;
  KB().setKeyboardState(NORMAL);
  newState = true;
}

String settingCommandSelect(String command) {
  String returnText = "";
  command = I18n::normalizeCommand(command);
  command.toLowerCase();

  if (command.startsWith("timeset") || command.startsWith("settime")) {
    String timePart = "";
    int spaceIdx = command.indexOf(' ');
    if (spaceIdx != -1) {
      timePart = command.substring(spaceIdx + 1);
      timePart.trim();
    }
    if (timePart.length() == 0) {
      actionSetTime();
    } else if (timePart.length() >= 4) { 
      CLOCK().setTimeFromString(timePart);
      returnText = TR(STR_SETTINGS_TIME_UPDATED);
    } else {
      returnText = TR(STR_SETTINGS_INVALID_FMT_HHMM);
    }
    return returnText;
  }
  else if (command.startsWith("mute ")) {
    String muteArg = command.substring(5);
    muteArg.trim();
    
    if (muteArg == "on" || muteArg == "t") {
      MUTE_BUZZER = true;
    } else if (muteArg == "off" || muteArg == "f") {
      MUTE_BUZZER = false;
    } else {
      return TR(STR_INVALID);
    }
    
    prefs.begin("PocketMage", false);
    prefs.putBool("MUTE_BUZZER", MUTE_BUZZER);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command == "lock" || command.startsWith("lock ")) {
    String lockArg = "";
    int lockSpaceIdx = command.indexOf(' ');
    if (lockSpaceIdx != -1) {
      lockArg = command.substring(lockSpaceIdx + 1);
      lockArg.trim();
      lockArg.toLowerCase();
    }
    if (lockArg.length() == 0) {
      return lockIsEnabled() ? TR(STR_LOCK_ENABLED) : TR(STR_LOCK_DISABLED);
    }
    else if (lockArg == "on") {
      if (!lockHasPin()) {
        actionSetPin();
      } 
      if (lockHasPin()) {
        prefs.begin("PocketMage", false);
        prefs.putBool("LOCK_ENABLED", true);
        prefs.end();
        deviceLocked = true;
        newState = true;
        return TR(STR_LOCK_ENABLED);
      }
      return "";
    }
    else if (lockArg == "off") {
      lockDisable();
      newState = true;
      return TR(STR_LOCK_DISABLED);
    } else {
      return TR(STR_LOCK_HELP);
    }
  }
  else if (command.startsWith("lockpin ")) {
    String pin = command.substring(8);
    pin.trim();
    if (!lockPinValid(pin)) return TR(STR_LOCK_PIN_INVALID);
    lockSetPin(pin);
    newState = true;
    return TR(STR_LOCK_PIN_SET);
  }
  else if (command.startsWith("dateset") || command.startsWith("setdate")) {
    String datePart = "";
    int spaceIdx = command.indexOf(' ');
    if (spaceIdx != -1) {
      datePart = command.substring(spaceIdx + 1);
      datePart.trim();
    }
    if (datePart.length() == 0) {
      actionSetDate();
    } else if (datePart.length() == 8 && datePart.toInt() > 0) {
      int year  = datePart.substring(0, 4).toInt();
      int month = datePart.substring(4, 6).toInt();
      int day   = datePart.substring(6, 8).toInt();
      static const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
      int maxDay = dim[month - 1];
      if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) maxDay = 29;
      if (year < 1970 || year > 2200 || month < 1 || month > 12 || day < 1 || day > maxDay) {
        returnText = TR(STR_SETTINGS_INVALID_DATE);
      } else {
        DateTime now = CLOCK().nowDT();
        CLOCK().getRTC().adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
        returnText = TR(STR_SETTINGS_DATE_UPDATED);
      }
    } else {
      returnText = TR(STR_SETTINGS_INVALID_FMT_YYYYMMDD);
    }
    return returnText;
  }
  else if (command.startsWith("lang ")) {
    String langPart = command.substring(5);
    langPart.trim();
    langPart.toLowerCase();
    if (!I18n::setLanguageByCode(langPart.c_str())) return TR(STR_TERM_HELP_LANG);
    prefs.begin("PocketMage", false);
    prefs.putInt("Language", static_cast<int>(I18n::language()));
    prefs.end();
    newState = true;
    return String(TR(STR_TERM_LANG_SET)) + I18n::nativeName();
  }
  else {
    return TR(STR_SETTINGS_HUH);
  }
}

void processKB_SETTINGS() {
  pocketmage::setCpuSpeed(240);
  char inchar = KB().updateKeypress();
  if (inchar == 0) {
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }

  // Update hardware touchpad scroll mapped natively to Settings Array
  int scrollStep = (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) ? 5 : 1;
  if (TOUCH().updateScroll(settingsList.size() - 1, settingsScrollIndex, scrollStep)) {
    newState = true;
  }

  int currentMillis = millis();

  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
    if (inchar != 0) {
      KBBounceMillis = currentMillis;

      // Forceful App Exit (FN + <)
      if (inchar == 12) { 
        HOME_INIT();
        return;
      }
      // Enter or Center Click to change setting
      else if (inchar == 13 || inchar == 20) { 
        SettingItem& item = settingsList[settingsScrollIndex];
        
        if (item.type == SetType::BOOLEAN) {
          int res = boolPrompt("Toggle " + item.name + "?");
          if (res == 1) {
            if (item.key == "LOCK_ENABLED") {
              if (!lockHasPin()) {
                actionSetPin();
                if (!lockHasPin()) {
                  // User aborted PIN creation
                  KB().setKeyboardState(NORMAL);
                  newState = true;
                  return;
                }
              }
              *(item.boolVal) = true;
              deviceLocked = true;
            } else {
              *(item.boolVal) = true;
            }
          } 
          else if (res == 0) {
            if (item.key == "LOCK_ENABLED") {
              lockDisable();
              *(item.boolVal) = false;
              deviceLocked = false;
            } else {
              *(item.boolVal) = false;
            }
          }

          if (res == 1 || res == 0) {
            prefs.begin("PocketMage", false);
            prefs.putBool(item.key.c_str(), *(item.boolVal));
            prefs.end();
            if (item.onUpdate) item.onUpdate();
          }
        } 
        else if (item.type == SetType::INTEGER) {
          String promptTxt = item.name + " (" + String(item.minVal) + "-" + String(item.maxVal) + "):";
          String resStr = textPrompt(promptTxt);
          
          if (resStr != "_RETURN_" && resStr != "_EXIT_" && resStr != "_CENTER_") {
            int val = resStr.toInt();
            if (val >= item.minVal && val <= item.maxVal) {
              *(item.intVal) = val;
              
              prefs.begin("PocketMage", false);
              prefs.putInt(item.key.c_str(), *(item.intVal));
              prefs.end();
              if (item.onUpdate) item.onUpdate();
            } else {
              OLED().sysMessage(TR(STR_INVALID), 1000);
            }
          }
        }
        else if (item.type == SetType::ACTION) {
          if (item.onUpdate) item.onUpdate();
        }

        // Return gracefully and redraw menus
        KB().setKeyboardState(NORMAL);
        newState = true;
      }
      // Space to manually type a setting command
      else if (inchar == 32) { 
        String cmd = textPrompt("Type a setting command:", "> ");
        if (cmd != "_RETURN_" && cmd != "_EXIT_" && cmd != "_CENTER_" && cmd != "") {
          String returnText = settingCommandSelect(cmd);
          if (returnText != "") OLED().sysMessage(returnText, 1000);
        }
        KB().setKeyboardState(NORMAL);
        newState = true;
      }
    }
  }

  // OLED Real-time Update
  currentMillis = millis();
  if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = currentMillis;

    // OLED Setting Scroll Preview always displayed as the default view
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    
    int startLine = 0;
    if (settingsScrollIndex >= 1) startLine = settingsScrollIndex - 1;
    
    int y = kOledPrevY0;
    for (int i = startLine; i < startLine + kOledPrevRows; i++) {
      if (i >= settingsList.size()) break;
      
      // Draw Triangle Selector for the current Item
      if (i == settingsScrollIndex) {
        u8g2.drawTriangle(0, y - 2 * kOledPrevTriH, 0, y, kOledPrevTriW, y - kOledPrevTriH);
      }
      
      FontEngine::drawText(DisplayTarget::OLED, kOledPrevX, y, settingsList[i].name, FontStyle::Tiny);
      y += kOledPrevPitch;
    }

    // Draw instructions on the right side of the OLED
    FontEngine::drawText(DisplayTarget::OLED, 140, 12, "ENTER to change", FontStyle::Tiny);
    FontEngine::drawText(DisplayTarget::OLED, 140, 26, "SPACE to type", FontStyle::Tiny);

    u8g2.sendBuffer();
  }
}

void einkHandler_SETTINGS() {
  if (newState) {
    newState = false;
    beginEinkScreen();
    display.drawBitmap(0, 0, _settings, 320, 218, GxEPD_BLACK);

    // Draw scrollbar vertically aligned to the left of the internal bounding box
    drawScrollbar(settingsList.size(), SETTINGS_ITEMS_PAGE, settingsScrollIndex, SETTINGS_BOX_X, SETTINGS_BOX_Y, SETTINGS_BOX_H, 4, false, GxEPD_BLACK, GxEPD_WHITE);

    int textX = SETTINGS_BOX_X + 8; // Offset text past scrollbar
    int valX = textX;
    int nameX = valX + 38; // Provide room on the left to draw a checkbox or the [value]

    // Calculate Sliding Window Pagination
    int startIdx = settingsScrollIndex - (SETTINGS_ITEMS_PAGE / 2);
    if (startIdx < 0) startIdx = 0;
    if (startIdx > max(0, (int)settingsList.size() - SETTINGS_ITEMS_PAGE)) {
      startIdx = max(0, (int)settingsList.size() - SETTINGS_ITEMS_PAGE);
    }

    int y = SETTINGS_BOX_Y + 16; // First Baseline (Adjusted for 20px Pitch)
    
    // Draw Settings Array Iterations
    for (int i = startIdx; i < min((int)settingsList.size(), startIdx + SETTINGS_ITEMS_PAGE); i++) {
      SettingItem& item = settingsList[i];

      // Visually invert/highlight currently selected setting row
      if (i == settingsScrollIndex) {
        // Dynamically size the bounding box to fit the exact text
        int nameWidth = FontEngine::textWidth(DisplayTarget::EINK, item.name, FontStyle::Body);
        int boxW = (nameX - valX) + nameWidth + 8; // value width + text width + padding
        
        display.fillRoundRect(valX - 4, y - 13, boxW, 18, 4, GxEPD_BLACK);
        u8g2f.setForegroundColor(GxEPD_WHITE);
      } else {
        u8g2f.setForegroundColor(GxEPD_BLACK);
      }

      // Draw values on the left
      if (item.type == SetType::BOOLEAN) {
        const uint8_t* icon = *(item.boolVal) ? _toggleON : _toggleOFF;
        uint16_t color = (i == settingsScrollIndex) ? GxEPD_WHITE : GxEPD_BLACK;
        display.drawBitmap(valX, y - 10, icon, SETTINGS_TOG_W, SETTINGS_TOG_H, color);
      } 
      else if (item.type == SetType::INTEGER) {
        String valStr = "[" + String(*(item.intVal)) + "]";
        FontEngine::drawText(DisplayTarget::EINK, valX, y, valStr, FontStyle::Body);
      }
      else if (item.type == SetType::ACTION) {
        FontEngine::drawText(DisplayTarget::EINK, valX, y, "[>>]", FontStyle::Body);
      }

      // Draw text label
      FontEngine::drawText(DisplayTarget::EINK, nameX, y, item.name, FontStyle::Body);
      y += SETTINGS_PITCH;
    }

    u8g2f.setForegroundColor(GxEPD_BLACK);
    
    // Output the static instruction prompt box below
    endEinkScreen("Type or select a setting");
  }
}
#endif
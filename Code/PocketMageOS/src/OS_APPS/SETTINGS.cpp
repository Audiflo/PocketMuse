// AUDIT 1

#include <globals.h>
#if !OTA_APP // POCKETMAGE_OS

<<<<<<< Updated upstream
=======
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
  SettingItem{"FAST_REFRESH", "Fast Refresh (Unstable!)", SetType::BOOLEAN, nullptr, &FAST_REFRESH, 0, 0, updateFastRef},
  SettingItem{"DEBUG_VERBOSE", "Debug Verbose", SetType::BOOLEAN, nullptr, &DEBUG_VERBOSE, 0, 0, updateBlank},
  SettingItem{"HOME_ON_BOOT", "Home on Boot", SetType::BOOLEAN, nullptr, &HOME_ON_BOOT, 0, 0, updateBlank},
  SettingItem{"ALLOW_NO_SD", "Allow No SD", SetType::BOOLEAN, nullptr, &ALLOW_NO_MICROSD, 0, 0, updateBlank},
  SettingItem{"LOCK_ENABLED", "Device Lock", SetType::BOOLEAN, nullptr, (bool*)&deviceLocked, 0, 0, updateBlank},
  SettingItem{"", "Set Lock PIN", SetType::ACTION, nullptr, nullptr, 0, 0, actionSetPin},
  SettingItem{"", "Set Time", SetType::ACTION, nullptr, nullptr, 0, 0, actionSetTime},
  SettingItem{"", "Set Date", SetType::ACTION, nullptr, nullptr, 0, 0, actionSetDate},
  SettingItem{"", "System Language", SetType::ACTION, nullptr, nullptr, 0, 0, actionSetLang}
};

>>>>>>> Stashed changes
// Simplified state machine
enum SettingsState { SETTINGS_MAIN };
SettingsState CurrentSettingsState = SETTINGS_MAIN;

// Layout constants
constexpr int SETTINGS_VAL_X      = 8;    // value column x
constexpr int SETTINGS_VAL_Y0     = 42;   // first scalar value baseline
constexpr int SETTINGS_VAL_Y1     = 65;   // second scalar value baseline
constexpr int SETTINGS_VAL2_X     = 163;  // second column x
constexpr int SETTINGS_TOG_X      = 8;    // toggle switch x
constexpr int SETTINGS_TOG_Y0     = 75;   // first toggle row y
constexpr int SETTINGS_TOG_PITCH  = 23;   // toggle row pitch
constexpr int SETTINGS_TOG_W      = 26;   // toggle bitmap width
constexpr int SETTINGS_TOG_H      = 11;   // toggle bitmap height

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
    
    // Extract the argument if there is a space
    int spaceIdx = command.indexOf(' ');
    if (spaceIdx != -1) {
      timePart = command.substring(spaceIdx + 1);
      timePart.trim();
    }

    // If no argument was provided, launch the interactive UI
    if (timePart.length() == 0) {
      int newTime = timePrompt(); // Returns integer like 1430 or 5
      
      if (newTime < 0) return returnText;
      // Format the integer back into a safe, padded string (e.g., "00:05" or "14:30")
      char timeBuf[6];
      snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", newTime / 100, newTime % 100);
      
      CLOCK().setTimeFromString(String(timeBuf));
      returnText = TR(STR_SETTINGS_TIME_UPDATED_TO) + String(timeBuf);
    } 
    // If a manual argument was provided, validate and parse it directly
    else if (timePart.length() >= 4) { 
      CLOCK().setTimeFromString(timePart);
      returnText = TR(STR_SETTINGS_TIME_UPDATED);
    }
    else {
      returnText = TR(STR_SETTINGS_INVALID_FMT_HHMM);
    }
    
    return returnText;
  }
  else if (command == "lock" || command.startsWith("lock ")) {
    String lockArg = "";
    int lockSpaceIdx = command.indexOf(' ');
    if (lockSpaceIdx != -1) {
      lockArg = command.substring(lockSpaceIdx + 1);
      lockArg.trim();
      lockArg.toLowerCase();
    }

    // "lock" with no argument: report current state
    if (lockArg.length() == 0) {
      return lockIsEnabled() ? TR(STR_LOCK_ENABLED) : TR(STR_LOCK_DISABLED);
    }
    else if (lockArg == "on") {
      // If no PIN is stored yet, walk through masked create + confirm
      if (!lockHasPin()) {
        String pin1 = textPrompt(TR(STR_LOCK_ENTER_PIN), "", true);
        if (pin1 == "_EXIT_" || pin1 == "_RETURN_" || pin1 == "_CENTER_") return "";
        String pin2 = textPrompt(TR(STR_LOCK_CONFIRM_PIN), "", true);
        if (pin2 == "_EXIT_" || pin2 == "_RETURN_" || pin2 == "_CENTER_") return "";

        if (!lockPinValid(pin1)) return TR(STR_LOCK_PIN_INVALID);
        if (pin1 != pin2) return TR(STR_LOCK_MISMATCH);

        lockSetPin(pin1);
      } else {
        // PIN already configured: just switch the gate on
        prefs.begin("PocketMage", false);
        prefs.putBool("LOCK_ENABLED", true);
        prefs.end();
        deviceLocked = true;
      }
      newState = true;
      return TR(STR_LOCK_ENABLED);
    }
    else if (lockArg == "off") {
      lockDisable();
      newState = true;
      return TR(STR_LOCK_DISABLED);
    }
    else {
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
    
    // Extract the argument if there is a space
    int spaceIdx = command.indexOf(' ');
    if (spaceIdx != -1) {
      datePart = command.substring(spaceIdx + 1);
      datePart.trim();
    }

    // If no argument was provided, launch the interactive UI
    if (datePart.length() == 0) {
      String newDate = datePrompt(); // Returns formatted "DD/MM/YYYY"

      if (newDate == "_EXIT_") return returnText;
      // Parse the returned string into integers
      int day   = newDate.substring(0, 2).toInt();
      int month = newDate.substring(3, 5).toInt();
      int year  = newDate.substring(6, 10).toInt();

      DateTime now = CLOCK().nowDT();  // Preserve current time
      CLOCK().getRTC().adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
      
      returnText = TR(STR_SETTINGS_DATE_UPDATED_TO) + newDate;
    }
    // If a manual argument was provided, validate and parse it directly
    else if (datePart.length() == 8 && datePart.toInt() > 0) {
      int year  = datePart.substring(0, 4).toInt();
      int month = datePart.substring(4, 6).toInt();
      int day   = datePart.substring(6, 8).toInt();

      static const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
      int maxDay = dim[month - 1];
      if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) maxDay = 29;
      if (year < 1970 || year > 2200 || month < 1 || month > 12 || day < 1 || day > maxDay) {
        returnText = TR(STR_SETTINGS_INVALID_DATE);
      } else {
        DateTime now = CLOCK().nowDT();  // Preserve current time
        CLOCK().getRTC().adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
        returnText = TR(STR_SETTINGS_DATE_UPDATED);
      }
    } else {
      returnText = TR(STR_SETTINGS_INVALID_FMT_YYYYMMDD);
    }
    
    return returnText;
  }
  else if (command.startsWith("lumina ")) {
    String luminaPart = command.substring(7);
    int lumina = stringToInt(luminaPart);
    if (lumina == -1) return TR(STR_INVALID);
    
    if (lumina > 255) lumina = 255;
    else if (lumina < 0) lumina = 0;
    
    OLED_BRIGHTNESS = lumina;
    u8g2.setContrast(OLED_BRIGHTNESS);
    
    prefs.begin("PocketMage", false);
    prefs.putInt("OLED_BRIGHTNESS", OLED_BRIGHTNESS);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("timeout ")) {
    String timeoutPart = command.substring(8);
    int timeout = stringToInt(timeoutPart);
    if (timeout == -1) return TR(STR_SETTINGS_INVALID_BANG);
    
    if (timeout > 3600) timeout = 3600;
    else if (timeout < 15) timeout = 15;
    
    TIMEOUT = timeout;
    
    prefs.begin("PocketMage", false);
    prefs.putInt("TIMEOUT", TIMEOUT);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("oledfps ")) {
    String oledfpsPart = command.substring(8);
    int oledfps = stringToInt(oledfpsPart);
    if (oledfps == -1) return TR(STR_INVALID);
    
    if (oledfps > 144) oledfps = 144;
    else if (oledfps < 5) oledfps = 5;
    
    OLED_MAX_FPS = oledfps;
    
    prefs.begin("PocketMage", false);
    prefs.putInt("OLED_MAX_FPS", OLED_MAX_FPS);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("clock ")) {
    String clockPart = command.substring(6);
    clockPart.trim();

    if (clockPart != "t" && clockPart != "f") return "Invalid";

    SYSTEM_CLOCK = (clockPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("SYSTEM_CLOCK", SYSTEM_CLOCK);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("showyear ")) {
    String yearPart = command.substring(9);
    yearPart.trim();

    if (yearPart != "t" && yearPart != "f") return "Invalid";

    SHOW_YEAR = (yearPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("SHOW_YEAR", SHOW_YEAR);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("savepower ")) {
    String savePowerPart = command.substring(10);
    savePowerPart.trim();

    if (savePowerPart != "t" && savePowerPart != "f") return "Invalid";

    SAVE_POWER = (savePowerPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("SAVE_POWER", SAVE_POWER);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("fastrefresh ")) {
    String fastRefreshPart = command.substring(12);
    fastRefreshPart.trim();

    if (fastRefreshPart != "t" && fastRefreshPart != "f") return "Invalid";

    FAST_REFRESH = (fastRefreshPart == "t");

    prefs.begin("PocketMage", false);
    prefs.putBool("FAST_REFRESH", FAST_REFRESH);
    prefs.end();

    // Switching from legacy to fast refresh mid-session must rebuild the
    // panel's previous-image RAM before the first partial redraw, otherwise
    // the differential update diffs against a stale frame.
    if (FAST_REFRESH) EINK().markPanelNeedsFullRefresh();

    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("debug ")) {
    String debugPart = command.substring(6);
    debugPart.trim();

    if (debugPart != "t" && debugPart != "f") return "Invalid";

    DEBUG_VERBOSE = (debugPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("DEBUG_VERBOSE", DEBUG_VERBOSE);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("boottohome ")) {
    String bootHomePart = command.substring(11);
    bootHomePart.trim();

    if (bootHomePart != "t" && bootHomePart != "f") return "Invalid";

    HOME_ON_BOOT = (bootHomePart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("HOME_ON_BOOT", HOME_ON_BOOT);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command.startsWith("allownosd ")) {
    String noSDPart = command.substring(10);
    noSDPart.trim();

    if (noSDPart != "t" && noSDPart != "f") return "Invalid";

    ALLOW_NO_MICROSD = (noSDPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("ALLOW_NO_SD", ALLOW_NO_MICROSD);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
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
  String command = "";
  String returnText = "";

<<<<<<< Updated upstream
  switch (CurrentSettingsState) {
    case SETTINGS_MAIN:
      command = textPrompt();
      if (command == "_RETURN_") return;
      else if (command != "_EXIT_") {
        returnText = settingCommandSelect(command);
        if (returnText != "") {
          OLED().sysMessage(returnText,1000);
=======
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
          // Direct Toggle logic instead of boolPrompt
          bool nextState = !(*(item.boolVal));
          
          if (nextState) { // Turning ON
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
          else { // Turning OFF
            if (item.key == "LOCK_ENABLED") {
              lockDisable();
              *(item.boolVal) = false;
              deviceLocked = false;
            } else {
              *(item.boolVal) = false;
            }
          }

          // Save the toggled value
          prefs.begin("PocketMage", false);
          prefs.putBool(item.key.c_str(), *(item.boolVal));
          prefs.end();
          if (item.onUpdate) item.onUpdate();
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
>>>>>>> Stashed changes
        }
      }
      else HOME_INIT();
      break;
  }
}

void einkHandler_SETTINGS() {
  if (newState) {
    newState = false;
    beginEinkScreen();
    display.drawBitmap(0, 0, _settings, 320, 218, GxEPD_BLACK);

    // First column of settings
    // OLED_BRIGHTNESS
    FontEngine::drawText(DisplayTarget::EINK, SETTINGS_VAL_X, SETTINGS_VAL_Y0, String(OLED_BRIGHTNESS), FontStyle::Body);
    // TIMEOUT
    FontEngine::drawText(DisplayTarget::EINK, SETTINGS_VAL_X, SETTINGS_VAL_Y1, String(TIMEOUT), FontStyle::Body);
    // SYSTEM_CLOCK
    if (SYSTEM_CLOCK) display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (0 * SETTINGS_TOG_PITCH), _toggleON, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    else display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (0 * SETTINGS_TOG_PITCH), _toggleOFF, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    // SHOW_YEAR
    if (SHOW_YEAR) display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (1 * SETTINGS_TOG_PITCH), _toggleON, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    else display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (1 * SETTINGS_TOG_PITCH), _toggleOFF, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    // SAVE_POWER
    if (SAVE_POWER) display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (2 * SETTINGS_TOG_PITCH), _toggleON, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    else display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (2 * SETTINGS_TOG_PITCH), _toggleOFF, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    // DEBUG_VERBOSE
    if (DEBUG_VERBOSE) display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (3 * SETTINGS_TOG_PITCH), _toggleON, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    else display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (3 * SETTINGS_TOG_PITCH), _toggleOFF, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    // HOME_ON_BOOT
    if (HOME_ON_BOOT) display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (4 * SETTINGS_TOG_PITCH), _toggleON, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    else display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (4 * SETTINGS_TOG_PITCH), _toggleOFF, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    // ALLOW_NO_MICROSD
    if (ALLOW_NO_MICROSD) display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (5 * SETTINGS_TOG_PITCH), _toggleON, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    else display.drawBitmap(SETTINGS_TOG_X, SETTINGS_TOG_Y0 + (5 * SETTINGS_TOG_PITCH), _toggleOFF, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    // FAST_REFRESH (experimental), second column first row
    if (FAST_REFRESH) display.drawBitmap(SETTINGS_VAL2_X, SETTINGS_TOG_Y0, _toggleON, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    else display.drawBitmap(SETTINGS_VAL2_X, SETTINGS_TOG_Y0, _toggleOFF, SETTINGS_TOG_W, SETTINGS_TOG_H, GxEPD_BLACK);
    // OLED_MAX_FPS
    FontEngine::drawText(DisplayTarget::EINK, SETTINGS_VAL2_X, SETTINGS_VAL_Y0, String(OLED_MAX_FPS), FontStyle::Body);

<<<<<<< Updated upstream
    endEinkScreen(TR(STR_HOME_TYPE_CMD));
=======
    int textX = SETTINGS_BOX_X + 10; // Offset text past scrollbar
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
        int boxW = (nameX - valX) + nameWidth + 12; // value width + text width + padding
        
        display.fillRoundRect(valX - 2, y - 13, boxW, 18, 4, GxEPD_BLACK);
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
>>>>>>> Stashed changes
  }
}
#endif
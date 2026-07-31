//  .d88888b  888888ba   //
//  88.    "' 88    `8b  //
//  `Y88888b. 88     88  //
//        `8b 88     88  //
//  d8'   .8P 88    .8P  //
//   Y88888P  8888888P   //

#include <pocketmage.h>
#include <globals.h>
#include <config.h>
#include <SD_MMC.h>
#include <SD.h>
#include <SPI.h>

static constexpr const char* TAG = "SD";

extern bool SAVE_POWER;

static PocketmageSD pm_sd;

PocketmageSD& PM_SD()      { return pm_sd; }
PocketmageSD& PM_SDAUTO()  { return pm_sd; }

void PocketmageSD::beginIO() { SDActive = true; }
void PocketmageSD::endIO()   { SDActive = false; }

static int countVisibleCharsFile(fs::FS &fs, const char* path) {
  File f = fs.open(path, "r");
  if (!f || f.isDirectory()) return 0;

  int count = 0;
  uint8_t buf[512];

  while (f.available()) {
    size_t len = f.read(buf, sizeof(buf));
    for (size_t i = 0; i < len; i++) {
      if (buf[i] >= 32 && buf[i] <= 126) {
        count++;
      }
    }
    vTaskDelay(1);
  }

  f.close();
  return count;
}

static const char* GUIDE_BACKGROUND =
  "How to add custom backgrounds:\n"
  "1. Make a background that is 1 bit (black OR white) and 320x240 pixels.\n"
  "2. Export your background as a .bmp file.\n"
  "3. Use image2cpp to convert your image to a .bin file.\n"
  "   Settings: Invert Image Colors = TRUE, Swap Bits in Byte = FALSE.\n"
  "4. Place the .bin file in this folder.\n"
  "5. Enjoy your new custom wallpapers!";

static const char* GUIDE_COMMANDS =
  "# PocketMage Keystrokes Guide\n"
  "This is a guide on common key combinations and commands on the PocketMage PDA device. "
  "The guide is split up into sections based on application.\n" "\n" "---\n"
  "## General Keystrokes (work in almost any app)\n"
  "- (FN) + ( < ) | Exit or back button\n"
  "- (FN) + ( > ) | Save document\n"
  "- (FN) + ( o ) | Clear Line\n"
  "- (FN) + (Key) | FN layer keymapping (legends on the PCB)\n"
  "- (SHFT) + (key) | Capital letter\n"
  "- ( o ) OR (ENTER) | Select button\n"
  "\n"
  "---\n"
  "## While Sleeping\n"
  "### Bypass home and directly enter an app\n"
  "You can bypass the home menu and enter directly into an app and wake up with one keystroke. "
  "Pressing the buttons below while PocketMage is sleeping will wake the device and boot into the corresponding app.\n"
  "\n"
  "- ( SPACE ) - Return to previous app (saved state from last sleep)\n"
  "- ( H ) - Home\n"
  "- ( U ) - USB\n"
  "- ( F ) - Filewiz\n"
  "- ( T ) - Tasks\n"
  "- ( N ) - TXT\n"
  "- ( S ) - Settings\n"
  "- ( C ) - Calendar\n"
  "- ( J ) - Journal\n"
  "- ( D ) - Dictionary (lexicon)\n"
  "- ( L ) - Loader\n"
  "\n"
  "---\n"
  "## Home App\n"
  "### Entering an OS app\n"
  "Type an app's name to enter that app. For example, to enter calendar, type \"calendar\". "
  "You can type the name as it appears on the screen or use a shortcut. " "For example, typing \"cal\" also enters the calendar.\n"
  "\n"
  "### Entering a 3rd party app\n"
  "For 3rd party apps, type the letter of the slot that app is installed in. "
  "For example if you have the Calc app installed in the first app slot, type \"a\" to enter the app.\n"
  "\n"
  "### Other commands\n"
  "Many other commands can be done from the homescreen, including all of the settings commands "
  "and some other fun ones for you to discover!\n"
  "\n"
  "---\n"
  "## TXT App\n"
  "- (FN) + ( < ) | Exit app\n"
  "- (FN) + ( > ) | Save document\n"
  "- (FN) + ( o ) | Enter filesystem (loading files)\n"
  "- (SHFT) + ( o ) | New blank text document\n"
  "- (FN) + (Key) | FN layer keymapping (legends on the PCB)\n"
  "- (SHFT) + (key) | Capital letter\n"
  "- (ENTER) | Create a new line\n"
  "- (SHFT) + ( < ) | Change text style (body, heading, etc.)\n"
  "- (SHFT) + ( > ) | Change formatting (bold, italics, etc.)\n"
  "- Scroll Bar | Swipe up or down to scroll through the document\n"
  "\n"
  "---\n"
  "## FILEWIZ\n"
  "- (FN) + ( < ) | Exit app\n"
  "- ( < ) AND ( > ) | Scroll left and right\n"
  "- ( o ) OR (ENTER) | Select file or folder\n"
  "- ( 0 ) TO ( 9 ) | Select recent file\n"
  "- ( BKSP ) | Go back a filesystem level\n"
  "\n"
  "---\n"
  "## USB\n"
  "Plug in the PocketMage to your PC to view the files. Eject and exit the app when you're finished.\n"
  "- (FN) + ( < ) | Exit app\n"
  "\n"
  "---\n"
  "## Settings\n"
  "Type the setting as it appears on the screen to change it. Some examples are given below. "
  "Note: all settings are case-insensitive, meaning that you can type in all lowercase. "
  "All of these settings are also available from the home menu command bar if you memorize them.\n"
  "- TimeSet [HH]:[MM] -> TimeSet 15:46\n"
  "- DateSet YYYYMMDD -> DateSet 20251230\n"
  "- ShowYear [bool] -> ShowYear t\n"
  "- Timeout [int] -> Timeout 300\n"
  "- (FN) + ( < ) | Exit app\n"
  "\n"
  "---\n"
  "## Tasks\n"
  "- ( N ) | Create a new task, follow on-screen prompts\n"
  "- (ENTER) | Enter information into prompt\n"
  "- ( 0 ) TO ( 9 ) | Select task for editing\n"
  "- (FN) + ( < ) | Exit app\n"
  "\n"
  "---\n"
  "## Calendar\n"
  "Type commands to navigate dates or create events. All commands are case-insensitive.\n"
  "\n"
  "### Month View\n"
  "- jan 2025 / feb 2030 / etc. | Jump to month and year\n"
  "- 20251225 | Jump to exact date (YYYYMMDD)\n"
  "- 14 | Jump to a day in the current month\n"
  "- ( N ) | New event\n"
  "- (FN) + ( < ) | Exit app\n"
  "\n"
  "### Week View\n"
  "- sun, mon, tue, wed, thu, fri, sat | Jump to weekday in the viewed week\n"
  "- ( N ) | New event\n"
  "- (FN) + ( < ) | Exit app\n"
  "\n"
  "### Day View\n"
  "- ( N ) | New event for selected day\n"
  "- 1, 2, 3, ... | Open event by index\n"
  "- (FN) + ( < ) | Exit app\n"
  "\n"
  "### Repeating Events\n"
  "- no | No repeat\n"
  "- daily | Repeat every day\n"
  "- weekly xx | Repeat every week, xx is one or more of mo, tu, we, th, fr, sa, su\n"
  "- monthly xx | Repeat monthly, xx is the day of the month (1-31) or ordinal weekday (ex. 2tu)\n"
  "- yearly xx | Repeat every year, xx is month and day of the month (ex. apr22)\n"
  "\n"
  "---\n"
  "## Journal\n"
  "Type a date to open or create a journal entry. Commands are case-insensitive.\n"
  "- ( T ) | Open today's journal entry\n"
  "- YYYYMMDD - Example: 20250314 | Open/create entry for exact date\n"
  "- jan 1 / feb 12 / etc. | Open/create entry for given month and day (uses current year)\n"
  "- (FN) + ( < ) | Exit app\n"
  "\n"
  "---\n"
  "## Lexicon\n"
  "Type a word to search the dictionary. Matches are loaded from the SD card. Commands are case-insensitive.\n"
  "- Type any word | Search for definitions (example: abandon)\n"
  "- (ENTER) | Execute search\n"
  "- ( < ) OR ( > ) | Previous / next definition\n"
  "- (FN) + ( < ) | Exit app\n"
  "\n"
  "---\n"
  "## App loader\n"
  "Manage and install .tar apps to OTA slots. Commands are case-insensitive.\n"
  "- ( S ) | Swap app in selected slot (choose a .tar file)\n"
  "- ( D ) | Delete app in selected slot\n"
  "- (FN) + ( < ) | Exit app / return to menu\n"
  "- Progress Bar | Shows extraction (0-50%) and installation (50-100%) status\n"
  "\n"
  "---\n"
  "## Sleep Modes\n"
  "When on battery, save power and look at a random screensaver. "
  "When charging, view a clock, upcoming tasks, and weather (work in progress)\n"
  "### Sleep (when not plugged into usb)\n"
  "- sleep button to enter sleep\n"
  "- any key on keyboard to wake\n"
  "### Now-Later (when usb is plugged in)\n"
  "- sleep button to enter now-later\n"
  "- sleep button to wake\n";

static void provisionFilesystem() {
  const char* dirs[] = {"/sys", "/notes", "/journal", "/dict", "/apps",
                        "/apps/temp", "/assets", "/assets/backgrounds", "/chats"};
  for (auto dir : dirs) if (!global_fs->exists(dir)) global_fs->mkdir(dir);

  if (!global_fs->exists("/assets/backgrounds/HOWTOADDBACKGROUNDS.txt")) {
    File f = global_fs->open("/assets/backgrounds/HOWTOADDBACKGROUNDS.txt", FILE_WRITE);
    if (f) { f.print(GUIDE_BACKGROUND); f.close(); }
  }

  if (!global_fs->exists("/sys/COMMAND_MANUAL.txt")) {
    File f = global_fs->open("/sys/COMMAND_MANUAL.txt", FILE_WRITE);
    if (f) { f.print(GUIDE_COMMANDS); f.close(); }
  }

  const char* sysFiles[] = {"/sys/events.txt", "/sys/tasks.txt", "/sys/SDMMC_META.txt"};
  for (auto file : sysFiles) {
    if (!global_fs->exists(file)) {
      File f = global_fs->open(file, FILE_WRITE);
      if (f) f.close();
    }
  }
}

void setupSD() {
  prefs.begin("PocketMage", true);
  SD_SPI_COMPATIBILITY = prefs.getBool("SD_SPI_CMPT", false);
  ALLOW_NO_MICROSD = prefs.getBool("ALLOW_NO_SD", true);
  prefs.end();
  Serial.print("SD_SPI_CMPT" + String(SD_SPI_COMPATIBILITY));
  delay(100);

  if (!SD_SPI_COMPATIBILITY) {
    global_fs = &SD_MMC;
    PM_SD().setMode(PocketmageSD::SDMMC);

    pocketmage::setCpuSpeed(240);

    #if POCKETMAGE_HW_VERSION == 2
      SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3);
      bool mode1bit = false;
    #else
      SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0);
      bool mode1bit = true;
    #endif

    bool sdOK = false;
    bool startedSD = false;
    sdcard_type_t cardType = CARD_NONE;
    for (int attempt = 1; attempt <= 25; attempt++) {
        if (SD_MMC.begin("/sdcard", mode1bit)) {
            startedSD = true;
            delay(120);
            cardType = SD_MMC.cardType();
            if (cardType != CARD_NONE) {
                sdOK = true;
                break;
            }
        }
        SD_MMC.end();
        delay(200);
    }

    if (!sdOK) {
        ESP_LOGE(TAG, "MOUNT FAILED");
        if (startedSD) {
            OLED().oledWord(
                String("SD Not Detected! [") +
                (cardType == CARD_MMC  ? "MMC"  :
                  cardType == CARD_SD   ? "SD"   :
                  cardType == CARD_SDHC ? "SDHC" :
                                          "NONE") + "]",
                false, false
            );
        } else {
          OLED().oledWord("SD Not Detected! [START_FAIL]", false, false);
          delay(3000);
          OLED().oledWord("Entering Compatibility Mode", false, false);
          prefs.begin("PocketMage", false);
          prefs.putBool("SD_SPI_CMPT", true);
          prefs.end();
          delay(3000);
          esp_restart();
        }

        delay(5000);
        if (ALLOW_NO_MICROSD) {
          OLED().sysMessage("All Work Will be Lost!",5000);
          PM_SD().setNoSD(true);
          return;
        } else {
          OLED().sysMessage("Insert SD Card and Reboot!",5000);
          OLED().setPowerSave(1);
          BZ().playJingle(Jingles::Shutdown);
          esp_deep_sleep_start();
          return;
        }
    }

    prefs.begin("PocketMage", false);
    prefs.putBool("SD_SPI_CMPT", false);
    prefs.end();

    provisionFilesystem();
  } else {
      global_fs = &SD;
      PM_SD().setMode(PocketmageSD::SDSPI);

      pocketmage::setCpuSpeed(240);

      hspi = new SPIClass(HSPI);
      hspi->begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
      pinMode(hspi->pinSS(), OUTPUT);
      if (!SD.begin(SD_CS, *hspi, 40000000)) {
          ESP_LOGE(TAG, "SPI SD Mount Failed");
          OLED().oledWord("SPI SD Not Detected!", false, false);
          delay(2000);

          if (ALLOW_NO_MICROSD) {
              OLED().oledWord("All Work Will Be Lost!", false, false);
              delay(5000);
              PM_SD().setNoSD(true);
              return;
          } else {
              OLED().oledWord("Compatibility Mode Failed. Retrying...", false, false);
              prefs.begin("PocketMage", false);
              prefs.putBool("SD_SPI_CMPT", false);
              prefs.end();
              delay(2000);
              OLED().setPowerSave(1);
              BZ().playJingle(Jingles::Shutdown);
              esp_deep_sleep_start();
              return;
          }
      }
      OLED().oledWord("SD Started In Compatibility Mode", false, false);

      provisionFilesystem();
  }
}

// ===================== high-level operations =====================

void PocketmageSD::saveFile() {
  fs::FS& fs = (mode_ == SDMMC) ? static_cast<fs::FS&>(SD_MMC) : static_cast<fs::FS&>(SD);
  if (noSD_) {
    OLED().sysMessage("SAVE FAILED - No SD!",5000);
    return;
  }
  beginIO();
  if (getCpuFrequencyMhz() != 240) {
    pocketmage::setCpuSpeed(240);
    delay(50);
  }

  String textToSave = vectorToString();
  ESP_LOGV(TAG, "Text to save: %s", textToSave.c_str());

  if (editingFile_ == "" || editingFile_ == "-")
    editingFile_ = "/temp.txt";
  keypad.disableInterrupts();
  if (!editingFile_.startsWith("/"))
    editingFile_ = "/" + editingFile_;
  writeFile(fs, editingFile_.c_str(), textToSave.c_str());

  writeMetadata(editingFile_);

  keypad.enableInterrupts();
  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}

void PocketmageSD::writeMetadata(const String& path) {
  fs::FS& fs = (mode_ == SDMMC) ? static_cast<fs::FS&>(SD_MMC) : static_cast<fs::FS&>(SD);
  beginIO();
  if (getCpuFrequencyMhz() != 240) {
    pocketmage::setCpuSpeed(240);
    delay(50);
  }

  File file = global_fs->open(path);
  if (!file || file.isDirectory()) {
    OLED().sysMessage("META WRITE ERR",1000);
    ESP_LOGE(TAG, "Invalid file for metadata: %s", path.c_str());
    return;
  }

  size_t fileSizeBytes = file.size();
  file.close();

  String fileSizeStr = String(fileSizeBytes) + " Bytes";

  int charCount = countVisibleCharsFile(fs, path.c_str());
  String charStr = String(charCount) + " Char";

  DateTime now = CLOCK().nowDT();
  char timestamp[20];
  sprintf(timestamp, "%04d%02d%02d-%02d%02d", now.year(), now.month(), now.day(), now.hour(),
          now.minute());

  String newEntry = path + "|" + timestamp + "|" + fileSizeStr + "|" + charStr;

  const char* metaPath = SYS_METADATA_FILE;
  File metaFile = global_fs->open(metaPath, FILE_READ);
  String updatedMeta = "";
  bool replaced = false;

  if (metaFile) {
    while (metaFile.available()) {
      String line = metaFile.readStringUntil('\n');
      if (line.startsWith(path + "|")) {
        updatedMeta += newEntry + "\n";
        replaced = true;
      } else if (line.length() > 1) {
        updatedMeta += line + "\n";
      }
    }
    metaFile.close();
  }

  if (!replaced) {
    updatedMeta += newEntry + "\n";
  }

  metaFile = global_fs->open(metaPath, FILE_WRITE);
  if (!metaFile) {
    ESP_LOGE(TAG, "Failed to open metadata file for writing: %s", metaPath);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    endIO();
    return;
  }
  metaFile.print(updatedMeta);
  metaFile.close();
  ESP_LOGI(TAG, "Metadata updated");

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}

void PocketmageSD::loadFile(bool showOLED) {
  fs::FS& fs = (mode_ == SDMMC) ? static_cast<fs::FS&>(SD_MMC) : static_cast<fs::FS&>(SD);
  if (noSD_) {
    OLED().sysMessage("LOAD FAILED - No SD!",5000);
    return;
  }
  beginIO();
  if (getCpuFrequencyMhz() != 240) {
    pocketmage::setCpuSpeed(240);
    delay(50);
  }

  keypad.disableInterrupts();
  if (showOLED)
    OLED().oledWord("Loading File");
  if (!editingFile_.startsWith("/"))
    editingFile_ = "/" + editingFile_;
  String textToLoad = readFileToString(fs, editingFile_.c_str());
  ESP_LOGV(TAG, "Text to load: %s", textToLoad.c_str());

  stringToVector(textToLoad);
  keypad.enableInterrupts();
  if (showOLED) {
    OLED().oledWord("File Loaded");
    delay(200);
  }
  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}

void PocketmageSD::delFile(String fileName) {
  fs::FS& fs = (mode_ == SDMMC) ? static_cast<fs::FS&>(SD_MMC) : static_cast<fs::FS&>(SD);
  if (noSD_) {
    OLED().sysMessage("DELETE FAILED - No SD!",5000);
    return;
  }
  beginIO();
  pocketmage::setCpuSpeed(240);
  delay(50);

  keypad.disableInterrupts();
  if (!fileName.startsWith("/"))
    fileName = "/" + fileName;
  deleteFile(fs, fileName.c_str());

  deleteMetadata(fileName);

  delay(1000);
  keypad.enableInterrupts();
  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}

void PocketmageSD::deleteMetadata(String path) {
  beginIO();
  pocketmage::setCpuSpeed(240);
  delay(50);

  const char* metaPath = SYS_METADATA_FILE;

  File metaFile = global_fs->open(metaPath, FILE_READ);
  if (!metaFile) {
    ESP_LOGE(TAG, "Metadata file not found: %s", metaPath);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    endIO();
    return;
  }

  std::vector<String> keptLines;
  while (metaFile.available()) {
    String line = metaFile.readStringUntil('\n');
    if (!line.startsWith(path + "|")) {
      keptLines.push_back(line);
    }
  }
  metaFile.close();

  global_fs->remove(metaPath);

  File writeFile = global_fs->open(metaPath, FILE_WRITE);
  if (!writeFile) {
    ESP_LOGE(TAG, "Failed to recreate metadata file. %s", metaPath);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    endIO();
    return;
  }

  for (const String& line : keptLines) {
    writeFile.println(line);
  }

  writeFile.close();
  ESP_LOGI(TAG, "Metadata entry deleted (if it existed).");

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}

void PocketmageSD::renFile(String oldFile, String newFile) {
  fs::FS& fs = (mode_ == SDMMC) ? static_cast<fs::FS&>(SD_MMC) : static_cast<fs::FS&>(SD);
  if (noSD_) {
    OLED().sysMessage("RENAME FAILED - No SD!",5000);
    return;
  }
  beginIO();
  pocketmage::setCpuSpeed(240);
  delay(50);

  keypad.disableInterrupts();
  if (!oldFile.startsWith("/"))
    oldFile = "/" + oldFile;
  if (!newFile.startsWith("/"))
    newFile = "/" + newFile;
  renameFile(fs, oldFile.c_str(), newFile.c_str());
  OLED().sysMessage(oldFile + " -> " + newFile ,1000);

  renMetadata(oldFile, newFile);

  keypad.enableInterrupts();
  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}
void PocketmageSD::renMetadata(String oldPath, String newPath) {
  beginIO();
  pocketmage::setCpuSpeed(240);
  delay(50);
  const char* metaPath = SYS_METADATA_FILE;

  File metaFile = global_fs->open(metaPath, FILE_READ);
  if (!metaFile) {
    ESP_LOGE(TAG, "Metadata file not found: %s", metaPath);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    endIO();
    return;
  }

  std::vector<String> updatedLines;

  while (metaFile.available()) {
    String line = metaFile.readStringUntil('\n');

    if (line.startsWith(oldPath + "|")) {
      int separatorIndex = line.indexOf('|');
      if (separatorIndex != -1) {
        String rest = line.substring(separatorIndex);
        line = newPath + rest;
      } else {
        line = newPath;
      }
    }
    updatedLines.push_back(line);
  }

  metaFile.close();

  global_fs->remove(metaPath);

  File writeFile = global_fs->open(metaPath, FILE_WRITE);
  if (!writeFile) {
    ESP_LOGE(TAG, "Failed to recreate metadata file. %s", metaPath);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    endIO();
    return;
  }

  for (const String& l : updatedLines) {
    writeFile.println(l);
  }

  writeFile.close();
  ESP_LOGI(TAG, "Metadata updated for renamed file.");

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}

void PocketmageSD::copyFile(String oldFile, String newFile) {
  fs::FS& fs = (mode_ == SDMMC) ? static_cast<fs::FS&>(SD_MMC) : static_cast<fs::FS&>(SD);
  if (noSD_) {
    OLED().sysMessage("COPY FAILED - No SD!",5000);
    return;
  }
  beginIO();
  pocketmage::setCpuSpeed(240);
  delay(50);

  keypad.disableInterrupts();
  OLED().oledWord("Loading File");
  if (!oldFile.startsWith("/"))
    oldFile = "/" + oldFile;
  if (!newFile.startsWith("/"))
    newFile = "/" + newFile;
  String textToLoad = readFileToString(fs, oldFile.c_str());
  writeFile(fs, newFile.c_str(), textToLoad.c_str());
  OLED().oledWord("Saved: " + newFile);

  writeMetadata(newFile);

  delay(1000);
  keypad.enableInterrupts();

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}

void PocketmageSD::appendToFile(String path, String inText) {
  fs::FS& fs = (mode_ == SDMMC) ? static_cast<fs::FS&>(SD_MMC) : static_cast<fs::FS&>(SD);
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return;
  }
  beginIO();
  pocketmage::setCpuSpeed(240);
  delay(50);

  keypad.disableInterrupts();
  appendFile(fs, path.c_str(), inText.c_str());

  writeMetadata(path);

  keypad.enableInterrupts();

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  endIO();
}

// ===================== low-level operations =====================

void PocketmageSD::listDir(fs::FS &fs, const char *dirname) {
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return;
  }
  pocketmage::setCpuSpeed(240);
  delay(50);
  bool prevTimeout = noTimeout;
  noTimeout = true;
  ESP_LOGI(tag, "Listing directory %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    noTimeout = prevTimeout;
    ESP_LOGE(tag, "Failed to open directory: %s", dirname);
    return;
  }
  if (!root.isDirectory()) {
    noTimeout = prevTimeout;
    ESP_LOGE(tag, "Not a directory: %s", dirname);
    return;
  }

  fileIndex_ = 0;
  for (int i = 0; i < MAX_FILES; i++) {
    filesList_[i] = "-";
  }

  File file = root.openNextFile();
  while (file && fileIndex_ < MAX_FILES) {
    if (!file.isDirectory()) {
      String fileName = String(file.name());

      bool excluded = false;
      for (const String &excludedFile : excludedFiles_) {
        if (fileName.equals(excludedFile) || ("/"+fileName).equals(excludedFile)) {
          excluded = true;
          break;
        }
      }

      if (!excluded) {
        filesList_[fileIndex_++] = fileName;
      }
    }
    file = root.openNextFile();
  }

  noTimeout = prevTimeout;
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void PocketmageSD::readFile(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return;
  }
  pocketmage::setCpuSpeed(240);
  delay(50);
  bool prevTimeout = noTimeout;
  noTimeout = true;
  ESP_LOGI(tag, "Reading file %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    noTimeout = prevTimeout;
    ESP_LOGE(tag, "Failed to open file for reading: %s", path);
    return;
  }

  file.close();
  noTimeout = prevTimeout;
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

String PocketmageSD::readFileToString(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return "";
  }
  pocketmage::setCpuSpeed(240);
  delay(50);

  bool prevTimeout = noTimeout;
  noTimeout = true;
  ESP_LOGI(tag, "Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    noTimeout = prevTimeout;
    ESP_LOGE(tag, "Failed to open file for reading: %s", path);
    OLED().sysMessage("Load Failed",500);
    return "";
  }

  ESP_LOGI(tag, "Reading from file: %s", file.path());
  String content = file.readString();

  file.close();
  EINK().setFullRefreshAfter(FULL_REFRESH_AFTER);
  noTimeout = prevTimeout;
  return content;
}

void PocketmageSD::writeFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return;
  }
  pocketmage::setCpuSpeed(240);
  delay(50);
  bool prevTimeout = noTimeout;
  noTimeout = true;
  ESP_LOGI(tag, "Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    noTimeout = prevTimeout;
    ESP_LOGE(tag, "Failed to open %s for writing", path);
    return;
  }
  if (file.print(message)) {
    ESP_LOGV(tag, "File written %s", path);
  } else {
    ESP_LOGE(tag, "Write failed for %s", path);
  }
  file.close();
  noTimeout = prevTimeout;
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void PocketmageSD::appendFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return;
  }
  pocketmage::setCpuSpeed(240);
  delay(50);
  bool prevTimeout = noTimeout;
  noTimeout = true;
  ESP_LOGI(tag, "Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    noTimeout = prevTimeout;
    ESP_LOGE(tag, "Failed to open for appending: %s", path);
    return;
  }
  if (file.println(message)) {
    ESP_LOGV(tag, "Message appended to %s", path);
  } else {
    ESP_LOGE(tag, "Append failed: %s", path);
  }
  file.close();
  noTimeout = prevTimeout;
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void PocketmageSD::renameFile(fs::FS &fs, const char *path1, const char *path2) {
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return;
  }
  pocketmage::setCpuSpeed(240);
  delay(50);
  bool prevTimeout = noTimeout;
  noTimeout = true;
  ESP_LOGI(tag, "Renaming file %s to %s\r\n", path1, path2);

  if (fs.rename(path1, path2)) {
    ESP_LOGV(tag, "Renamed %s to %s\r\n", path1, path2);
  } else {
    ESP_LOGE(tag, "Rename failed: %s to %s", path1, path2);
  }
  noTimeout = prevTimeout;
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void PocketmageSD::deleteFile(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return;
  }
  pocketmage::setCpuSpeed(240);
  delay(50);
  bool prevTimeout = noTimeout;
  noTimeout = true;
  ESP_LOGI(tag, "Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    ESP_LOGV(tag, "File deleted: %s", path);
  } else {
    ESP_LOGE(tag, "Delete failed for %s", path);
  }
  noTimeout = prevTimeout;
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

bool PocketmageSD::readBinaryFile(const char* path, uint8_t* buf, size_t len) {
  if (noSD_) {
    OLED().sysMessage("OP FAILED - No SD!",5000);
    return false;
  }

  pocketmage::setCpuSpeed(240);

  bool prevTimeout = noTimeout;
  noTimeout = true;

  File f = global_fs->open(path, "r");
  if (!f || f.isDirectory()) {
    noTimeout = prevTimeout;
    ESP_LOGE(tag, "Failed to open file: %s", path);
    return false;
  }

  size_t n = f.read(buf, len);
  f.close();

  noTimeout = prevTimeout;
  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  return n == len;
}

size_t PocketmageSD::getFileSize(const char* path) {
  if (noSD_)
    return 0;

  File f = global_fs->open(path, "r");
  if (!f)
    return 0;
  size_t size = f.size();
  f.close();
  return size;
}

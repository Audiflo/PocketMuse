//  .d88888b  888888ba   //
//  88.    "' 88    `8b  //
//  `Y88888b. 88     88  //
//        `8b 88     88  //
//  d8'   .8P 88    .8P  //
//   Y88888P  8888888P   //


#pragma once
#include <Arduino.h>
#include <FS.h>

// forward-declaration to avoid including U8g2lib.h, GxEPD2_BW.h, pocketmage_oled.h, and pocketmage_eink.h
class PocketmageOled;
class PocketmageEink;

class PocketmageSD {
public:
  explicit PocketmageSD() {}

  enum Mode { SDMMC = 0, SDSPI = 1 };

  void setMode(Mode m) { mode_ = m; }
  Mode getMode() const { return mode_; }

  void saveFile();
  void writeMetadata(const String& path);
  void loadFile(bool showOLED = true);
  void delFile(String fileName);
  void deleteMetadata(String path);
  void renFile(String oldFile, String newFile);
  void renMetadata(String oldPath, String newPath);
  void copyFile(String oldFile, String newFile);
  void appendToFile(String path, String inText);

  void beginIO();
  void endIO();

  // Getters / Setters
  bool    getNoSD()          const { return noSD_; }
  void    setNoSD(bool v)          { noSD_ = v; }

  String  getWorkingFile()   const { return workingFile_; }
  void    setWorkingFile(const String& v) { workingFile_ = v; }

  String  getEditingFile()   const { return editingFile_; }
  void    setEditingFile(const String& v) { editingFile_ = v; }

  String  getFilesListIndex(int index) const { return filesList_[index]; }
  void    setFilesListIndex(int index, const String& v) { filesList_[index] = v; }

  void    listDir(fs::FS &fs, const char *dirname);
  void    readFile(fs::FS &fs, const char *path);
  String  readFileToString(fs::FS &fs, const char *path);
  void    writeFile(fs::FS &fs, const char *path, const char *message);
  void    appendFile(fs::FS &fs, const char *path, const char *message);
  void    renameFile(fs::FS &fs, const char *path1, const char *path2);
  void    deleteFile(fs::FS &fs, const char *path);
  bool    readBinaryFile(const char* path, uint8_t* buf, size_t len);
  size_t  getFileSize(const char* path);

private:
  Mode    mode_        = SDMMC;
  bool    noSD_        = false;

  String  editingFile_;
  String  workingFile_;
  String  filesList_[MAX_FILES];

  uint8_t fileIndex_        = 0;
  String  excludedFiles_[3]  = { "/temp.txt", "/settings.txt", "/tasks.txt" };

  static constexpr const char* tag = "MAGE_SD";
};

void setupSD();
PocketmageSD& PM_SD();
PocketmageSD& PM_SDAUTO();

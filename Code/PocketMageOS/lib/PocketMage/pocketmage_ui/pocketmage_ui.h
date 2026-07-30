#pragma once
#include <Arduino.h>
#include <vector>
#include <GxEPD2_BW.h>
#include <pocketmage_font/pocketmage_font.h>

enum class EinkRefresh : uint8_t {
    Normal,
    ForceFull,
    MultiPass1,
    MultiPass2,
};

void drawScrollbar(int total, int visible, int index,
                   int barX = -1, int barY = 0, int barH = -1,
                   int barW = 3, bool clearBg = false,
                   uint16_t fg = GxEPD_BLACK, uint16_t bg = GxEPD_WHITE);

void beginEinkScreen(bool preserveBg = false);
void endEinkScreen(const char* statusText, EinkRefresh mode = EinkRefresh::MultiPass2);

void drawListItem(int x, int y, const String& text, int maxWidth = -1);

void drawAppIcon(int x, int y, const uint8_t* bitmap, const String& name);

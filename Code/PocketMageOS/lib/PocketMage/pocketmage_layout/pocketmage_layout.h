#pragma once
#include <Arduino.h>
#include <vector>
#include <pocketmage_font/pocketmage_font.h>
#include <pocketmage_eink/pocketmage_eink.h>

// Display geometry (single hardware: SSD1326 256x32 OLED, GDEQ031T10 320x240 e-ink)
constexpr int kOledWidth    = 256;
constexpr int kOledHeight   = 32;
constexpr int kEinkWidth    = 320;
constexpr int kEinkHeight   = 240;
constexpr int kEinkStatusH  = 26;   // bottom status-bar band (PocketmageEink::drawStatusBar)
constexpr int kEinkContentH = kEinkHeight - kEinkStatusH;  // 214: region above the status bar

// E-ink text frames (frames.cpp)
constexpr int kFrameTextPadX  = 4;    // left text pad inside a frame (was X_OFFSET)
constexpr int kFrameCursorPad = 16;   // right/center clearance for the cursor arrow

// Canonical e-ink row pitch: font cell height + the e-ink line spacing.
inline int einkRowPitch(FontStyle s) {
  return FontEngine::fontHeight(DisplayTarget::EINK, s) + EINK().getLineSpacing();
}

// OLED primitives (pocketmage_oled.cpp)
constexpr int kOledWordBaseline      = 16;   // oledWord/sysMessage cascade center baseline
constexpr int kOledWordLargeBaseline = 21;   // oledWord allowLarge (Heading2) baseline
constexpr int kOledEditBaseline      = 20;   // oledLine input/cursor baseline
constexpr int kOledEditRightPad      = 8;    // oledLine/sysMessage right margin
constexpr int kOledEditCursorX       = 0;    // oledLine cursor column
constexpr int kOledEditCursorY       = 1;    // oledLine cursor top
constexpr int kOledEditCursorH       = 22;   // oledLine cursor height
constexpr int kOledProgressMaxW      = kEinkWidth - 5;  // reference width for the full-text progress bar
constexpr float kOledProgressFullFrac = 0.8f;          // progress fraction that triggers the "full" arrow
constexpr int kOledInfoBaseline      = kOledHeight;       // bottom info bar text baseline (Tiny)
constexpr int kOledInfoBatteryW      = 10;
constexpr int kOledInfoBatteryH      = 6;
constexpr int kOledInfoBatteryY      = kOledHeight - kOledInfoBatteryH;
constexpr int kOledInfoGap           = 6;    // gap between info bar items
constexpr int kOledInfoFirstX        = kOledInfoBatteryW + kOledInfoGap;  // x of the first text item

// OLED lock prompt lock glyph (LOCK.cpp): 16x16, top-right corner
constexpr int kOledLockGlyphW = 16;
constexpr int kOledLockGlyphH = 16;
constexpr int kOledLockGlyphY = 2;
constexpr int kOledLockGlyphX = kOledLockGlyphW + 2;  // right margin = glyph width + 2

// OLED scroll preview (PocketmageOled::oledScroll / oledScrollFrame)
constexpr int kOledScrollPreviewW   = 128;  // 128px-wide preview strip (scrolloled0)
constexpr int kOledScrollRowPitch   = 4;    // vertical pitch of preview bars
constexpr int kOledScrollBarH       = 2;    // preview bar height
constexpr int kOledScrollBaseY      = 28;   // baseline row for preview bars
constexpr int kOledScrollNormX      = 61;   // preview bar x for top-level lines
constexpr int kOledScrollTabX       = 68;   // preview bar x for tabbed lines
constexpr int kOledScrollNormMaxW   = 56;   // max preview bar width (top-level)
constexpr int kOledScrollTabMaxW    = 49;   // max preview bar width (tabbed)
constexpr int kOledScrollTextX      = 140;  // current-line preview x
constexpr int kOledScrollLabelY     = 12;   // "Lines:" label baseline
constexpr int kOledScrollValueY     = 24;   // line-number / current-line baseline

// OLED sysMessage overlay
constexpr int kOledSysMsgPadX       = 8;    // text margin inside the overlay frame
constexpr int kOledSysMsgFrameH     = kOledHeight + 16;  // overlay frame height
constexpr int kOledSysMsgRadius     = 10;   // overlay corner radius
constexpr int kOledSysMsgRaise      = 5;    // raise sysMessage text above the oledWord baseline

// App icon grid (HOME / APPLOADER)
constexpr int kIconCellSize = 40;   // app icon cell size
constexpr int kIconNameGap  = 13;   // icon-to-name baseline gap

// App-name label cascade: largest style first, all serif (no family mixing).
// Body (ncenR10) first, then BodyNarrow (timR10 - Times 10pt, measurably
// narrower at the same point size) as a middle step, then Small (ncenR08) as
// the floor.  Used by the HOME/APPLOADER grid and the SETTINGS list; each
// picks its own maxWidth.  Picks the largest style whose width fits
// (FontEngine::fitStyle); truncateWithEllipsis() handles anything that still
// overflows Small.
static constexpr FontStyle kLabelCascade[] = {
  FontStyle::Body, FontStyle::BodyNarrow, FontStyle::Small,
};
static constexpr int kLabelCascadeCount =
    sizeof(kLabelCascade) / sizeof(kLabelCascade[0]);
constexpr int kGridLabelMaxW = 60;  // max label width = grid cell pitch

// OLED scroll-preview rows (task/terminal/chat scroll previews)
constexpr int kOledPrevY0     = 7;   // first preview-row baseline
constexpr int kOledPrevPitch  = 8;   // preview row pitch
constexpr int kOledPrevRows   = 4;   // preview rows shown
constexpr int kOledPrevX      = 6;   // preview text x
constexpr int kOledPrevTriW   = 4;   // cursor triangle half-width
constexpr int kOledPrevTriH   = 3;   // cursor triangle half-height

// Find the longest prefix of s[0..n) that fits within maxTextWidth pixels
// using the given e-ink style. Word-aware: breaks at spaces.
// Returns the number of characters that fit (0 if none fit, or 1+ for newline).
size_t sliceThatFits(const char* s, size_t n, int maxTextWidth, FontStyle style);

// Truncate text to fit within maxWidthPx using the given style.  Defaults to
// the e-ink metrics; pass DisplayTarget::OLED to measure against the OLED
// font table instead.  Appends "..." when truncated and returns the shortened
// string.  The cut always lands on a UTF-8 character boundary (a multi-byte
// sequence is never split).
String truncateWithEllipsis(const String& text, int maxWidthPx, FontStyle style,
                            DisplayTarget target = DisplayTarget::EINK);

// Word-wrap text to fit within maxWidthPx using the given font style.
// Returns one string per wrapped line.
std::vector<String> wordWrap(const String& text, int maxWidthPx, FontStyle style);

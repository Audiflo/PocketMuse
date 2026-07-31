#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <U8g2_for_Adafruit_GFX.h>

// Logical text role.  A FontTable maps each role to a concrete font per display target
// (OLED / E-Ink).  Swapping the FontTable at runtime is how language / i18n support works.
enum class FontStyle : uint8_t {
  Tiny,          // u8g2_font_5x7_tf
  Body,          // ncenR10_tf     - serif body
  BodyBold,      // ncenB10_tf
  BodyItalic,    // ncenI10_tf
  BodyBoldItalic,// ncenBI10_tf
  Medium,        // ncenR12_tf     - 12pt serif for OLED
  Mono,          // courR10_tf     - monospace body
  MonoBold,      // courB10_tf
  MonoItalic,    // courI10_tf
  MonoBoldItalic,// courBI10_tf
  Sans,          // helvR10_tf     - sans body
  SansBold,      // helvB10_tf
  SansItalic,    // helvI10_tf
  SansBoldItalic,// helvBI10_tf
  SmallHeading,  // ncenB08_tf     - status bars / compact labels
  Heading3,      // ncenB12_tf     - 12pt heading
  Heading2,      // ncenB18_tf     - 18pt heading
  Heading1,      // ncenB24_tf     - 24pt heading
  Large,         // lubR18_tf      - scroll preview / large OLED text
  Status,        // ncenB08_tf     - status bar / dialog labels
  OledWord,      // ncenB14_tf     - oledWord / sysMessage cascade parent
  Terminal,      // 7x13B_tf       - terminal app
  Micro,         // 4x6_tf         - calendar event times
  TerminalBig,   // courB14_tf     - terminal wr_inkText size 3
  ClockDigit,    // luBIS14_tn     - clock/date-set OLED digits

  _StyleCount    // sentinel, must be last
};

// Which physical display a text operation targets.
enum class DisplayTarget : uint8_t {
  OLED,          // u8g2 (U8G2_SSD1326_ER_256X32_F_4W_HW_SPI)
  EINK,          // u8g2f (U8G2_FOR_ADAFRUIT_GFX on the GxEPD2 buffer)
};

// One style variant, resolved for both display targets.
struct FontEntry {
  const uint8_t* oled;     // u8g2 font pointer for OLED
  const uint8_t* eink;     // u8g2 font pointer for E-Ink
  uint8_t        height;   // full cell height in pixels
};

// TXT.cpp markdown editor matrix indices.
enum : uint8_t {
  TxtFamilySerif = 0,
  TxtFamilySans  = 1,
  TxtFamilyMono  = 2,
  TxtSizeBody    = 0,      // 10pt: body / code / quote / list
  TxtSizeH3      = 1,      // 12pt
  TxtSizeH2      = 2,      // 18pt
  TxtSizeH1      = 3,      // 24pt
  TxtVariantN    = 0,      // regular
  TxtVariantB    = 1,      // bold
  TxtVariantI    = 2,      // italic
  TxtVariantBI   = 3,      // bold + italic
};

// A complete font table for one language/region.
// The default English table is built-in; call setFontTable() to swap at runtime.
struct FontTable {
  FontEntry tiny;
  FontEntry body;
  FontEntry bodyBold;
  FontEntry bodyItalic;
  FontEntry bodyBoldItalic;
  FontEntry medium;
  FontEntry mono;
  FontEntry monoBold;
  FontEntry monoItalic;
  FontEntry monoBoldItalic;
  FontEntry sans;
  FontEntry sansBold;
  FontEntry sansItalic;
  FontEntry sansBoldItalic;
  FontEntry smallHeading;
  FontEntry heading3;
  FontEntry heading2;
  FontEntry heading1;
  FontEntry large;
  FontEntry status;
  FontEntry oledWord;
  FontEntry terminal;
  FontEntry micro;
  FontEntry terminalBig;
  FontEntry clockDigit;

  // TXT markdown editor fonts: [family][sizeIdx][variant].
  // Heading regular variants alias to their bold font (headings are already bold).
  FontEntry txt[3][4][4];
};

class FontEngine {
public:
  // Apply a font table.  Passing nullptr re-applies the built-in default.
  static void init(const FontTable* table = nullptr);
  static void setFontTable(const FontTable* table);

  // ---- Unified API ----

  // Draw UTF-8 text on target at (x, y).  y is the baseline (as in U8g2).
  static void drawText(DisplayTarget target, int x, int y,
                       const char* text, FontStyle style);
  static void drawText(DisplayTarget target, int x, int y,
                       const String& text, FontStyle style);

  // Draw a single glyph on target.
  static void drawGlyph(DisplayTarget target, int x, int y,
                        uint16_t unicode, FontStyle style);

  // Text width for a style.  Never changes the font that drawText left set.
  static int textWidth(DisplayTarget target, const char* text, FontStyle style);
  static int textWidth(DisplayTarget target, const String& text, FontStyle style);

  // Per-character width for the given style.  Cached for OLED (codepoints
  // 32-255); other codepoints and the E-Ink target are measured live.
  static int charWidth(DisplayTarget target, uint16_t unicode, FontStyle style);

  // Font metrics for a style.  First call per (target, style) measures the
  // font, then the result is cached, so layout code never switches fonts
  // merely to measure.
  static int fontHeight(DisplayTarget target, FontStyle style);
  static int fontAscent(DisplayTarget target, FontStyle style);
  static int fontDescent(DisplayTarget target, FontStyle style);

  // Stateful text color for one target.  Value semantics match both
  // u8g2.setDrawColor and u8g2f.setForegroundColor: 0 = black, 1 = white.
  // Only the requested target's color is touched; an E-ink status bar draw
  // must not change the OLED draw color and vice versa.
  static void setTextColor(DisplayTarget target, uint16_t color);

  // TXT.cpp editor helper: draw a line with per-character style switching.
  static void drawTextEditor(DisplayTarget target, int x, int y,
                             const char* text, const FontStyle* styles, int len);

  // Map (family, bold, italic, headingLevel) to a FontStyle.
  //   family 0 = serif, 1 = sans, 2 = mono
  //   headingLevel 0 = body, 1/2/3 = H1/H2/H3, 4 = code, 5 = quote, 6 = list
  static FontStyle resolveStyle(uint8_t family, bool bold,
                                bool italic, uint8_t headingLevel);

  // Raw pointer access (for edge cases / the TXT markdown renderer).
  static const uint8_t* fontPtr(DisplayTarget target, FontStyle style);

  // TXT markdown matrix lookup: font pointer for family x sizeIdx x variant.
  static const uint8_t* txtFont(DisplayTarget target,
                                uint8_t family, uint8_t sizeIdx, uint8_t variant);

private:
  static const FontEntry& entry(FontStyle s);
  static void applyFont(DisplayTarget target, FontStyle style);

  static const FontTable* table_;

  // Width cache per style (covers codepoints 32-255 only, OLED).
  static constexpr int kCacheCodepoints = 256;
  struct WidthCache {
    bool     valid = false;
    uint8_t  widths[kCacheCodepoints];
  };
  static WidthCache widthCache_[static_cast<int>(FontStyle::_StyleCount)];

  // Lazily-measured font metrics per style and target.
  struct Metrics {
    bool     valid = false;
    uint8_t  ascent;
    uint8_t  descent;
  };
  static Metrics metrics_[static_cast<int>(FontStyle::_StyleCount)][2];

  static void buildWidthCache(FontStyle onStyle);
  static void buildMetrics(DisplayTarget target, FontStyle onStyle);
};

// Built-in English font table
extern const FontTable kDefaultFontTable;

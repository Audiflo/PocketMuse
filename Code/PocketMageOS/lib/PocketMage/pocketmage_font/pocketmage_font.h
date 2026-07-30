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

  _StyleCount    // sentinel, must be last
};

// One style variant, resolved for both display targets.
struct FontEntry {
  const uint8_t* oled;     // u8g2 font pointer for OLED
  const uint8_t* eink;     // u8g2 font pointer for E-Ink
  uint8_t        height;   // full cell height in pixels (ascent - descent)
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
};

class FontEngine {
public:
  // Apply a font table.  Passing nullptr re-applies the built-in default.
  static void init(const FontTable* table = nullptr);
  static void setFontTable(const FontTable* table);

  // OLED operations
  // Select a style on the OLED display.  Subsequent oledDraw* / oledTextWidth
  // calls use this style until changed.
  static void setOledStyle(FontStyle style);

  // Draw UTF-8 text on OLED at (x, y).  y is the baseline (as in U8g2).
  static void oledDraw(int x, int y, const char* text);
  static void oledDraw(int x, int y, const String& text);

  // Draw a single glyph on OLED.
  static void oledDrawGlyph(int x, int y, uint16_t unicode);

  // Text width in the currently-set OLED style.
  static int oledTextWidth(const char* text);
  static int oledTextWidth(const String& text);

  // Text width for a specific style (without changing the active style).
  static int oledTextWidth(FontStyle style, const char* text);
  static int oledTextWidth(FontStyle style, const String& text);

  // Cached per-character width for the given style.  Builds the cache on
  // first use for that style.  Cache covers codepoints 32-255; points
  // outside that range are measured live and not cached.
  static int oledCharWidth(uint16_t unicode, FontStyle style);

  // Font metrics for the currently-set OLED style.
  static int oledFontHeight();
  static int oledFontAscent();
  static int oledFontDescent();

  // Convenience: resolve the FontStyle for the TXT.cpp editor's type/bold/italic
  // combos and draw a line with per-character style switching.
  static void oledDrawEditor(int x, int y, const char* text,
                              const FontStyle* styles, int len);

  // E-Ink operations
  static void setEinkStyle(FontStyle style);
  static void setEinkColor(uint16_t color);
  static void einkDraw(int x, int y, const char* text);
  static void einkDraw(int x, int y, const String& text);
  static int  einkTextWidth(const char* text);
  static int  einkTextWidth(const String& text);

  // Measure in a specific style without changing the active E-Ink style.
  static int  einkTextWidth(FontStyle style, const char* text);
  static int  einkTextWidth(FontStyle style, const String& text);
  static int  einkFontHeight();
  static int  einkFontAscent();
  static int  einkFontDescent();

  // TXT.cpp editor helpers
  // Map (family, bold, italic, headingLevel) to a FontStyle.
  //   family 0 = serif, 1 = sans, 2 = mono
  //   headingLevel 0 = body, 1/2/3 = H1/H2/H3, 4 = code, 5 = quote, 6 = list
  static FontStyle resolveStyle(uint8_t family, bool bold,
                                 bool italic, uint8_t headingLevel);

  // Raw pointer access (for edge cases)
  static const uint8_t* oledFontPtr(FontStyle style);
  static const uint8_t* einkFontPtr(FontStyle style);

private:
  static const FontEntry& entry(FontStyle s);
  static const FontTable* table_;
  static FontStyle        oledActiveStyle_;
  static FontStyle        einkActiveStyle_;

  // Width cache per style (covers codepoints 32-255 only).
  static constexpr int kCacheCodepoints = 256;
  struct WidthCache {
    bool     valid = false;
    uint8_t  widths[kCacheCodepoints];
  };
  static WidthCache widthCache_[static_cast<int>(FontStyle::_StyleCount)];

  // Build the width cache entry for onStyle.
  static void buildWidthCache(FontStyle onStyle);
};

// Built-in English font table
extern const FontTable kDefaultFontTable;

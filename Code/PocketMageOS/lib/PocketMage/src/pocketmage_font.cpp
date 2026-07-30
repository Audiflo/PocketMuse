#include <pocketmage_font.h>

extern U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2;
extern U8G2_FOR_ADAFRUIT_GFX u8g2f;

// Default English font table
const FontTable kDefaultFontTable = {
  // tiny
  { u8g2_font_5x7_tf,    u8g2_font_5x7_tf,    8 },
  // body
  { u8g2_font_ncenR10_tf, u8g2_font_ncenR10_tf, 14 },
  // bodyBold
  { u8g2_font_ncenB10_tf, u8g2_font_ncenB10_tf, 14 },
  // bodyItalic
  { u8g2_font_ncenI10_tf, u8g2_font_ncenI10_tf, 14 },
  // bodyBoldItalic
  { u8g2_font_ncenBI10_tf, u8g2_font_ncenBI10_tf, 14 },
  // mono
  { u8g2_font_courR10_tf, u8g2_font_courR10_tf, 14 },
  // monoBold
  { u8g2_font_courB10_tf, u8g2_font_courB10_tf, 14 },
  // monoItalic
  { u8g2_font_courI10_tf, u8g2_font_courI10_tf, 14 },
  // monoBoldItalic
  { u8g2_font_courBI10_tf, u8g2_font_courBI10_tf, 14 },
  // sans
  { u8g2_font_helvR10_tf, u8g2_font_helvR10_tf, 14 },
  // sansBold
  { u8g2_font_helvB10_tf, u8g2_font_helvB10_tf, 14 },
  // sansItalic
  { u8g2_font_helvI10_tf, u8g2_font_helvI10_tf, 14 },
  // sansBoldItalic
  { u8g2_font_helvBI10_tf, u8g2_font_helvBI10_tf, 14 },
  // smallHeading (ncenB08)
  { u8g2_font_ncenB08_tf, u8g2_font_ncenB08_tf, 10 },
  // heading3 (ncenB12)
  { u8g2_font_ncenB12_tf, u8g2_font_ncenB12_tf, 16 },
  // heading2 (ncenB18)
  { u8g2_font_ncenB18_tf, u8g2_font_ncenB18_tf, 22 },
  // heading1 (ncenB24)
  { u8g2_font_ncenB24_tf, u8g2_font_ncenB24_tf, 30 },
  // large (lubR18)
  { u8g2_font_lubR18_tf,  u8g2_font_lubR18_tf,  22 },
  // status (ncenB08)
  { u8g2_font_ncenB08_tf, u8g2_font_ncenB08_tf, 10 },
  // oledWord cascade base (ncenB14)
  { u8g2_font_ncenB14_tf, u8g2_font_ncenB14_tf, 18 },
  // terminal (7x13B)
  { u8g2_font_7x13B_tf,   u8g2_font_7x13B_tf,   13 },
};

// Static state
const FontTable* FontEngine::table_          = &kDefaultFontTable;
FontStyle        FontEngine::oledActiveStyle_ = FontStyle::Tiny;
FontStyle        FontEngine::einkActiveStyle_ = FontStyle::Body;
FontEngine::WidthCache FontEngine::widthCache_[static_cast<int>(FontStyle::_StyleCount)];

// Internal helpers
const FontEntry& FontEngine::entry(FontStyle s) {
  // Map the style enum to the FontEntry inside the active table.
  // The enum values MUST match the struct field order in FontTable.
  static_assert(static_cast<int>(FontStyle::_StyleCount) == 21,
                "FontStyle enum count must match FontTable field count");
  const FontTable& t = *table_;
  switch (s) {
    case FontStyle::Tiny:          return t.tiny;
    case FontStyle::Body:          return t.body;
    case FontStyle::BodyBold:      return t.bodyBold;
    case FontStyle::BodyItalic:    return t.bodyItalic;
    case FontStyle::BodyBoldItalic:return t.bodyBoldItalic;
    case FontStyle::Mono:          return t.mono;
    case FontStyle::MonoBold:      return t.monoBold;
    case FontStyle::MonoItalic:    return t.monoItalic;
    case FontStyle::MonoBoldItalic:return t.monoBoldItalic;
    case FontStyle::Sans:          return t.sans;
    case FontStyle::SansBold:      return t.sansBold;
    case FontStyle::SansItalic:    return t.sansItalic;
    case FontStyle::SansBoldItalic:return t.sansBoldItalic;
    case FontStyle::SmallHeading:  return t.smallHeading;
    case FontStyle::Heading3:      return t.heading3;
    case FontStyle::Heading2:      return t.heading2;
    case FontStyle::Heading1:      return t.heading1;
    case FontStyle::Large:         return t.large;
    case FontStyle::Status:        return t.status;
    case FontStyle::OledWord:      return t.oledWord;
    case FontStyle::Terminal:      return t.terminal;
    default:                       return t.body;
  }
}

void FontEngine::buildWidthCache(FontStyle onStyle) {
  WidthCache& cache = widthCache_[static_cast<int>(onStyle)];
  const uint8_t* font = entry(onStyle).oled;
  if (!font) { cache.valid = true; return; }

  u8g2.setFont(font);
  char tmp[4] = {};
  for (int i = 32; i < 256; i++) {
    if (i < 128) {
      tmp[0] = static_cast<char>(i);
      tmp[1] = 0;
    } else {
      tmp[0] = static_cast<char>(0xC0 | (i >> 6));
      tmp[1] = static_cast<char>(0x80 | (i & 0x3F));
      tmp[2] = 0;
    }
    cache.widths[i] = static_cast<uint8_t>(u8g2.getUTF8Width(tmp));
  }
  cache.valid = true;
}

// Lifecycle
void FontEngine::init(const FontTable* table) {
  setFontTable(table ? table : &kDefaultFontTable);
  // Reset width caches
  for (auto& c : widthCache_) c.valid = false;
  // Transparent background by default
  u8g2.setFontMode(1);
  u8g2f.setFontMode(1);
  // Set initial styles
  setOledStyle(FontStyle::Tiny);
  setEinkStyle(FontStyle::Body);
}

void FontEngine::setFontTable(const FontTable* table) {
  table_ = table ? table : &kDefaultFontTable;
  // Invalidate all width caches so they rebuild with the new fonts
  for (auto& c : widthCache_) c.valid = false;
}

// OLED
void FontEngine::setOledStyle(FontStyle style) {
  oledActiveStyle_ = style;
  const FontEntry& e = entry(style);
  if (e.oled) {
    u8g2.setFont(e.oled);
    u8g2.setFontMode(1);
  }
}

void FontEngine::oledDraw(int x, int y, const char* text) {
  u8g2.drawUTF8(x, y, text);
}

void FontEngine::oledDraw(int x, int y, const String& text) {
  u8g2.drawUTF8(x, y, text.c_str());
}

void FontEngine::oledDrawGlyph(int x, int y, uint16_t unicode) {
  u8g2.drawGlyph(x, y, unicode);
}

int FontEngine::oledTextWidth(const char* text) {
  return u8g2.getUTF8Width(text);
}

int FontEngine::oledTextWidth(const String& text) {
  return u8g2.getUTF8Width(text.c_str());
}

int FontEngine::oledTextWidth(FontStyle style, const char* text) {
  FontStyle saved = oledActiveStyle_;
  setOledStyle(style);
  int w = oledTextWidth(text);
  setOledStyle(saved);
  return w;
}

int FontEngine::oledTextWidth(FontStyle style, const String& text) {
  return oledTextWidth(style, text.c_str());
}

int FontEngine::oledCharWidth(uint16_t unicode, FontStyle style) {
  if (unicode < 32 || unicode >= 256) return 0;
  WidthCache& cache = widthCache_[static_cast<int>(style)];
  if (!cache.valid) buildWidthCache(style);
  return cache.widths[unicode];
}

int FontEngine::oledFontHeight() {
  return u8g2.getFontAscent() - u8g2.getFontDescent();
}
int FontEngine::oledFontAscent() {
  return u8g2.getFontAscent();
}
int FontEngine::oledFontDescent() {
  return u8g2.getFontDescent();
}

void FontEngine::oledDrawEditor(int x, int y, const char* text,
                                 const FontStyle* styles, int len) {
  // Walk the string and apply per-character styles.
  // This is used by the TXT.cpp editor OLED line renderer.
  int xpos = x;
  for (int i = 0; i < len; i++) {
    setOledStyle(styles[i]);
    uint16_t unicode = static_cast<uint8_t>(text[i]);
    oledDrawGlyph(xpos, y, unicode);
    xpos += oledCharWidth(unicode, styles[i]);
  }
}

// E-Ink
void FontEngine::setEinkStyle(FontStyle style) {
  einkActiveStyle_ = style;
  const FontEntry& e = entry(style);
  if (e.eink) {
    u8g2f.setFont(e.eink);
    u8g2f.setFontMode(1);
  }
}

void FontEngine::setEinkColor(uint16_t color) {
  u8g2f.setForegroundColor(color);
}

void FontEngine::einkDraw(int x, int y, const char* text) {
  u8g2f.drawUTF8(x, y, text);
}

void FontEngine::einkDraw(int x, int y, const String& text) {
  u8g2f.drawUTF8(x, y, text.c_str());
}

int FontEngine::einkTextWidth(const char* text) {
  return u8g2f.getUTF8Width(text);
}

int FontEngine::einkTextWidth(const String& text) {
  return u8g2f.getUTF8Width(text.c_str());
}

int FontEngine::einkTextWidth(FontStyle style, const char* text) {
  FontStyle saved = einkActiveStyle_;
  setEinkStyle(style);
  int w = einkTextWidth(text);
  setEinkStyle(saved);
  return w;
}

int FontEngine::einkTextWidth(FontStyle style, const String& text) {
  return einkTextWidth(style, text.c_str());
}

int FontEngine::einkFontHeight() {
  return u8g2f.getFontAscent() - u8g2f.getFontDescent();
}
int FontEngine::einkFontAscent() {
  return u8g2f.getFontAscent();
}
int FontEngine::einkFontDescent() {
  return u8g2f.getFontDescent();
}

// TXT.cpp editor helpers
FontStyle FontEngine::resolveStyle(uint8_t family, bool bold,
                                    bool italic, uint8_t headingLevel) {
  // headingLevel: 0=body, 1=H1, 2=H2, 3=H3, 4=code, 5=quote, 6=list
  if (headingLevel == 4) {
    if (bold && italic) return FontStyle::MonoBoldItalic;
    if (bold)           return FontStyle::MonoBold;
    if (italic)         return FontStyle::MonoItalic;
    return FontStyle::Mono;
  }

  if (headingLevel == 1) {
    if (family == 0) {
      if (bold && italic) return FontStyle::BodyBoldItalic; // H1_BI
      return FontStyle::Heading1;
    }
    if (family == 2) return FontStyle::Heading1;
    return FontStyle::Heading1;
  }
  if (headingLevel == 2) {
    return FontStyle::Heading2;
  }
  if (headingLevel == 3) {
    return FontStyle::Heading3;
  }

  // Body text (headingLevel == 0) or quote/list
  FontStyle base;
  switch (family) {
    case 1: base = FontStyle::Sans; break;
    case 2: base = FontStyle::Mono; break;
    default: base = FontStyle::Body; break;
  }

  if (bold && italic) {
    base = static_cast<FontStyle>(static_cast<int>(base) + 3);
  } else if (bold) {
    base = static_cast<FontStyle>(static_cast<int>(base) + 1);
  } else if (italic) {
    base = static_cast<FontStyle>(static_cast<int>(base) + 2);
  }
  return base;
}

// Raw pointer access
const uint8_t* FontEngine::oledFontPtr(FontStyle style) {
  return entry(style).oled;
}

const uint8_t* FontEngine::einkFontPtr(FontStyle style) {
  return entry(style).eink;
}

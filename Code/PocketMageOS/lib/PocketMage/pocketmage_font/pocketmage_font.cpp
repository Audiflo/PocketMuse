#include "pocketmage_font.h"

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
  // medium
  { u8g2_font_ncenR12_tf,  u8g2_font_ncenR12_tf,  16 },
  // small (ncenR08)
  { u8g2_font_ncenR08_tf,  u8g2_font_ncenR08_tf,  10 },
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
  // caption (ncenB08)
  { u8g2_font_ncenB08_tf, u8g2_font_ncenB08_tf, 10 },
  // heading3 (ncenB12)
  { u8g2_font_ncenB12_tf, u8g2_font_ncenB12_tf, 16 },
  // heading2 (ncenB18)
  { u8g2_font_ncenB18_tf, u8g2_font_ncenB18_tf, 22 },
  // heading1 (ncenB24)
  { u8g2_font_ncenB24_tf, u8g2_font_ncenB24_tf, 30 },
  // large (lubR18)
  { u8g2_font_lubR18_tf,  u8g2_font_lubR18_tf,  22 },
  // oledWord cascade base (ncenB14)
  { u8g2_font_ncenB14_tf, u8g2_font_ncenB14_tf, 18 },
  // terminal (7x13B)
  { u8g2_font_7x13B_tf,   u8g2_font_7x13B_tf,   13 },
  // terminalBig (courB14)
  { u8g2_font_courB14_tf, u8g2_font_courB14_tf, 18 },
  // clockDigit (luBIS14, non-transparent)
  { u8g2_font_luBIS14_tn, u8g2_font_luBIS14_tn, 18 },

  // TXT markdown matrix: [family][sizeIdx][variant]
  {
  {   // family 0 = serif (ncen)
    {   // sizeIdx 0 = 10pt (body/code/quote/list)
      { u8g2_font_ncenR10_tf,  u8g2_font_ncenR10_tf,  14 },   // N
      { u8g2_font_ncenB10_tf,  u8g2_font_ncenB10_tf,  14 },   // B
      { u8g2_font_ncenI10_tf,  u8g2_font_ncenI10_tf,  14 },   // I
      { u8g2_font_ncenBI10_tf, u8g2_font_ncenBI10_tf, 14 },   // BI
    },
    {   // sizeIdx 1 = 12pt (h3)
      { u8g2_font_ncenB12_tf,  u8g2_font_ncenB12_tf,  16 },
      { u8g2_font_ncenB12_tf,  u8g2_font_ncenB12_tf,  16 },
      { u8g2_font_ncenBI12_tf, u8g2_font_ncenBI12_tf, 16 },
      { u8g2_font_ncenBI12_tf, u8g2_font_ncenBI12_tf, 16 },
    },
    {   // sizeIdx 2 = 18pt (h2)
      { u8g2_font_ncenB18_tf,  u8g2_font_ncenB18_tf,  22 },
      { u8g2_font_ncenB18_tf,  u8g2_font_ncenB18_tf,  22 },
      { u8g2_font_ncenBI18_tf, u8g2_font_ncenBI18_tf, 22 },
      { u8g2_font_ncenBI18_tf, u8g2_font_ncenBI18_tf, 22 },
    },
    {   // sizeIdx 3 = 24pt (h1)
      { u8g2_font_ncenB24_tf,  u8g2_font_ncenB24_tf,  30 },
      { u8g2_font_ncenB24_tf,  u8g2_font_ncenB24_tf,  30 },
      { u8g2_font_ncenBI24_tf, u8g2_font_ncenBI24_tf, 30 },
      { u8g2_font_ncenBI24_tf, u8g2_font_ncenBI24_tf, 30 },
    },
  },

  {   // family 1 = sans (helv)
    {   // sizeIdx 0 = 10pt
      { u8g2_font_helvR10_tf,  u8g2_font_helvR10_tf,  14 },
      { u8g2_font_helvB10_tf,  u8g2_font_helvB10_tf,  14 },
      { u8g2_font_helvI10_tf,  u8g2_font_helvI10_tf,  14 },
      { u8g2_font_helvBI10_tf, u8g2_font_helvBI10_tf, 14 },
    },
    {   // sizeIdx 1 = 12pt (h3)
      { u8g2_font_helvB12_tf,  u8g2_font_helvB12_tf,  16 },
      { u8g2_font_helvB12_tf,  u8g2_font_helvB12_tf,  16 },
      { u8g2_font_helvBI12_tf, u8g2_font_helvBI12_tf, 16 },
      { u8g2_font_helvBI12_tf, u8g2_font_helvBI12_tf, 16 },
    },
    {   // sizeIdx 2 = 18pt (h2)
      { u8g2_font_helvB18_tf,  u8g2_font_helvB18_tf,  22 },
      { u8g2_font_helvB18_tf,  u8g2_font_helvB18_tf,  22 },
      { u8g2_font_helvBI18_tf, u8g2_font_helvBI18_tf, 22 },
      { u8g2_font_helvBI18_tf, u8g2_font_helvBI18_tf, 22 },
    },
    {   // sizeIdx 3 = 24pt (h1)
      { u8g2_font_helvB24_tf,  u8g2_font_helvB24_tf,  30 },
      { u8g2_font_helvB24_tf,  u8g2_font_helvB24_tf,  30 },
      { u8g2_font_helvBI24_tf, u8g2_font_helvBI24_tf, 30 },
      { u8g2_font_helvBI24_tf, u8g2_font_helvBI24_tf, 30 },
    },
  },

  {   // family 2 = mono (cour)
    {   // sizeIdx 0 = 10pt
      { u8g2_font_courR10_tf,  u8g2_font_courR10_tf,  14 },
      { u8g2_font_courB10_tf,  u8g2_font_courB10_tf,  14 },
      { u8g2_font_courI10_tf,  u8g2_font_courI10_tf,  14 },
      { u8g2_font_courBI10_tf, u8g2_font_courBI10_tf, 14 },
    },
    {   // sizeIdx 1 = 12pt (h3)
      { u8g2_font_courB12_tf,  u8g2_font_courB12_tf,  16 },
      { u8g2_font_courB12_tf,  u8g2_font_courB12_tf,  16 },
      { u8g2_font_courBI12_tf, u8g2_font_courBI12_tf, 16 },
      { u8g2_font_courBI12_tf, u8g2_font_courBI12_tf, 16 },
    },
    {   // sizeIdx 2 = 18pt (h2)
      { u8g2_font_courB18_tf,  u8g2_font_courB18_tf,  22 },
      { u8g2_font_courB18_tf,  u8g2_font_courB18_tf,  22 },
      { u8g2_font_courBI18_tf, u8g2_font_courBI18_tf, 22 },
      { u8g2_font_courBI18_tf, u8g2_font_courBI18_tf, 22 },
    },
    {   // sizeIdx 3 = 24pt (h1)
      { u8g2_font_courB24_tf,  u8g2_font_courB24_tf,  30 },
      { u8g2_font_courB24_tf,  u8g2_font_courB24_tf,  30 },
      { u8g2_font_courBI24_tf, u8g2_font_courBI24_tf, 30 },
      { u8g2_font_courBI24_tf, u8g2_font_courBI24_tf, 30 },
    },
  },
  }
};

// Static state
const FontTable* FontEngine::table_          = &kDefaultFontTable;
FontEngine::WidthCache FontEngine::widthCache_[static_cast<int>(FontStyle::_StyleCount)];
FontEngine::Metrics FontEngine::metrics_[static_cast<int>(FontStyle::_StyleCount)][2];

// Internal helpers
const FontEntry& FontEngine::entry(FontStyle s) {
  // Map the style enum to the FontEntry inside the active table.
  // The enum values MUST match the struct field order in FontTable.
  static_assert(static_cast<int>(FontStyle::_StyleCount) == 24,
                "FontStyle enum count must match FontTable field count");
  const FontTable& t = *table_;
  switch (s) {
    case FontStyle::Tiny:          return t.tiny;
    case FontStyle::Body:          return t.body;
    case FontStyle::BodyBold:      return t.bodyBold;
    case FontStyle::BodyItalic:    return t.bodyItalic;
    case FontStyle::BodyBoldItalic:return t.bodyBoldItalic;
    case FontStyle::Medium:        return t.medium;
    case FontStyle::Small:         return t.small;
    case FontStyle::Mono:          return t.mono;
    case FontStyle::MonoBold:      return t.monoBold;
    case FontStyle::MonoItalic:    return t.monoItalic;
    case FontStyle::MonoBoldItalic:return t.monoBoldItalic;
    case FontStyle::Sans:          return t.sans;
    case FontStyle::SansBold:      return t.sansBold;
    case FontStyle::SansItalic:    return t.sansItalic;
    case FontStyle::SansBoldItalic:return t.sansBoldItalic;
    case FontStyle::Caption:       return t.caption;
    case FontStyle::Heading3:      return t.heading3;
    case FontStyle::Heading2:      return t.heading2;
    case FontStyle::Heading1:      return t.heading1;
    case FontStyle::Large:         return t.large;
    case FontStyle::OledWord:      return t.oledWord;
    case FontStyle::Terminal:      return t.terminal;
    case FontStyle::TerminalBig:   return t.terminalBig;
    case FontStyle::ClockDigit:    return t.clockDigit;
    default:                       return t.body;
  }
}

void FontEngine::applyEntry(DisplayTarget target, const FontEntry& e) {
  if (target == DisplayTarget::OLED) {
    if (e.oled) {
      u8g2.setFont(e.oled);
      u8g2.setFontMode(1);
    }
  } else {
    if (e.eink) {
      u8g2f.setFont(e.eink);
      u8g2f.setFontMode(1);
    }
  }
}

void FontEngine::applyFont(DisplayTarget target, FontStyle style) {
  applyEntry(target, entry(style));
}

void FontEngine::encodeUtf8(uint16_t unicode, char* out) {
  if (unicode < 0x80) {
    out[0] = static_cast<char>(unicode);
    out[1] = 0;
  } else if (unicode < 0x800) {
    out[0] = static_cast<char>(0xC0 | (unicode >> 6));
    out[1] = static_cast<char>(0x80 | (unicode & 0x3F));
    out[2] = 0;
  } else {
    out[0] = static_cast<char>(0xE0 | (unicode >> 12));
    out[1] = static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
    out[2] = static_cast<char>(0x80 | (unicode & 0x3F));
    out[3] = 0;
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

void FontEngine::buildMetrics(DisplayTarget target, FontStyle onStyle) {
  Metrics& m = metrics_[static_cast<int>(onStyle)][static_cast<int>(target)];
  applyFont(target, onStyle);
  if (target == DisplayTarget::OLED) {
    m.ascent  = static_cast<uint8_t>(u8g2.getFontAscent());
    m.descent = static_cast<uint8_t>(-u8g2.getFontDescent());
  } else {
    m.ascent  = static_cast<uint8_t>(u8g2f.getFontAscent());
    m.descent = static_cast<uint8_t>(-u8g2f.getFontDescent());
  }
  m.valid = true;
}

// Lifecycle
void FontEngine::init(const FontTable* table) {
  table_ = table ? table : &kDefaultFontTable;
  // Reset width caches and metrics
  for (auto& c : widthCache_) c.valid = false;
  for (auto& row : metrics_) for (auto& m : row) m.valid = false;
  // Transparent background by default
  u8g2.setFontMode(1);
  u8g2f.setFontMode(1);
}

// ---- Unified API ----

void FontEngine::drawText(DisplayTarget target, int x, int y,
                          const char* text, FontStyle style) {
  applyFont(target, style);
  if (target == DisplayTarget::OLED) {
    u8g2.drawUTF8(x, y, text);
  } else {
    u8g2f.drawUTF8(x, y, text);
  }
}

void FontEngine::drawText(DisplayTarget target, int x, int y,
                          const String& text, FontStyle style) {
  drawText(target, x, y, text.c_str(), style);
}

void FontEngine::drawGlyph(DisplayTarget target, int x, int y,
                           uint16_t unicode, FontStyle style) {
  applyFont(target, style);
  if (target == DisplayTarget::OLED) {
    u8g2.drawGlyph(x, y, unicode);
  } else {
    u8g2f.drawGlyph(x, y, unicode);
  }
}

int FontEngine::textWidth(DisplayTarget target, const char* text, FontStyle style) {
  applyFont(target, style);
  if (target == DisplayTarget::OLED) {
    return u8g2.getUTF8Width(text);
  }
  return u8g2f.getUTF8Width(text);
}

int FontEngine::textWidth(DisplayTarget target, const String& text, FontStyle style) {
  return textWidth(target, text.c_str(), style);
}

FontStyle FontEngine::fitStyle(DisplayTarget target, const char* text, int maxWidth,
                               const FontStyle* cascade, int count) {
  for (int i = 0; i < count; i++) {
    if (textWidth(target, text, cascade[i]) <= maxWidth) return cascade[i];
  }
  return cascade[count - 1];
}

int FontEngine::charWidth(DisplayTarget target, uint16_t unicode, FontStyle style) {
  if (target == DisplayTarget::OLED && unicode >= 32 && unicode < 256) {
    WidthCache& cache = widthCache_[static_cast<int>(style)];
    if (!cache.valid) buildWidthCache(style);
    return cache.widths[unicode];
  }

  // Live measure: out-of-range codepoints on OLED, everything on E-Ink.
  char utf8[5] = {};
  encodeUtf8(unicode, utf8);
  applyFont(target, style);
  if (target == DisplayTarget::OLED) {
    return u8g2.getUTF8Width(utf8);
  }
  return u8g2f.getUTF8Width(utf8);
}

int FontEngine::fontHeight(DisplayTarget target, FontStyle style) {
  return fontAscent(target, style) + fontDescent(target, style);
}

int FontEngine::fontAscent(DisplayTarget target, FontStyle style) {
  Metrics& m = metrics_[static_cast<int>(style)][static_cast<int>(target)];
  if (!m.valid) buildMetrics(target, style);
  return m.ascent;
}

int FontEngine::fontDescent(DisplayTarget target, FontStyle style) {
  Metrics& m = metrics_[static_cast<int>(style)][static_cast<int>(target)];
  if (!m.valid) buildMetrics(target, style);
  return m.descent;
}

void FontEngine::setTextColor(DisplayTarget target, uint16_t color) {
  if (target == DisplayTarget::OLED) {
    u8g2.setDrawColor(color);
  } else {
    u8g2f.setForegroundColor(color);
  }
}

const FontEntry& FontEngine::txtEntry(uint8_t family, uint8_t sizeIdx, uint8_t variant) {
  if (family > 2)  family = 0;
  if (sizeIdx > 3) sizeIdx = 0;
  if (variant > 3) variant = 0;
  return table_->txt[family][sizeIdx][variant];
}

void FontEngine::drawTextTxt(DisplayTarget target, int x, int y, const char* text,
                             uint8_t family, uint8_t sizeIdx, uint8_t variant) {
  applyEntry(target, txtEntry(family, sizeIdx, variant));
  if (target == DisplayTarget::OLED) {
    u8g2.drawUTF8(x, y, text);
  } else {
    u8g2f.drawUTF8(x, y, text);
  }
}

void FontEngine::drawGlyphTxt(DisplayTarget target, int x, int y, uint16_t unicode,
                              uint8_t family, uint8_t sizeIdx, uint8_t variant) {
  applyEntry(target, txtEntry(family, sizeIdx, variant));
  if (target == DisplayTarget::OLED) {
    u8g2.drawGlyph(x, y, unicode);
  } else {
    u8g2f.drawGlyph(x, y, unicode);
  }
}

int FontEngine::textWidthTxt(DisplayTarget target, const char* text,
                             uint8_t family, uint8_t sizeIdx, uint8_t variant) {
  applyEntry(target, txtEntry(family, sizeIdx, variant));
  if (target == DisplayTarget::OLED) {
    return u8g2.getUTF8Width(text);
  }
  return u8g2f.getUTF8Width(text);
}

int FontEngine::charWidthTxt(DisplayTarget target, uint16_t unicode,
                             uint8_t family, uint8_t sizeIdx, uint8_t variant) {
  char utf8[5] = {};
  encodeUtf8(unicode, utf8);
  applyEntry(target, txtEntry(family, sizeIdx, variant));
  if (target == DisplayTarget::OLED) {
    return u8g2.getUTF8Width(utf8);
  }
  return u8g2f.getUTF8Width(utf8);
}

int FontEngine::fontHeightTxt(uint8_t family, uint8_t sizeIdx, uint8_t variant) {
  return txtEntry(family, sizeIdx, variant).height;
}

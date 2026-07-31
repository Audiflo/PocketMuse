#pragma once
#include <Arduino.h>
#include "pocketmage_i18n_gen.h"

// Language / string-table support.
//
// Every user-visible string in the OS is a StringID index into a per-language
// table.  Language switches at runtime via setLanguage().
//
// The Lang and StringID enums plus the per-language string tables are
// generated from the .po/.aliases catalogs in this directory.
//
// Accented input is folded (UTF-8 diacritics -> ASCII, lowercased) so both
// "réglages" and "reglages" resolve to settings.  Non-LATIN-1 multi-byte
// characters (curly quotes, dashes) and truncated sequences are dropped from
// the folded output.  Only command VERBS are folded and aliased; argument text
// (filenames) is always preserved byte-for-byte.

class I18n {
public:
  // Switch the active string table.  Out-of-range values clamp to English.
  static void setLanguage(Lang lang);

  // Switch by two-letter code ("en" / "fr" / "es").  Returns false and leaves
  // the current language unchanged when the code is unknown.
  static bool setLanguageByCode(const char* code);

  static Lang language();
  static int languageCount();

  // Two-letter code of the active (or indexed) language.
  static const char* code();
  static const char* code(int idx);

  // Native display name ("English" / "Français" / "Español").
  static const char* nativeName();
  static const char* nativeName(int idx);

  // Look up the active-language text for a string ID.
  static const char* get(StringID id);

  // Indexed helpers backed by the active table (enum blocks in the catalog).
  static const char* monthName(int month);   // 1..12, else the ERR entry
  static const char* dayName(int idx);       // 0..6, Sunday-first
  static const char* appName(int idx);       // home grid label, 0..10
  static const char* kbAppName(int idx);     // app-switcher badge, 0..11

  // Resolve native-language command aliases.  Folds diacritics, replaces the
  // first word (or the whole line, for multi-word aliases such as the easter
  // eggs) with the canonical English command, and preserves any argument text
  // byte-for-byte.  Returns the input unchanged when nothing matches.
  static String normalizeCommand(const String& raw);

private:
  static Lang lang_;
  static const char* aliasFor(const char* folded);
  static String fold(const String& s);
};

// Shorthand for call sites: TR(STR_X) == I18n::get(STR_X).
#define TR(id) I18n::get(id)

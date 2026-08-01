#!/usr/bin/env python3
"""Generate the pocketmage_i18n enum and string tables from .po catalogs.

Single source of truth:
  lib/PocketMage/pocketmage_i18n/languages/en/pocketmage_i18n.po
      order + English
  lib/PocketMage/pocketmage_i18n/languages/<lang>/pocketmage_i18n.po
  lib/PocketMage/pocketmage_i18n/languages/<lang>/pocketmage_i18n.aliases

The .po msgctxt is the StringID enum member name.  File order in en.po is the
enum order.  The four indexed helper ranges are pinned to explicit enum values
so inserting strings above them never shifts appName()/dayName()/monthName()/
kbAppName() indexing.

Run directly or from platformio.ini extra_scripts.  Exit non-zero on any
validation failure so broken catalogs abort the build.
"""

import os
import re
import sys
import unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
I18N_DIR = os.path.join(ROOT, "lib", "PocketMage", "pocketmage_i18n")
LANG_DIR = os.path.join(I18N_DIR, "languages")
GEN_H = os.path.join(I18N_DIR, "pocketmage_i18n_gen.h")
GEN_CPP = os.path.join(I18N_DIR, "pocketmage_i18n_gen.cpp")

# Indexed helper ranges (head, tail).  Heads get explicit = N enum values.
HELPER_RANGES = [
    ("STR_KB_APP_CANCEL", "STR_KB_APP_LOADER"),
    ("STR_GRID_TXT", "STR_GRID_LOADER"),
    ("STR_DAY_SUNDAY", "STR_DAY_SATURDAY"),
    ("STR_MONTH_JAN", "STR_MONTH_ERR"),
]

ID_RE = re.compile(r"^[A-Z][A-Z0-9_]*$")


def error(msg):
    print(f"[gen_i18n] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def parse_po(path):
    """Parse a gettext .po file into entries.

    Returns (languages, entries) where entries is a list of dicts:
      {ctxt, msgid, msgstr, comments:[...]}
    Comments are `#`/`#.` lines directly above an entry.  msgid/msgstr may span
    multiple quoted lines (concatenated, escapes handled).
    """
    with open(path, encoding="utf-8") as fh:
        text = fh.read()

    entries = []
    cur = None
    pending_comments = []
    languages = None
    last_field = None

    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            continue
        if line.startswith("#"):
            pending_comments.append(line.lstrip("# ").strip())
            continue
        if line.startswith("msgctxt "):
            cur = {"ctxt": parse_quoted(line[len("msgctxt "):]),
                   "msgid": "", "msgstr": "", "comments": list(pending_comments)}
            pending_comments = []
            last_field = "msgid"
            entries.append(cur)
        elif line.startswith("msgid "):
            if cur is None:
                cur = {"ctxt": None, "msgid": "", "msgstr": "",
                       "comments": list(pending_comments)}
                entries.append(cur)
            cur["msgid"] += parse_quoted(line[len("msgid "):])
            last_field = "msgid"
        elif line.startswith("msgstr "):
            if cur is None:
                cur = {"ctxt": None, "msgid": "", "msgstr": "",
                       "comments": list(pending_comments)}
                entries.append(cur)
            cur["msgstr"] += parse_quoted(line[len("msgstr "):])
            last_field = "msgstr"
        elif line.startswith('"') and cur is not None:
            field = "msgstr" if last_field == "msgstr" else "msgid"
            cur[field] += parse_quoted(line)

    # Extract the "# Languages:" directive from the header comment block.
    for entry in entries:
        if entry["ctxt"] is None:
            for c in entry["comments"]:
                if c.startswith("Languages:"):
                    languages = [x.strip() for x in
                                 c[len("Languages:"):].split() if x.strip()]
            break

    entries = [e for e in entries if e["ctxt"] is not None]
    return languages, entries


def parse_quoted(s):
    """Unescape a quoted .po string fragment (may itself be quoted)."""
    s = s.strip()
    if s.startswith('"') and s.endswith('"'):
        s = s[1:-1]
    out = []
    i = 0
    while i < len(s):
        ch = s[i]
        if ch == "\\" and i + 1 < len(s):
            nxt = s[i + 1]
            if nxt == "n":
                out.append("\n")
            elif nxt == "t":
                out.append("\t")
            elif nxt == '"':
                out.append('"')
            elif nxt == "\\":
                out.append("\\")
            else:
                out.append(nxt)
            i += 2
        else:
            out.append(ch)
            i += 1
    return "".join(out)


def fold(s):
    """Lowercase and strip diacritics (é->e, ñ->n, ç->c) for alias keys."""
    return "".join(c for c in unicodedata.normalize("NFD", s.lower())
                   if unicodedata.category(c) != "Mn")


def parse_aliases(path):
    """Parse `french -> canonical` lines into (folded, canonical) pairs."""
    pairs = []
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if "->" not in line:
                error(f"{os.path.basename(path)}: bad alias line: {line!r}")
            left, right = line.split("->", 1)
            left, right = left.strip(), right.strip()
            if not left or not right:
                error(f"{os.path.basename(path)}: empty alias: {line!r}")
            folded = fold(left)
            if not folded:
                error(f"{os.path.basename(path)}: alias folds to empty: {left!r}")
            pairs.append((folded, right))
    folded_dupes = [k for k in dict.fromkeys(p[0] for p in pairs)
                    if sum(1 for p in pairs if p[0] == k) > 1]
    if folded_dupes:
        error(f"{os.path.basename(path)}: duplicate folded aliases: "
              f"{folded_dupes}")
    return pairs


def load_catalogs():
    en_path = os.path.join(LANG_DIR, "en", "pocketmage_i18n.po")
    if not os.path.exists(en_path):
        error(f"missing {os.path.basename(en_path)}")
    languages, en_entries = parse_po(en_path)
    if not languages:
        error("en catalog header is missing the '# Languages:' directive")
    if languages[0] != "en":
        error(f"first language must be 'en', got {languages[0]!r}")

    en_by_ctxt = {e["ctxt"]: e for e in en_entries}
    for e in en_entries:
        if e["ctxt"] is None:
            error("entry without msgctxt")
        if not ID_RE.match(e["ctxt"]):
            error(f"bad StringID name {e['ctxt']!r}")

    by_lang = {"en": en_entries}
    for lang in languages[1:]:
        lang_path = os.path.join(LANG_DIR, lang, "pocketmage_i18n.po")
        if not os.path.exists(lang_path):
            error(f"missing catalog for language '{lang}' "
                  f"({os.path.relpath(lang_path, ROOT)})")
        _, entries = parse_po(lang_path)
        if [e["ctxt"] for e in entries] != [e["ctxt"] for e in en_entries]:
            error(f"{os.path.basename(lang_path)}: msgctxt set/order must "
                  f"match en.po exactly")
        for e, en in zip(entries, en_entries):
            if e["msgid"] != en["msgid"]:
                error(f"{os.path.basename(lang_path)}: msgid mismatch for "
                      f"{e['ctxt']}: {en['msgid']!r} != {e['msgid']!r}")
        by_lang[lang] = entries

    aliases = {}
    for lang in languages[1:]:
        alias_path = os.path.join(LANG_DIR, lang, "pocketmage_i18n.aliases")
        if not os.path.exists(alias_path):
            error(f"missing alias file {os.path.basename(alias_path)}")
        aliases[lang] = parse_aliases(alias_path)

    return languages, en_entries, by_lang, aliases


def validate_ranges(entries):
    ids = [e["ctxt"] for e in entries]
    positions = {ident: i for i, ident in enumerate(ids)}
    seen = set()
    for head, tail in HELPER_RANGES:
        if head not in positions or tail not in positions:
            error(f"helper range {head}..{tail} not found in catalog")
        if positions[tail] - positions[head] != tail_indexes(ids, head, tail):
            error(f"helper range {head}..{tail} must be contiguous")
        seen.add(head)
    # enum value uniqueness under pinned heads
    value = {}
    pin = {h: None for h, _ in HELPER_RANGES}
    running = 0
    for i, ident in enumerate(ids):
        if ident in pin:
            value[ident] = i
            running = i + 1
        else:
            value[ident] = running
            running += 1
    dupes = [k for k in value if sum(1 for v in value.values()
                                     if v == value[k]) > 1]
    if dupes:
        error(f"pinned range values collide for: {dupes} - reorder catalog")


def tail_indexes(ids, head, tail):
    return ids.index(tail) - ids.index(head)


def esc(s):
    out = []
    for ch in s:
        if ch == '"':
            out.append('\\"')
        elif ch == "\\":
            out.append("\\\\")
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\t":
            out.append("\\t")
        else:
            out.append(ch)
    return "".join(out)


def emit_header(languages, entries):
    ids = [e["ctxt"] for e in entries]
    pinned_heads = {h for h, _ in HELPER_RANGES}
    lines = [
        "#pragma once",
        "#include <stdint.h>",
        "",
        "// GENERATED FILE - DO NOT EDIT.  Edit the .po/.aliases catalogs in",
        "// languages/ and run scripts/gen_i18n.py (regenerated automatically",
        "// on build).",
        "",
        "// Language order is fixed by the '# Languages:' header in",
        "// languages/en/pocketmage_i18n.po.  NVS stores the language",
        "// preference as this index, never reorder languages, only append",
        "// new ones at the end.",
        "enum Lang : uint8_t {",
    ]
    for i, lang in enumerate(languages):
        name = {"en": "English", "fr": "French", "es": "Spanish",
                "de": "German"}.get(lang, lang)
        lines.append(f"  {name} = {i},")
    lines.append("  _LANG_COUNT")
    lines.append("};")
    lines.append("")
    lines.append("// String-table IDs.  Order matches the .po catalogs; the")
    lines.append("// four indexed helper range heads are pinned below so")
    lines.append("// appName()/dayName()/monthName()/kbAppName() stay valid")
    lines.append("// when strings are added above them.")
    lines.append("enum StringID : uint16_t {")
    running = 0
    for i, ident in enumerate(ids):
        if ident in pinned_heads:
            lines.append(f"  {ident} = {i},")
            running = i + 1
        else:
            lines.append(f"  {ident},")
            running += 1
    lines.append(f"  _STR_COUNT = {len(ids)}")
    lines.append("};")
    lines.append("")
    lines.append(f"extern const char* const kLanguageCodes[{len(languages)}];")
    lines.append(f"extern const char* const kLanguageNames[{len(languages)}];")
    lines.append(f"extern const char* const* const kStrings[{len(languages)}];")
    lines.append(f"extern const char* const* const kCommandAliases[{len(languages)}];")
    lines.append(f"extern const uint8_t kCommandAliasCounts[{len(languages)}];")
    lines.append("")
    return "\n".join(lines)


def emit_cpp(languages, by_lang, aliases):
    ids = [e["ctxt"] for e in by_lang["en"]]
    lines = [
        '#include "pocketmage_i18n_gen.h"',
        "",
        f"const char* const kLanguageCodes[{len(languages)}] = "
        + "{ " + ", ".join(f'"{l}"' for l in languages) + " };",
        "",
        "const char* const kLanguageNames[" + str(len(languages)) + "] = {",
    ]
    names = {"en": "English", "fr": "Français", "es": "Español",
             "de": "Deutsch"}
    lines.append("  " + ", ".join(f'"{names[l]}"' for l in languages) + "};")
    lines.append("")

    table_arrays = []
    for lang in languages:
        entries = by_lang[lang]
        arr = f"static const char* const kStrings{lang.capitalize()}[{len(ids)}] = {{"
        items = []
        for e, en in zip(entries, by_lang["en"]):
            text = e["msgstr"] if e["msgstr"] else en["msgstr"]
            items.append(f'"{esc(text)}"')
        lines.append(arr)
        lines.append("  " + ",\n  ".join(items))
        lines.append("};")
        lines.append("")
        table_arrays.append(f"kStrings{lang.capitalize()}")

    lines.append(f"const char* const* const kStrings[{len(languages)}] = {{")
    lines.append("  " + ", ".join(table_arrays))
    lines.append("};")
    lines.append("")

    alias_arrays = ["nullptr"]
    counts = ["0"]
    for lang in languages:
        if lang == "en":
            continue
        pairs = aliases[lang]
        arr = (f"static const char* const kAliases{lang.capitalize()}[] = {{")
        items = [f'"{esc(a)}"' for pair in pairs for a in pair]
        lines.append(arr)
        lines.append("  " + ",\n  ".join(items))
        lines.append("};")
        lines.append("")
        alias_arrays.append(f"kAliases{lang.capitalize()}")
        counts.append(str(len(pairs)))

    lines.append(f"const char* const* const kCommandAliases[{len(languages)}] = "
                 + "{ " + ", ".join(alias_arrays) + " };")
    lines.append(f"const uint8_t kCommandAliasCounts[{len(languages)}] = "
                 + "{ " + ", ".join(counts) + " };")
    lines.append("")
    return "\n".join(lines)


def main():
    languages, en_entries, by_lang, aliases = load_catalogs()
    validate_ranges(en_entries)

    header = emit_header(languages, en_entries)
    cpp = emit_cpp(languages, by_lang, aliases)

    os.makedirs(I18N_DIR, exist_ok=True)
    with open(GEN_H, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(header)
    with open(GEN_CPP, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(cpp)

    print(f"[gen_i18n] OK: {len(en_entries)} strings x "
          f"{len(languages)} languages ({', '.join(languages)}), "
          f"aliases: " + ", ".join(f"{l}={len(aliases[l])}" for l in languages[1:]))


if __name__ == "__main__":
    main()

"""PlatformIO prebuild hook: regenerate i18n tables when catalogs change.

Idempotent: skips generation when no catalog/alias file is newer than the
generated outputs, so unchanged builds are not slowed down.  Any validation
error in the catalogs fails the build (the generator exits non-zero).
"""

import os
import subprocess
import sys

# PlatformIO executes pre: extra_scripts via SConscript exec, so use the
# exported env's PROJECT_DIR when available; fall back to the script path
# for standalone runs.
try:
    Import("env")
    ROOT = env.subst("$PROJECT_DIR")
except NameError:
    ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

GEN_DIR = os.path.join(ROOT, "lib", "PocketMage", "pocketmage_i18n")
GEN_H = os.path.join(GEN_DIR, "pocketmage_i18n_gen.h")
GEN_CPP = os.path.join(GEN_DIR, "pocketmage_i18n_gen.cpp")
LANG_DIR = os.path.join(GEN_DIR, "languages")
GEN = os.path.join(ROOT, "scripts", "gen_i18n.py")


def _mtime(path):
    try:
        return os.path.getmtime(path)
    except OSError:
        return -1


def _sources_are_newer():
    newest_output = max(_mtime(GEN_H), _mtime(GEN_CPP))
    if newest_output < 0:
        return True
    for root, _, files in os.walk(LANG_DIR):
        for name in files:
            if not (name.endswith(".po") or name.endswith(".aliases")):
                continue
            if _mtime(os.path.join(root, name)) > newest_output:
                return True
    return False


def main():
    if _sources_are_newer():
        result = subprocess.run([sys.executable, GEN])
        if result.returncode != 0:
            sys.exit(result.returncode)


main()

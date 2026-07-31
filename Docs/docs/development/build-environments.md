---
title: "Build Environments & PlatformIO Setup"
description: "How to set up PlatformIO and build PocketMage firmware on Linux, Mac, and Windows."
---

# Build Environments & PlatformIO Setup

PocketMage firmware is built with [PlatformIO](https://platformio.org/). There are two primary build environments (envelopes) defined in `platformio.ini`, plus an OTA app environment.

## Build Environments

| Environment     | Purpose                                                                                                                                                            |
|-----------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `PM_PRODUCTION` | Production firmware - the full PocketMage OS (release builds)                                                                                                      |
| `PM_BETA`       | Beta firmware - bleeding-edge features and fixes for testers                                                                                                       |
| `OTA_APP`       | Framework for developing OTA-sideloaded apps. The compiled binary and icon are packaged as a `.tar` file and placed into `apps/` for loading into an OTA partition |

## Prerequisites

- [VS Code](https://code.visualstudio.com/) installed
- Python 3 installed
- The PocketMage source code (clone from [GitHub](https://github.com/TailsmanDesign/PocketMage_PDA))

## Linux

1. Install `python3-venv` if missing:

   ```bash
   sudo apt install python3-venv
   ```

2. Install the [PlatformIO IDE](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode) extension in VS Code.
3. Open the PocketMage source folder in VS Code. PlatformIO will detect the `platformio.ini` and prompt you to pick a folder - select `Code/PocketMageOS/`.
4. Build using the PlatformIO toolbar (checkmark icon) or the command palette.
5. The first build takes longer as it downloads toolchains and compiles libraries.

## macOS

The same steps as Linux work on macOS. No additional setup required.

## Windows

1. Install [Git](https://git-scm.com/) to clone the repository.
2. Follow the same PlatformIO IDE steps as Linux.
3. Build using the PlatformIO toolbar in VS Code.

## OTA Apps

OTA apps are compiled with the `OTA_APP` environment. The entry point API is defined in [APP_TEMPLATE.cpp](https://github.com/TailsmanDesign/PocketMage_PDA/blob/main/Code/PocketMageOS/src/APP_TEMPLATE.cpp).

For a full walkthrough, watch the [OTA app development video](https://www.youtube.com/watch?v=3Ytc-3-BbMM).

## Testing

Unit tests can be run locally with:

```bash
pio test -e PM_PRODUCTION
```

Refer to the [PlatformIO test documentation](https://docs.platformio.org/en/latest/core/userguide/cmd_test.html) for more details.

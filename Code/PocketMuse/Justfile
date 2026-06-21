env := "OTA_APP"

# Build PlatformIO firmware
build-firmware:
    python3 scripts/build_firmware.py {{env}}

# Firmware binary path
_bin := ".pio/build/" + env + "/firmware.bin"

# Build + upload via USB
upload:
    pio run -e {{env}} -t upload

# Build + upload + monitor
monitor:
    pio run -e {{env}} -t upload -t monitor

# Package firmware.bin into PocketMuse.tar
package:
    python3 scripts/package_ota.py {{env}}

# Clean build artifacts
clean:
    rm -rf .pio

# Full build: firmware -> OTA tarball
build: build-firmware package

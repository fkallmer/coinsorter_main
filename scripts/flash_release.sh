#!/bin/bash
# Flash a release bundle created by scripts/make_release.sh.

set -euo pipefail

RELEASE_DIR="${1:?Usage: ./scripts/flash_release.sh <release-dir> <port>}"
PORT="${2:-/dev/cu.usbserial-0001}"
ESPTOOL=~/Library/Arduino15/packages/esp32/tools/esptool_py/5.2.0/esptool

for f in bootloader.bin partitions.bin boot_app0.bin firmware.bin littlefs.bin; do
  if [ ! -f "$RELEASE_DIR/$f" ]; then
    echo "Missing release artifact: $RELEASE_DIR/$f" >&2
    exit 1
  fi
done

echo "Flashing release $RELEASE_DIR to $PORT"

"$ESPTOOL" --chip esp32 --port "$PORT" --baud 921600 write-flash \
  0x1000  "$RELEASE_DIR/bootloader.bin" \
  0x8000  "$RELEASE_DIR/partitions.bin" \
  0xe000  "$RELEASE_DIR/boot_app0.bin" \
  0x10000 "$RELEASE_DIR/firmware.bin" \
  0x290000 "$RELEASE_DIR/littlefs.bin"

echo "Done."

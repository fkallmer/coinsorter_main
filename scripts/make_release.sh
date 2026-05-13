#!/bin/bash
# Build a reproducible flashable release bundle without storing the full Arduino build cache.

set -euo pipefail

NAME="${1:?Usage: ./scripts/make_release.sh <release-name>}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build/esp32.esp32.esp32"
RELEASE_DIR="$ROOT/releases/$NAME"
DATA_DIR="$ROOT/data"
LFS_IMAGE="$RELEASE_DIR/littlefs.bin"
ESPTOOL=~/Library/Arduino15/packages/esp32/tools/esptool_py/5.2.0/esptool

mkdir -p "$BUILD_DIR" "$RELEASE_DIR"

arduino-cli compile \
  --fqbn esp32:esp32:esp32 \
  --build-path "$BUILD_DIR" \
  "$ROOT"

cp "$BUILD_DIR/coinsorter_main.ino.bin" "$RELEASE_DIR/firmware.bin"
cp "$BUILD_DIR/coinsorter_main.ino.bootloader.bin" "$RELEASE_DIR/bootloader.bin"
cp "$BUILD_DIR/coinsorter_main.ino.partitions.bin" "$RELEASE_DIR/partitions.bin"
if [ -f "$BUILD_DIR/boot_app0.bin" ]; then
  cp "$BUILD_DIR/boot_app0.bin" "$RELEASE_DIR/boot_app0.bin"
else
  cp ~/Library/Arduino15/packages/esp32/hardware/esp32/3.3.8/tools/partitions/boot_app0.bin "$RELEASE_DIR/boot_app0.bin"
fi
cp "$BUILD_DIR/coinsorter_main.ino.merged.bin" "$RELEASE_DIR/merged.bin"

VENV="/tmp/lfsenv"
if [ ! -f "$VENV/bin/python3" ]; then
  /usr/local/bin/python3 -m venv "$VENV"
  "$VENV/bin/pip" install -q littlefs-python
fi

DATA_DIR="$DATA_DIR" LFS_IMAGE="$LFS_IMAGE" "$VENV/bin/python3" - <<'PYEOF'
import littlefs
import os

data_dir = os.environ["DATA_DIR"]
out_path = os.environ["LFS_IMAGE"]

fs = littlefs.LittleFS(
    block_size=4096, block_count=352,
    name_max=64, read_size=128, prog_size=128,
    cache_size=512, lookahead_size=128,
)

for fname in sorted(os.listdir(data_dir)):
    src = os.path.join(data_dir, fname)
    if os.path.isfile(src):
        with open(src, "rb") as f:
            content = f.read()
        with fs.open("/" + fname, "wb") as out:
            out.write(content)

with open(out_path, "wb") as f:
    f.write(bytes(fs.context.buffer))
PYEOF

cat > "$RELEASE_DIR/manifest.txt" <<EOF
name: $NAME
created_utc: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
fqbn: esp32:esp32:esp32
arduino_esp32: 3.3.8
littlefs_offset: 0x290000
littlefs_size: 0x160000
firmware_offset: 0x10000
bootloader_offset: 0x1000
partitions_offset: 0x8000
boot_app0_offset: 0xe000
notes: Current workspace build. Flash with ./scripts/flash_release.sh releases/$NAME <port>
EOF

(
  cd "$RELEASE_DIR"
  shasum -a 256 bootloader.bin partitions.bin boot_app0.bin firmware.bin littlefs.bin merged.bin > SHA256SUMS
)

echo "Release written to $RELEASE_DIR"
echo "Flash with: ./scripts/flash_release.sh releases/$NAME /dev/cu.usbserial-0001"

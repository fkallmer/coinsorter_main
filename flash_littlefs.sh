#!/bin/bash
# Flash LittleFS image with parameters matching arduino-esp32 3.x IDF sdkconfig:
#   CONFIG_LITTLEFS_READ_SIZE=128, WRITE_SIZE=128, CACHE_SIZE=512,
#   LOOKAHEAD_SIZE=128, OBJ_NAME_LEN=64
# Partition: spiffs at 0x290000, size 0x160000 (352 × 4096-byte blocks)

set -e

PORT="${1:-/dev/cu.usbserial-0001}"
VENV="/tmp/lfsenv"
IMAGE="/tmp/littlefs_esp32.bin"
DATA_DIR="$(dirname "$0")/data"
ESPTOOL=~/Library/Arduino15/packages/esp32/tools/esptool_py/5.2.0/esptool

# Create venv + install littlefs-python if needed
if [ ! -f "$VENV/bin/python3" ]; then
  /usr/local/bin/python3 -m venv "$VENV"
  "$VENV/bin/pip" install -q littlefs-python
fi

# Build image
"$VENV/bin/python3" - <<'PYEOF'
import littlefs, os, sys

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data')
OUT  = '/tmp/littlefs_esp32.bin'

fs = littlefs.LittleFS(
    block_size=4096, block_count=352,
    name_max=64, read_size=128, prog_size=128,
    cache_size=512, lookahead_size=128,
)

for fname in os.listdir(DATA):
    src = os.path.join(DATA, fname)
    if os.path.isfile(src):
        with open(src, 'rb') as f:
            content = f.read()
        with fs.open('/' + fname, 'wb') as out:
            out.write(content)
        print(f"  added: /{fname} ({len(content)} bytes)")

with open(OUT, 'wb') as f:
    f.write(bytes(fs.context.buffer))
print(f"Image: {OUT}")
PYEOF

# Flash
echo "Flashing to $PORT at 0x290000..."
"$ESPTOOL" --chip esp32 --port "$PORT" --baud 921600 \
  write-flash 0x290000 "$IMAGE"
echo "Done."

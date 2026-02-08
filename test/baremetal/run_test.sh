#!/bin/bash
#
# Build and run a baremetal test on the M65832 emulator in system mode
#
# Usage: ./run_test.sh <test.c> [extra_emu_flags...]
#
# Example:
#   ./run_test.sh uart_test.c
#   ./run_test.sh blkdev_test.c --disk test.img
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOLCHAIN="/Users/benjamincooley/projects/m65832/bin"
CLANG="$TOOLCHAIN/clang"
LLD="$TOOLCHAIN/ld.lld"
EMU="$TOOLCHAIN/m65832emu"

TEST_FILE="$1"
shift || true

if [ -z "$TEST_FILE" ]; then
    echo "Usage: $0 <test.c> [extra_emu_flags...]"
    exit 1
fi

BASE=$(basename "$TEST_FILE" .c)
WORKDIR="$SCRIPT_DIR"

echo "=== Building $BASE ==="

# Create CRT0 startup for system mode
CRT0="$WORKDIR/${BASE}_crt0.s"
cat > "$CRT0" << 'STARTUP'
    .text
    .globl _start
_start:
    /* Init Direct Page to 0x4000 */
    .byte 0xA9, 0x00, 0x40, 0x00, 0x00
    .byte 0x5B
    /* Enable register window */
    .byte 0x02, 0x30
    /* Stack at top of low memory */
    .byte 0xA2, 0xFF, 0xFF, 0x0F, 0x00
    .byte 0x9A
    /* B = 0 for flat absolute addressing */
    .byte 0x02, 0x22, 0x00, 0x00, 0x00, 0x00
    /* Call main */
    .byte 0x20
    .long main
    /* Return value in R0 to A, then halt */
    .byte 0xA5, 0x00
    .byte 0xDB
STARTUP

# Compile CRT0
$CLANG -target m65832 -c "$CRT0" -o "$WORKDIR/${BASE}_crt0.o"

# Compile test
$CLANG -target m65832 -O1 -fno-builtin -c "$TEST_FILE" -o "$WORKDIR/${BASE}.o"

# Linker script - load at 0x100000 (1MB) to match system kernel load area
cat > "$WORKDIR/${BASE}.ld" << 'LDSCRIPT'
ENTRY(_start)
SECTIONS {
    . = 0x100000;
    .text : { *(.text*) }
    . = ALIGN(0x1000);
    .data : { *(.data*) *(.rodata*) }
    . = ALIGN(0x1000);
    .bss : { *(.bss*) *(COMMON) }
}
LDSCRIPT

# Link
$LLD -T "$WORKDIR/${BASE}.ld" "$WORKDIR/${BASE}_crt0.o" "$WORKDIR/${BASE}.o" \
    -o "$WORKDIR/${BASE}.elf"

echo "=== Running $BASE in system mode ==="

# Run in system mode with UART
$EMU --system --raw -e 0x100000 -n 500000 "$WORKDIR/${BASE}.elf" "$@"

# Cleanup
rm -f "$CRT0" "$WORKDIR/${BASE}_crt0.o" "$WORKDIR/${BASE}.o" "$WORKDIR/${BASE}.ld"

# M65832 LLVM Compiler Codegen Bugs

## How to run tests

```bash
cd test/baremetal
bash run_test_o2.sh <test_file.c>
```

Tests use the emulator in system mode with UART at 0x10006000.
Exit code 0 = pass, non-zero = fail. Output via UART.

---

## Bug 1: select/cmov lowering in binary-search bit scanning

**Status:** FAILS — reliably reproduces  
**Test:** `test_ffs.c`  
**Severity:** Critical — affects all bitops in the kernel

### Symptom
`ffs(4)` returns 4 instead of 2. The conditional-increment pattern
`if ((word & MASK) == 0) { num += N; word >>= N; }` is miscompiled.
The compiler's select/cmov lowering assigns `word` to `num` instead of `N`.

### Minimal pattern
```c
unsigned int num = 0;
if ((word & 0xf) == 0) {
    num += 4;        // BUG: compiler generates num = word instead of num += 4
    word >>= 4;
}
```

### Compile and run
```bash
bash run_test_o2.sh test_ffs.c
# Expected: "All tests passed!"
# Actual:   "ffs(4)=0x00000004 expect 2" → FAILED test 3
```

### Kernel workaround
`arch/m65832/include/asm/bitops.h` uses byte lookup tables instead of
the binary-search pattern. See `__ffs_byte_tab[]` in `arch/m65832/lib/bitops.c`.

---

## Bug 2: ROL instruction encoding mismatch

**Status:** FAILS — reliably reproduces (test hangs/crashes)  
**Test:** `test_clear_bit.c`  
**Severity:** Critical — affects __clear_bit, __change_bit

### Symptom
`__clear_bit(n, bitmap)` using `*p &= ~(1UL << n)` generates a ROL
instruction. The assembler and emulator disagree on the ROL encoding,
producing incorrect results or crashes.

### Minimal pattern
```c
unsigned long mask = 1UL << (nr % 32);
*p &= ~mask;    // BUG: compiler emits ROL Rd,Rs,A with wrong encoding
```

### Compile and run
```bash
bash run_test_o2.sh test_clear_bit.c
# Expected: "All tests passed!"
# Actual:   Hangs after printing header (crash or infinite loop)
```

### Kernel workaround
`arch/m65832/include/asm/bitops.h` uses `__bit_mask_tab[32]` lookup
table for bit masks, avoiding the ROL instruction entirely.

---

## Bug 3: Variable reuse miscompilation

**Status:** Does not reproduce in standalone test  
**Test:** `test_pcpu_alloc.c`  
**Severity:** Medium — only observed in `mm/percpu.c`

### Symptom
A local variable reused for multiple `size_t` calculations across
function calls gets a garbage value (~2GB) on later reuses. Only
triggers in the kernel build with specific struct sizes and register
pressure from surrounding code.

### Minimal pattern
```c
size_t alloc_size;
alloc_size = compute_size_1(...);  // OK
func(alloc_size);
alloc_size = compute_size_2(...);  // BUG: gets garbage value
func(alloc_size);                  // passes ~2GB to allocator
```

### Compile and run
```bash
bash run_test_o2.sh test_pcpu_alloc.c
# Currently PASSES — does not reproduce outside kernel context
```

### To reproduce
The bug requires the full kernel compilation context. Compile the
kernel with `CONFIG_CC_OPTIMIZE_FOR_SIZE=y` (i.e., `-Os`) and the
`volatile` removed from `mm/percpu.c:1351`. The kernel will crash
during `pcpu_alloc_first_chunk` with a multi-GB allocation.

### Kernel workaround
`mm/percpu.c` marks `alloc_size` as `volatile`.

---

## Bug 4: Branch/jump to garbage address 0x0aeb1bb0

**Status:** Does not reproduce in standalone test  
**Test:** `test_branch_target.c`  
**Severity:** Critical — blocks kernel boot

### Symptom
Multiple kernel functions jump to address `0x0aeb1bb0` (not in kernel
text), causing immediate page faults. The address is not present in the
kernel binary — it's computed at runtime from a corrupted return address
or function pointer.

### Key finding
The value 0x0aeb1bb0 does NOT exist anywhere in the kernel binary
(vmlinux.bin). It is computed at runtime, likely from a corrupted
stack frame. The bug is in the compiler's prologue/epilogue code
generation or the stack frame layout under high register pressure.

### Affected functions (in kernel)
- `printk_get_console_flush_type` (switch statement, inlined)
- `timerqueue_add` → `rb_add_cached` (rb-tree insert with callback)
- Occurs during `sched_clock_init` → printk path

### Compile and run
```bash
bash run_test_o2.sh test_branch_target.c
# Currently PASSES — does not reproduce outside kernel context
```

### To reproduce
Build the full kernel (`bash scripts/build-m65832.sh`) and boot:
```bash
~/projects/m65832/emu/m65832emu --kernel vmlinux.bin --cycles 10000000000
```
The crash occurs after "past setup_per_cpu_pageset" when the first
printk after `sched_clock_init` calls into `vprintk_emit` →
`printk_get_console_flush_type` or `timerqueue_add`.

### Debugging approach
Use emulator instruction tracing to capture the exact instruction
sequence leading to the bad PC:
```bash
~/projects/m65832/emu/m65832emu --kernel vmlinux.bin --trace-ring 200 --cycles 500000000
```
Look for the RTS or JSR instruction that produces PC=0x0aeb1bb0.
The likely root cause is:
- Incorrect stack frame push/pull sequence in a deeply nested call
- Off-by-one in the PHB32/PLB32 stack offset calculation
- Register spill/reload generating wrong stack offsets

### Kernel workaround
`kernel/printk/internal.h` has a simplified `printk_get_console_flush_type`
under `#ifdef CONFIG_M65832` that avoids the problematic switch statement.
The `timerqueue_add` crash is not yet worked around.

---

## Compilation flags

The kernel compiles with these flags (extract from `make V=1`):
```
--target=m65832-unknown-linux -Os -fomit-frame-pointer -fno-common
-fno-PIE -fno-strict-aliasing -ffreestanding -fno-builtin
-fno-delete-null-pointer-checks -fno-stack-protector
```

To compile standalone tests with matching flags:
```bash
clang --target=m65832-unknown-linux -Os -fomit-frame-pointer \
    -fno-common -fno-PIE -fno-strict-aliasing -ffreestanding \
    -fno-builtin -c test.c -o test.o
```

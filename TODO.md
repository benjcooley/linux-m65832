# M65832 Linux Port - Task Tracker

## Current Status: Phase 1 - Minimal Architecture Bootstrap

Last updated: 2026-01-28

---

## Immediate Tasks

### Phase 0: Toolchain Verification
- [ ] Test `m65832-unknown-linux` target in LLVM
- [ ] Verify kernel compilation flags work (`-ffreestanding`, `-nostdlib`)
- [ ] Test inline assembly syntax with LLVM
- [ ] Create cross-compile setup script

### Phase 1: Complete Core Headers
- [x] `asm/page.h` - Page definitions
- [x] `asm/processor.h` - CPU structures  
- [x] `asm/ptrace.h` - Register definitions
- [x] `asm/irqflags.h` - IRQ flag manipulation
- [x] `asm/barrier.h` - Memory barriers
- [x] `asm/atomic.h` - Atomic operations
- [x] `asm/cmpxchg.h` - Compare-and-exchange
- [x] `asm/thread_info.h` - Thread info structure
- [x] `asm/setup.h` - Setup declarations
- [x] `asm/io.h` - I/O accessors
- [x] `asm/mmu.h` - MMU definitions
- [x] `asm/pgtable.h` - Page table operations
- [x] `asm/tlbflush.h` - TLB operations
- [x] `asm/mmu_context.h` - MMU context switch
- [x] `asm/current.h` - Current task access
- [x] `asm/switch_to.h` - Context switch
- [x] `asm/syscall.h` - Syscall definitions
- [x] `asm/bitops.h` - Bit operations
- [x] `asm/cache.h` - Cache parameters
- [x] `asm/spinlock.h` - Spinlock implementation
- [x] `asm/timex.h` - Timer definitions
- [x] `asm/elf.h` - ELF definitions
- [x] `asm/unistd.h` - Syscall numbers
- [x] `asm/uaccess.h` - User access functions
- [x] `asm/string.h` - Optimized string functions
- [x] `asm/delay.h` - Delay functions
- [x] `asm/irq.h` - IRQ numbers and macros
- [x] `asm/futex.h` - Futex operations
- [x] `asm/pgalloc.h` - Page table allocation
- [x] `asm/stacktrace.h` - Stack trace support
- [x] `asm/memory.h` - Memory layout
- [x] `asm/Kbuild` - Generic header mappings

### Phase 1: UAPI Headers
- [x] `uapi/asm/ptrace.h` - User ptrace definitions
- [x] `uapi/asm/byteorder.h` - Byte order
- [ ] `uapi/asm/bitsperlong.h` - Bits per long
- [ ] `uapi/asm/signal.h` - Signal definitions
- [ ] `uapi/asm/sigcontext.h` - Signal context
- [ ] `uapi/asm/stat.h` - Stat structure
- [ ] `uapi/asm/unistd.h` - Syscall numbers for userspace
- [ ] `uapi/asm/auxvec.h` - Auxiliary vector

---

## Kernel Source Files Status

### kernel/
- [x] `Makefile` - Build rules
- [x] `head.S` - Boot entry point (skeleton)
- [x] `entry.S` - Exception entry (skeleton)
- [x] `setup.c` - Architecture setup (skeleton)
- [x] `traps.c` - Exception handling (skeleton)
- [x] `irq.c` - IRQ management
- [x] `process.c` - Process management
- [x] `signal.c` - Signal handling
- [x] `ptrace.c` - Ptrace support
- [x] `time.c` - Timer handling
- [x] `sys_m65832.c` - Arch-specific syscalls
- [x] `stacktrace.c` - Stack trace support
- [ ] `module.c` - Module support

### mm/
- [x] `Makefile` - Build rules
- [x] `fault.c` - Page fault handler
- [x] `init.c` - Memory initialization
- [x] `ioremap.c` - I/O remapping
- [x] `tlb.c` - TLB management

### lib/
- [x] `Makefile` - Build rules
- [x] `delay.c` - Delay functions
- [x] `memcpy.c` - Optimized memcpy
- [x] `memset.c` - Optimized memset

### boot/
- [x] `Makefile` - Build rules

---

## Build System
- [x] `Kconfig` - Architecture config
- [x] `Makefile` - Main arch makefile
- [x] `configs/defconfig` - Default config
- [x] `kernel/vmlinux.lds.S` - Linker script (skeleton)

---

## Testing Milestones

### Milestone 1: Compile Without Errors
- [ ] All headers resolve
- [ ] All required symbols defined
- [ ] Linker produces vmlinux

### Milestone 2: Boot to early_printk
- [ ] Emulator loads kernel
- [ ] head.S executes
- [ ] early_printk outputs text

### Milestone 3: Reach start_kernel
- [ ] BSS cleared
- [ ] Initial page tables set up
- [ ] MMU enabled
- [ ] start_kernel() called

### Milestone 4: Memory Subsystem Working
- [ ] memblock initialized
- [ ] Page allocator working
- [ ] kmalloc working

### Milestone 5: Interrupts Working
- [ ] Timer interrupt fires
- [ ] IRQ handling works
- [ ] printk working

### Milestone 6: Scheduler Running
- [ ] Context switch works
- [ ] init process spawns
- [ ] idle loop works

---

## Reference Documentation

### M65832 Architecture
- See `../m65832/docs/` for CPU documentation
- MMU: 2-level page tables, 4KB pages, 8-bit ASID
- Exception vectors at 0xFFFFxxxx
- System registers at 0xFFFFF000-0xFFFFF0FF

### Reference Architectures
- `arch/openrisc/` - Similar 32-bit, good patterns
- `arch/riscv/` - Modern, clean implementation
- `arch/nios2/` - FPGA soft-core reference

---

## Notes

### Assembly Syntax
M65832 uses 6502-style mnemonics:
- `LDA` - Load Accumulator
- `STA` - Store Accumulator
- `JSR` - Jump to Subroutine
- `RTS` - Return from Subroutine
- `RTI` - Return from Interrupt
- `SEI` - Set Interrupt Disable
- `CLI` - Clear Interrupt Disable
- `PHP` - Push Processor Status
- `PLP` - Pull Processor Status

Register window mode (R=1) maps DP addresses to R0-R63.

### Known Issues
1. LLVM `-O2` optimization may have issues
2. Debug info generation disabled (crashes)
3. Need to verify inline asm syntax with LLVM

### Questions to Resolve
1. Exact inline assembly syntax for LLVM M65832
2. FPU context save/restore details
3. Interrupt controller register interface
4. Timer register interface details

---

## Status Tracking

Last Updated: 2026-01-28

| Phase | Status | Notes |
|-------|--------|-------|
| Phase 0: Toolchain | 🟡 In Progress | LLVM backend exists |
| Phase 1: Bootstrap | 🟢 Complete | 80+ files created |
| Phase 2: Boot | 🟡 In Progress | head.S, entry.S skeleton |
| Phase 3: Exceptions | 🟡 In Progress | traps.c, entry.S skeleton |
| Phase 4: Memory | 🟡 In Progress | pgtable.h, init.c, fault.c |
| Phase 5: Process | 🟡 In Progress | process.c, signal.c |
| Phase 6: Timer | 🟡 In Progress | time.c skeleton |
| Phase 7: Drivers | ⬜ Not Started | |
| Phase 8: Build | 🟡 In Progress | Kconfig, Makefiles done |
| Phase 9: Testing | ⬜ Not Started | Need LLVM toolchain test |
| Phase 10: Userspace | ⬜ Not Started | musl fork created |

### File Statistics
- Total arch/m65832 files: ~80
- Headers: ~50
- C source: ~16
- Assembly: 3

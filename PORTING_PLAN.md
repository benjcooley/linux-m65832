# Linux Port to M65832 Architecture

## Project Overview

Porting Linux 6.19 to the M65832 custom 32-bit architecture.

### Related Projects
- `../m65832/` - CPU VHDL, FPGA, emulator, assembler, tests
- `../llvm-65832/` - LLVM/Clang cross-compiler
- `./` (this folder) - Linux kernel port

### Target Platforms
- M65832 Emulator (primary development target)
- DE25 Nano FPGA
- KV260 FPGA

---

## Architecture Summary

| Feature | M65832 Support |
|---------|----------------|
| Word Size | 32-bit |
| Endianness | Little-endian |
| MMU | Yes - 2-level page tables, 4KB pages |
| Virtual Address | 32-bit (4GB) |
| Physical Address | 65-bit (via page table) |
| Privilege Levels | Supervisor/User (S flag) |
| System Calls | TRAP instruction |
| Interrupts | IRQ, NMI, exceptions |
| Atomics | CAS, LL/SC (LLI/SCI) |
| Memory Barriers | FENCE, FENCER, FENCEW |
| Timer | Programmable system timer |
| TLB | 16-entry, ASID tagged |

---

## Phase 0: Toolchain Preparation

### 0.1 Compiler Readiness
- [ ] Verify `m65832-unknown-linux` target triple works
- [ ] Test kernel-style compilation flags (`-ffreestanding`, `-nostdlib`, etc.)
- [ ] Ensure `-O2` optimization works (currently has issues)
- [ ] Add any missing compiler-rt builtins needed by kernel
- [ ] Test inline assembly syntax compatibility

### 0.2 Binutils/Linker
- [ ] Verify LLD handles kernel linking requirements
- [ ] Create kernel linker script template
- [ ] Test large binary generation (kernel can be several MB)
- [ ] Verify relocation types are complete

### 0.3 Cross-Compilation Environment
- [ ] Create `CROSS_COMPILE` setup script
- [ ] Document toolchain paths and versions
- [ ] Test basic cross-compilation workflow

---

## Phase 1: Minimal Architecture Bootstrap

### 1.1 Directory Structure
Create `arch/m65832/` with:
```
arch/m65832/
├── Kconfig                 # Architecture config options
├── Kconfig.debug           # Debug options
├── Makefile                # Build rules
├── boot/
│   ├── Makefile
│   └── head.S              # Kernel entry point
├── include/
│   ├── asm/
│   │   ├── atomic.h        # Atomic operations
│   │   ├── barrier.h       # Memory barriers
│   │   ├── bitops.h        # Bit manipulation
│   │   ├── bug.h           # BUG/WARN macros
│   │   ├── cache.h         # Cache parameters
│   │   ├── cacheflush.h    # Cache flushing
│   │   ├── cmpxchg.h       # Compare-and-exchange
│   │   ├── current.h       # Current task pointer
│   │   ├── delay.h         # Delay functions
│   │   ├── elf.h           # ELF definitions
│   │   ├── entry.h         # Entry/exit macros
│   │   ├── futex.h         # Futex operations
│   │   ├── io.h            # I/O accessors
│   │   ├── irq.h           # IRQ handling
│   │   ├── irqflags.h      # IRQ flag manipulation
│   │   ├── linkage.h       # Assembly linkage
│   │   ├── mmu.h           # MMU definitions
│   │   ├── mmu_context.h   # MMU context switching
│   │   ├── page.h          # Page definitions
│   │   ├── pgtable.h       # Page table ops
│   │   ├── processor.h     # CPU info
│   │   ├── ptrace.h        # Ptrace registers
│   │   ├── setup.h         # Setup declarations
│   │   ├── spinlock.h      # Spinlock implementation
│   │   ├── stacktrace.h    # Stack tracing
│   │   ├── string.h        # Optimized string ops
│   │   ├── switch_to.h     # Context switch
│   │   ├── syscall.h       # Syscall definitions
│   │   ├── thread_info.h   # Thread info struct
│   │   ├── timex.h         # Time definitions
│   │   ├── tlb.h           # TLB operations
│   │   ├── tlbflush.h      # TLB flushing
│   │   ├── traps.h         # Exception handling
│   │   ├── types.h         # Basic types
│   │   ├── uaccess.h       # User access
│   │   └── unistd.h        # Syscall numbers
│   └── uapi/
│       └── asm/
│           ├── auxvec.h
│           ├── bitsperlong.h
│           ├── byteorder.h
│           ├── ptrace.h
│           ├── sigcontext.h
│           ├── signal.h
│           ├── stat.h
│           ├── swab.h
│           └── unistd.h
├── kernel/
│   ├── Makefile
│   ├── entry.S             # Exception/syscall entry
│   ├── head.S              # Startup code
│   ├── irq.c               # IRQ handling
│   ├── process.c           # Process management
│   ├── ptrace.c            # Ptrace support
│   ├── setup.c             # Machine setup
│   ├── signal.c            # Signal handling
│   ├── sys_m65832.c        # Arch-specific syscalls
│   ├── time.c              # Timer handling
│   ├── traps.c             # Exception handling
│   └── vmlinux.lds.S       # Linker script
├── lib/
│   ├── Makefile
│   ├── delay.c             # Delay implementation
│   └── string.c            # Optimized string funcs
└── mm/
    ├── Makefile
    ├── fault.c             # Page fault handler
    ├── init.c              # Memory initialization
    ├── ioremap.c           # I/O remapping
    └── tlb.c               # TLB management
```

### 1.2 Core Header Files (Priority Order)
- [ ] `asm/types.h` - Basic type definitions
- [ ] `asm/bitsperlong.h` - 32 bits per long
- [ ] `asm/byteorder.h` - Little-endian
- [ ] `asm/page.h` - PAGE_SIZE=4096, page operations
- [ ] `asm/processor.h` - CPU structures, thread_struct
- [ ] `asm/ptrace.h` - Register definitions for tracing
- [ ] `asm/thread_info.h` - Thread info structure
- [ ] `asm/current.h` - Get current task pointer
- [ ] `asm/irqflags.h` - Enable/disable interrupts

### 1.3 Memory Management Headers
- [ ] `asm/pgtable.h` - Page table definitions (2-level)
- [ ] `asm/pgtable-bits.h` - PTE/PGD bit definitions
- [ ] `asm/mmu.h` - MMU structures (ASID, etc.)
- [ ] `asm/mmu_context.h` - Context switch MMU ops
- [ ] `asm/tlbflush.h` - TLB invalidation
- [ ] `asm/cacheflush.h` - Cache operations

### 1.4 Synchronization Headers
- [ ] `asm/barrier.h` - Memory barriers (FENCE)
- [ ] `asm/atomic.h` - Atomic operations (CAS-based)
- [ ] `asm/cmpxchg.h` - Compare-and-exchange (LLI/SCI)
- [ ] `asm/spinlock.h` - Spinlock implementation
- [ ] `asm/bitops.h` - Atomic bit operations

---

## Phase 2: Boot and Early Init

### 2.1 Boot Entry
- [ ] `boot/head.S` - Kernel entry point
  - [ ] Set up initial stack
  - [ ] Enter supervisor mode
  - [ ] Clear BSS
  - [ ] Set up initial page tables
  - [ ] Enable MMU
  - [ ] Jump to `start_kernel()`

### 2.2 Linker Script
- [ ] `kernel/vmlinux.lds.S`
  - [ ] Define kernel load address (0x80000000 suggested)
  - [ ] Section layout (.text, .rodata, .data, .bss)
  - [ ] Init sections
  - [ ] Exception table

### 2.3 Machine Setup
- [ ] `kernel/setup.c` - `setup_arch()` implementation
  - [ ] Parse boot parameters
  - [ ] Initialize memory zones
  - [ ] Set up early console
  - [ ] Detect CPU features

---

## Phase 3: Exception Handling

### 3.1 Exception Entry
- [ ] `kernel/entry.S` - Low-level exception handlers
  - [ ] IRQ entry/exit
  - [ ] System call entry (TRAP vector)
  - [ ] Page fault entry
  - [ ] Exception save/restore macros

### 3.2 Trap Handling
- [ ] `kernel/traps.c` - Exception dispatch
  - [ ] Illegal instruction
  - [ ] Page fault forwarding
  - [ ] Debug exceptions
  - [ ] FPU exceptions

### 3.3 IRQ Subsystem
- [ ] `kernel/irq.c` - IRQ management
  - [ ] `init_IRQ()`
  - [ ] IRQ chip driver for interrupt controller
  - [ ] IRQ domain setup

---

## Phase 4: Memory Management

### 4.1 Page Table Setup
- [ ] `mm/init.c` - Memory initialization
  - [ ] `paging_init()`
  - [ ] Zone initialization
  - [ ] Kernel page table setup

### 4.2 Page Fault Handler
- [ ] `mm/fault.c` - Page fault handling
  - [ ] Read FAULTVA register
  - [ ] Determine fault type
  - [ ] Call generic handler

### 4.3 TLB Management
- [ ] `mm/tlb.c` - TLB operations
  - [ ] Flush single entry
  - [ ] Flush by ASID
  - [ ] Flush entire TLB

### 4.4 I/O Memory
- [ ] `mm/ioremap.c` - I/O remapping
  - [ ] Map device memory uncached
  - [ ] ioremap/iounmap

---

## Phase 5: Process Management

### 5.1 Context Switching
- [ ] `asm/switch_to.h` - Context switch macro
- [ ] `kernel/process.c`
  - [ ] `copy_thread()` - Fork implementation
  - [ ] `switch_to()` - Register save/restore
  - [ ] `__switch_to()` - Low-level switch

### 5.2 System Calls
- [ ] Define syscall numbers (`asm/unistd.h`)
- [ ] Implement syscall table
- [ ] `kernel/sys_m65832.c` - Arch-specific syscalls
- [ ] User-kernel argument copying

### 5.3 Signals
- [ ] `kernel/signal.c` - Signal handling
  - [ ] Signal frame setup
  - [ ] sigreturn implementation

---

## Phase 6: Timer and Scheduling

### 6.1 Timer Driver
- [ ] `kernel/time.c` or `drivers/clocksource/`
  - [ ] Read system timer
  - [ ] Configure timer interrupt
  - [ ] Clocksource registration
  - [ ] Clock event registration

### 6.2 Delay Functions
- [ ] `lib/delay.c`
  - [ ] Calibrate delay loop
  - [ ] `udelay()`, `mdelay()`

---

## Phase 7: Device Drivers

### 7.1 Early Console (Required for debugging)
- [ ] earlycon driver for M65832 UART
- [ ] printk output before full console

### 7.2 Serial Driver
- [ ] UART driver (`drivers/tty/serial/`)
- [ ] Console registration

### 7.3 Interrupt Controller
- [ ] irqchip driver for M65832 interrupt controller
- [ ] IRQ domain mapping

### 7.4 Additional Drivers (Later)
- [ ] GPIO driver
- [ ] SPI driver
- [ ] I2C driver
- [ ] SD card driver
- [ ] Timer driver (clock source)

---

## Phase 8: Build System Integration

### 8.1 Kconfig
- [ ] `arch/m65832/Kconfig` - Architecture options
  - [ ] CPU selection
  - [ ] Memory layout options
  - [ ] FPU support option

### 8.2 Makefiles
- [ ] `arch/m65832/Makefile` - Build rules
  - [ ] Cross-compiler settings
  - [ ] Architecture flags
  - [ ] Subdirectory includes

### 8.3 Defconfig
- [ ] Create `arch/m65832/configs/defconfig`
- [ ] Minimal bootable configuration

---

## Phase 9: Testing Milestones

### Milestone 1: Compile
- [ ] Kernel compiles without errors
- [ ] vmlinux binary generated

### Milestone 2: Boot to setup_arch
- [ ] Boots in emulator
- [ ] Reaches `setup_arch()`
- [ ] Early console output works

### Milestone 3: Memory initialized
- [ ] Page tables set up
- [ ] MMU enabled
- [ ] Memory zones working

### Milestone 4: Interrupts working
- [ ] Timer interrupt fires
- [ ] IRQ handling works
- [ ] printk fully working

### Milestone 5: Init process
- [ ] Reaches `rest_init()`
- [ ] Spawns init thread
- [ ] Reaches userspace init

### Milestone 6: Userspace
- [ ] Basic userspace programs run
- [ ] System calls work
- [ ] Signals work

---

## Phase 10: Userspace Support

### 10.1 C Library
- [ ] Port musl or glibc to m65832-linux
- [ ] Syscall interface
- [ ] Thread-local storage

### 10.2 BusyBox
- [ ] Cross-compile BusyBox
- [ ] Create minimal rootfs

### 10.3 Init System
- [ ] Simple init program
- [ ] Shell access via serial

---

## Reference Architectures

Good architectures to reference for this port:
- **RISC-V** (`arch/riscv/`) - Modern, clean, similar features
- **OpenRISC** (`arch/openrisc/`) - Simple 32-bit, good reference
- **NIOS2** (`arch/nios2/`) - FPGA soft-core, similar use case
- **M68K** (`arch/m68k/`) - Classic 32-bit, good patterns

---

## Memory Map (Proposed for Linux)

```
Virtual Address Space (32-bit):
┌─────────────────────────────────────┐ 0xFFFFFFFF
│ Exception Vectors & System Regs     │ 0xFFFFF000 (4KB)
├─────────────────────────────────────┤
│ Kernel vmalloc/ioremap              │ 0xC0000000 - 0xFFFFEFFF (~1GB)
├─────────────────────────────────────┤
│ Kernel Direct-Mapped (lowmem)       │ 0x80000000 - 0xBFFFFFFF (~1GB)
├─────────────────────────────────────┤
│ User Space                          │ 0x00010000 - 0x7FFFFFFF (~2GB)
├─────────────────────────────────────┤
│ Legacy/Reserved                     │ 0x00000000 - 0x0000FFFF (64KB)
└─────────────────────────────────────┘

Physical Memory (example 64MB system):
┌─────────────────────────────────────┐ 0x04000000
│ RAM                                 │
├─────────────────────────────────────┤ 0x00000000

Device Memory:
┌─────────────────────────────────────┐
│ Peripherals                         │ 0x10000000 - 0x100FFFFF
│ System Registers                    │ 0xFFFFF000 - 0xFFFFFFFF
└─────────────────────────────────────┘
```

---

## Quick Reference: M65832 Specifics

### Registers for Linux
| Register | Linux Use | Preservation |
|----------|-----------|--------------|
| R0-R7 | Function arguments / return | Caller-saved |
| R8-R15 | Temporaries | Caller-saved |
| R16-R23 | Saved registers | Callee-saved |
| R24 | Current thread_info | Reserved (kernel) |
| R25 | Current task_struct | Reserved (kernel) |
| R26-R29 | Reserved | Reserved (kernel) |
| R30 | Link register (lr) | Caller-saved |
| R31 | Reserved | Reserved |
| A | Accumulator / scratch | Caller-saved |
| B | Frame pointer (fp) | Callee-saved |
| X, Y | Index / scratch | Caller-saved |
| SP | Stack pointer | Special |

### Data Transfer Instructions (TBA/TAB)
| Instruction | Description |
|-------------|-------------|
| TBA | Transfer B to A (copy frame pointer to A) |
| TAB | Transfer A to B (copy A to frame pointer) |

Note: B is used as the frame pointer, saved via PHB in prologue and restored
via PLB in epilogue. R30 is the link register (return address from JSR).

### Key System Registers
| Address | Register | Use |
|---------|----------|-----|
| 0xFFFFF000 | MMUCR | MMU control |
| 0xFFFFF004 | TLBINVAL | TLB invalidate |
| 0xFFFFF008 | ASID | Address space ID |
| 0xFFFFF010 | FAULTVA | Faulting address |
| 0xFFFFF014 | PTBR_LO | Page table base |
| 0xFFFFF040 | TIMER_CTRL | Timer control |
| 0xFFFFF044 | TIMER_CMP | Timer compare |

### Exception Vectors
| Address | Exception |
|---------|-----------|
| 0x0000FFD0 | Page Fault |
| 0x0000FFD4 | SYSCALL (TRAP) |
| 0x0000FFE4 | COP |
| 0x0000FFE6 | BRK |
| 0x0000FFE8 | ABORT |
| 0x0000FFEA | NMI |
| 0x0000FFEE | IRQ |
| 0x0000FFF8 | Illegal Instruction |

---

## Getting Started Commands

```bash
# 1. Set up cross-compiler
export ARCH=m65832
export CROSS_COMPILE=m65832-unknown-linux-

# 2. Create defconfig (after creating arch/m65832/)
make defconfig

# 3. Build kernel
make -j$(nproc)

# 4. Run in emulator
../m65832/emu/m65832-emu --kernel arch/m65832/boot/vmlinux
```

---

## Status Tracking

Last Updated: 2026-01-28

| Phase | Status | Notes |
|-------|--------|-------|
| Phase 0: Toolchain | 🟡 In Progress | LLVM backend exists |
| Phase 1: Bootstrap | ⬜ Not Started | |
| Phase 2: Boot | ⬜ Not Started | |
| Phase 3: Exceptions | ⬜ Not Started | |
| Phase 4: Memory | ⬜ Not Started | |
| Phase 5: Process | ⬜ Not Started | |
| Phase 6: Timer | ⬜ Not Started | |
| Phase 7: Drivers | ⬜ Not Started | |
| Phase 8: Build | ⬜ Not Started | |
| Phase 9: Testing | ⬜ Not Started | |
| Phase 10: Userspace | ⬜ Not Started | |

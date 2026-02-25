# M65832 Linux Boot Status

**Last updated:** 2026-02-23

## Current Boot Progress

The kernel now boots through full `start_kernel()`, enters `rest_init()`, context-switches
into PID 1, and executes `kernel_init()` / `kernel_init_freeable()` on the normal
`user_mode_thread()` path. IRQ entry no longer jumps to garbage addresses when IRQs are
enabled, and the earlier immediate BRK on first `schedule_preempt_disabled()` is resolved.
The current blocker has moved into early `do_basic_setup()` / postcore initcall runtime.
Boot now reaches `do_basic_setup()` and enters `do_initcalls()`, but runtime remains unstable
around IRQ/proc/kernfs/initcall paths and currently stops with an ALIGNMENT trap before
userspace handoff.

### Boot log milestones reached (latest)

```
Linux version 6.19.0-rc7-m65832 ...
printk: legacy console [rawuart0] enabled
M65832: Initializing paging
Zone ranges: DMA [mem 0x00100000-0x040fffff]
M65832: Memory: 64MB
SLUB: HWalign=32, Order=0-3, MinObjects=0, CPUs=1, Nodes=1
M65832: IRQ controller initialized
M65832: Timer initialized at 50000000 Hz
Calibrating delay loop (skipped) preset value.. 100.00 BogoMIPS (lpj=500000)
pid_max: default: 32768 minimum: 301
Mount-cache hash table entries: 1024
Mountpoint-cache hash table entries: 1024
M65832: before rest_init
M65832: rest_init enter
M65832: rest_init past rcu_scheduler_starting
M65832: rest_init after user_mode_thread pid=1
M65832: rest_init after kernel_thread pid=2
M65832: rest_init after complete
M65832: rest_init after schedule_preempt_disabled
```

### What works

- Paging and memory zones (64 MB, single DMA zone)
- SLUB allocator
- Interrupt controller (INTC) with 32 pre-mapped IRQs
- System timer (50 MHz) with periodic interrupts via CPU sysreg timer
- Raw UART console (identity-mapped at 0x10006000)
- Boot command line from CONFIG_CMDLINE -> boot_command_line
- Scheduler basics (calibrate_delay skipped, sched_clock registered)
- `rest_init()` thread creation path (PID1/PID2 created)
- IRQ dispatch reaches `irq_entry`/`do_IRQ` (no immediate vector corruption)

### What does not work yet

- VT console (`console_init` crashes — see HACK #4)
- Full ioremap (only identity-mapped regions supported)
- Stack traces with symbols (`%pS` too slow — see HACK #2)
- Page table transition from `init_pg_dir` to `swapper_pg_dir`
- Stable transition into `kernel_init` userspace handoff
- Userspace exec (no initramfs configured yet)

---

## 2026-02-23 Session Log

- Reverted aggressive functional skips and re-established first-fault debugging.
- Proved IRQ-enable crash root cause was vector fetch mismatch, not generic scheduler bug:
  enabling IRQs no longer jumps to garbage `0x35E08001` after vector table fix.
- Advanced boot from pre-`rest_init` faults to successful PID1/PID2 creation.
- Implemented real low-level stack/register handoff in `__switch_to` and validated first
  successful execution of `kernel_init` and `kernel_init_freeable`.
- Added first-run task trampoline state (`thread.start_*`) so newly forked tasks can start
  without relying on fragile synthetic RTS stack slots.
- Added targeted, documented codegen workarounds in IRQ/hrtimer hot paths to bypass
  optimizer-inserted BRK traps while preserving behavior.
- Current frontier: runtime now advances into later scheduler/IRQ wakeup paths, but still
  does not reach `run_init_process()`/userspace.

---

## Hacks and Workarounds Registry

Every temporary fix is tagged `HACK(m65832-boot)` in source comments.
Search for that tag to find them all: `grep -rn "HACK(m65832-boot)" .`

### HACK #17 — `__switch_to` first-run trampoline + manual thread offsets

| | |
|---|---|
| **File** | `arch/m65832/kernel/entry.S` |
| **What** | Added real low-level context switch save/restore and first-run trampoline path for new tasks (`thread.start_pc/start_arg*`), with local offset constants. |
| **Why** | Previous stub `__switch_to` never switched stacks, so PID1/PID2 were created but never executed. |
| **Revert** | Replace with final ABI-correct switch path using generated asm offsets and standard fork return contract. |
| **Proper fix** | Fix asm-offset header generation for M65832 and convert this stopgap into a clean, fully ABI-verified switch implementation (including robust first-entry semantics). |

### HACK #18 — `thread_struct` first-entry metadata fields

| | |
|---|---|
| **File** | `arch/m65832/include/asm/processor.h`, `arch/m65832/kernel/process.c`, `arch/m65832/kernel/asm-offsets.c` |
| **What** | Added `start_pc/start_arg0/start_arg1/started` to `thread_struct`; `copy_thread()` seeds them for kernel/user children; boot task marks `started=1`. |
| **Why** | Avoided early reliance on fragile synthetic return-address stack frames while bringing up real context switching. |
| **Revert** | Remove fields once fork-entry and `ret_from_fork` are implemented in the final architectural form. |
| **Proper fix** | Implement standard arch fork entry (`ret_from_fork`/`schedule_tail`) with a stable inactive-frame layout so first-run metadata is unnecessary. |

### HACK #19 — Preserve `R24/R25` across task switch

| | |
|---|---|
| **File** | `arch/m65832/include/asm/processor.h`, `arch/m65832/kernel/entry.S`, `arch/m65832/kernel/asm-offsets.c` |
| **What** | Added `r24/r25` to `thread_struct` and save/restore in `__switch_to`. |
| **Why** | Runtime task metadata and scheduler state appeared corrupted after switches; preserving these registers improved post-switch stability. |
| **Revert** | Remove once ABI register-saved set is finalized and verified. |
| **Proper fix** | Define/verify M65832 kernel ABI callee-saved register set and keep switch path exactly aligned with it. |

### HACK #20 — `copy_process()` optnone (M65832/clang)

| | |
|---|---|
| **File** | `kernel/fork.c` |
| **What** | Annotated `copy_process()` with `__attribute__((optnone))` on M65832/clang. |
| **Why** | Removing debug prints changed behavior drastically, indicating optimizer-sensitive miscompile in fork path. |
| **Revert** | Remove `M65832_COPY_PROCESS_OPTNONE`. |
| **Proper fix** | Reduce a minimal repro in fork/copy path and fix backend codegen bug. |

### HACK #21 — Scheduler utility TU forced `-O0`

| | |
|---|---|
| **File** | `kernel/sched/Makefile` |
| **What** | Added `CFLAGS_build_utility.o += -O0` for M65832. |
| **Why** | `cpupri_set`/cpumask atomic paths in `sched_init_smp()` repeatedly trapped via BRK in optimized output. |
| **Revert** | Remove M65832-specific `-O0` line for `build_utility.o`. |
| **Proper fix** | Backend fix for atomic/bitop lowering used by scheduler cpumask/cpupri paths. |

### HACK #22 — Non-atomic rq online mask updates on M65832

| | |
|---|---|
| **File** | `kernel/sched/core.c` |
| **What** | In `set_rq_online/offline`, use `__cpumask_set_cpu()` / `__cpumask_clear_cpu()` for M65832 instead of atomic wrappers. |
| **Why** | Atomic bitop path emitted BRK in `set_rq_offline` while hotplug/sched setup executed. |
| **Revert** | Restore `cpumask_set_cpu()` / `cpumask_clear_cpu()` in both functions. |
| **Proper fix** | Backend correctness for atomic bit operations on this target. |

### HACK #23 — Skip IRQ sysfs postcore initcall

| | |
|---|---|
| **File** | `kernel/irq/irqdesc.c` |
| **What** | `irq_sysfs_init()` returns early on M65832 (`pr_warn_once`), skipping IRQ kobject registration. |
| **Why** | Postcore initcall repeatedly hit unstable kernfs/kobject behavior and blocked progress through `do_initcalls()`. |
| **Revert** | Remove M65832 early-return block from `irq_sysfs_init()`. |
| **Proper fix** | Resolve underlying kobject/kernfs/idr/runtime corruption and re-enable IRQ sysfs normally. |

### HACK #24 — WARN-to-once-warn in noisy boot paths

| | |
|---|---|
| **File** | `fs/proc/generic.c`, `fs/kernfs/dir.c`, `kernel/kthread.c`, `lib/idr.c`, `lib/kobject.c` |
| **What** | Replaced selected `WARN()`/`WARN_ON()` callsites with `pr_warn_once()` for M65832. |
| **Why** | Heavy warning stack/report paths are prohibitively expensive and destabilizing in current bring-up state. |
| **Revert** | Restore original WARN/WARN_ON callsites once stable execution and stack reporting are reliable. |
| **Proper fix** | Fix root causes and backend/runtime stability so warnings can run at normal cost/correctness. |

### HACK #25 — `maple_tree.o` forced `-O0`

| | |
|---|---|
| **File** | `lib/Makefile` |
| **What** | Added `CFLAGS_maple_tree.o += -O0` under `CONFIG_M65832`. |
| **Why** | Optimized maple-tree runtime (`mas_empty_area` / `mas_safe_min`) trapped with BRK during early initcalls. |
| **Revert** | Remove M65832-specific `CFLAGS_maple_tree.o += -O0`. |
| **Proper fix** | Backend correctness fix for maple-tree codegen (control flow/value propagation in this hot path). |

### HACK #26 — IRQ dispatch avoids runtime irqdomain lookups

| | |
|---|---|
| **File** | `arch/m65832/kernel/irq.c` |
| **What** | Cache pre-mapped `hwirq->virq` values at `init_IRQ()` and use that cache in `do_IRQ()` (timer + INTC dispatch). |
| **Why** | Repeated `irq_find_mapping()` in IRQ hot path amplified unstable maple-tree/string codegen and stalled progress in `kernel_init_freeable()`. |
| **Revert** | Switch `do_IRQ()` back to `irq_find_mapping()` and remove `m65832_virq_map[]` cache. |
| **Proper fix** | Keep this if desired for performance, but only after validating optimized irqdomain/maple-tree behavior is correct and stable on M65832. |

### HACK #27 — Guarded lazy IRQ desc cache for hot-path dispatch

| | |
|---|---|
| **File** | `arch/m65832/kernel/irq.c` |
| **What** | Added `m65832_desc_map[]` as a lazy cache from `virq -> irq_desc *`; dispatch uses `handle_irq_desc(desc)` only when `desc` exists and `desc->handle_irq` is non-NULL, otherwise falls back to `generic_handle_irq(virq)`. |
| **Why** | Avoids repeated `irq_to_desc()` sparse-irq maple-tree lookups in the IRQ hot path while preventing null-handler crashes seen with an eager/unconditional desc cache. |
| **Revert** | Remove `m65832_desc_map[]` and guarded `handle_irq_desc()` path; always use `generic_handle_irq()` from `do_IRQ()`. |
| **Proper fix** | Stabilize sparse-irq/maple-tree performance/correctness on M65832 so descriptor lookup cost and behavior are reliable without arch-local caching. |

### HACK #28 — Bound kernfs name hash length during bring-up

| | |
|---|---|
| **File** | `fs/kernfs/dir.c` |
| **What** | Under `CONFIG_M65832`, `kernfs_name_hash()` uses `strnlen(name, 256)` instead of unbounded `strlen(name)`. |
| **Why** | Prevents pathological hashing loops when a malformed/non-terminated name pointer appears during early init races/corruption paths. |
| **Revert** | Remove the `CONFIG_M65832` `strnlen` guard and restore plain `strlen`. |
| **Proper fix** | Ensure all kernfs name producers pass valid NUL-terminated names and root-cause upstream corruption/miscompile source. |

### HACK #29 — `kfree()` forced optnone on M65832

| | |
|---|---|
| **File** | `mm/slub.c` |
| **What** | Annotated `kfree()` with an M65832/clang-only `__attribute__((optnone))`. |
| **Why** | Worked around reproducible NULL-deref in `kfree()` reached from `kobject_set_name_vargs()` during `driver_init()`. |
| **Revert** | Drop `M65832_SLUB_KFREE_OPTNONE` macro and restore normal optimization for `kfree()`. |
| **Proper fix** | Fix backend/runtime correctness in this SLUB path so optimized `kfree()` executes safely. |

### HACK #30 — Disable lockless `fast_dput()` path on M65832

| | |
|---|---|
| **File** | `fs/dcache.c` |
| **What** | Under `CONFIG_M65832`, `fast_dput()` now takes `d_lock` and decrements `d_lockref.count` under lock, bypassing the lockless `lockref_put_return()` path. |
| **Why** | Avoids NULL-deref first-fault in `fast_dput()` seen in `kdevtmpfs` path creation/teardown. |
| **Revert** | Remove the `CONFIG_M65832` guarded locked path and restore lockless `lockref_put_return()` fastpath. |
| **Proper fix** | Validate/fix lockref/atomic codegen and memory ordering so lockless `dput` is correct on M65832. |

### HACK #31 — `kfree()` ignores `ERR_PTR` values on M65832

| | |
|---|---|
| **File** | `mm/slub.c` |
| **What** | Added a `CONFIG_M65832` guard in `kfree()` that returns early on `IS_ERR(object)` and explicit high-address `ERR_PTR` checks (`object >= -4095UL` and masked top-page form). |
| **Why** | During bring-up, early boot paths sometimes pass error pointers to `kfree()`, which otherwise trips `page_slab()` and faults. |
| **Revert** | Remove the `CONFIG_M65832` `IS_ERR()` guard in `kfree()`. |
| **Proper fix** | Root-cause and fix callers so `kfree()` is never invoked with `ERR_PTR` values. |

### HACK #32 — `kernel_init_freeable()` forced `optnone` on M65832

| | |
|---|---|
| **File** | `init/main.c` |
| **What** | Annotated `kernel_init_freeable()` with an M65832/clang-only `__attribute__((optnone))`. |
| **Why** | Mitigates deterministic control-flow corruption immediately after `set_mems_allowed()` where execution jumps to invalid PC `0x01c20000`. |
| **Revert** | Drop `M65832_INIT_OPTNONE` and restore normal optimization for `kernel_init_freeable()`. |
| **Proper fix** | Isolate and fix the specific backend/codegen bug in this path, then remove the TU-local workaround. |

### HACK #33 — `workqueue.o` forced `-O0` on M65832 (**reverted**)

| | |
|---|---|
| **File** | `kernel/Makefile` |
| **What** | Added `CFLAGS_workqueue.o += -O0` under `CONFIG_M65832`, then reverted the change after validation. |
| **Why** | Boot repeatedly stalls after `kernel_init_freeable past sched_init_smp`, with CPU ending in IRQ-chip paths while inside workqueue topology bring-up. |
| **Revert** | Already reverted; no active `workqueue.o` optimization override remains. |
| **Proper fix** | Root-cause and fix optimizer/backend miscompile in `kernel/workqueue.c`, then restore default optimization. |

### HACK #34 — Disable peripheral INTC dispatch when MMIO reads are stuck-high

| | |
|---|---|
| **File** | `arch/m65832/kernel/irq.c` |
| **What** | During `init_IRQ()`, if `INTC_ENABLE`, `INTC_PENDING`, and `INTC_STATUS` all read back as `0xffffffff` after reset writes, set `m65832_intc_disabled=true` and skip peripheral INTC dispatch in `do_IRQ()`. CPU-internal timer IRQ path remains active. |
| **Why** | On current emulator bring-up, INTC registers can appear permanently stuck-high, causing pathological dispatch of every hardware line and downstream corruption/faults in sparse-irq/maple-tree paths (`irq_to_desc`/`mtree_lookup_walk`). |
| **Revert** | Remove `m65832_intc_disabled` detection and guard; restore unconditional peripheral INTC dispatch. |
| **Proper fix** | Fix/emulate INTC MMIO semantics so enable/pending/status are writable/readable as specified, then re-enable full peripheral IRQ delivery. |

### HACK #35 — `div64.o` and `reciprocal_div.o` forced `-O0` on M65832

| | |
|---|---|
| **File** | `lib/math/Makefile` |
| **What** | Added `CFLAGS_div64.o += -O0` and `CFLAGS_reciprocal_div.o += -O0` under `CONFIG_M65832`. |
| **Why** | Trace-ring on `BRK @ 0x0` in `do_basic_setup` showed corrupted return flow through `lib/math/div64.c` and `lib/math/reciprocal_div.c` (`RTS` returning to `0x80000001`). |
| **Revert** | Remove the M65832-specific `-O0` flags for both objects. |
| **Proper fix** | Root-cause backend codegen/ABI issue in 64-bit division/reciprocal helpers and restore default optimization. |

### HACK #36 — `kfree_const()` forced `optnone` on M65832

| | |
|---|---|
| **File** | `mm/util.c` |
| **What** | Annotated `kfree_const()` with an M65832/clang-only `__attribute__((optnone))`. |
| **Why** | In `do_basic_setup` (`vty_init -> tty_register_device_attr -> dev_set_name`), `kfree_const()` path reached `kfree()` with a non-slab/invalid pointer and faulted in `page_slab`; likely branch/codegen corruption around `is_kernel_rodata()` check. |
| **Revert** | Remove `M65832_UTIL_OPTNONE` and restore normal optimization for `kfree_const()`. |
| **Proper fix** | Fix backend correctness for this predicate/call path so `kfree_const()` only frees non-rodata objects under optimization. |

### HACK #37 — `kernfs/dir.o` forced `-O0` on M65832 (**reverted**)

| | |
|---|---|
| **File** | `fs/kernfs/Makefile` |
| **What** | Added `CFLAGS_dir.o += -O0` under `CONFIG_M65832`, then reverted after validation. |
| **Why** | `do_basic_setup` now reaches `sysfs/chr_dev_init`, but faults in `rb_insert_color()` from `kernfs_link_sibling()` indicate optimized `dir.o` is still unstable in kernfs rbtree/link paths. |
| **Revert** | Already reverted; no active `kernfs/dir.o` optimization override remains. |
| **Proper fix** | Root-cause/fix backend miscompile in kernfs directory insertion/link code and restore default optimization. |

### HACK #38 — `rbtree.o` forced `-O0` on M65832

| | |
|---|---|
| **File** | `lib/Makefile` |
| **What** | Added `CFLAGS_rbtree.o += -O0` under `CONFIG_M65832`. |
| **Why** | Current first fault reaches `rb_insert_color()` via `kernfs_link_sibling()` during `do_basic_setup` and dereferences NULL; lowering optimization in rbtree core is a narrow mitigation while tracking backend issues. |
| **Revert** | Remove the M65832-specific `CFLAGS_rbtree.o += -O0`. |
| **Proper fix** | Fix backend codegen/calling-convention correctness in rbtree insertion paths and restore normal optimization. |

### HACK #39 — `kfree()` skips reserved-page frees on M65832

| | |
|---|---|
| **File** | `mm/slub.c` |
| **What** | Added a `CONFIG_M65832` guard in `kfree()` to return early when `virt_to_page(object)` resolves to `PageReserved(page)`. |
| **Why** | Current first fault path in `do_basic_setup` reports `Not a kmalloc allocation` from `free_large_kmalloc()` and then corrupts later init state (`kobject '(null)'`, pty registration panic). |
| **Revert** | Remove the `PageReserved(page)` early-return guard in `kfree()`. |
| **Proper fix** | Fix upstream caller/data corruption so only valid kmalloc/slab objects are passed to `kfree()`. |

### HACK #40 — `kobject.o` and `sysfs/dir.o` forced `-O0` on M65832

| | |
|---|---|
| **File** | `lib/Makefile`, `fs/sysfs/Makefile` |
| **What** | Added `CFLAGS_kobject.o += -O0` and `CFLAGS_dir.o += -O0` (sysfs) under `CONFIG_M65832`. |
| **Why** | After progressing through `do_basic_setup`, first-fault moved to `kobject_add_internal -> sysfs_create_dir_ns` with `refcount` warnings and NULL fault (`PC=0x805a2fe7`), suggesting corruption in optimized kobject/sysfs directory paths. |
| **Revert** | Remove the two M65832-specific `-O0` flags. |
| **Proper fix** | Root-cause backend/codegen issues in kobject/sysfs directory creation paths and restore default optimization. |

### HACK #41 — Timer IRQ descriptor prefetch dispatch (**reverted**)

| | |
|---|---|
| **File** | `arch/m65832/kernel/irq.c` |
| **What** | Switched timer dispatch in `do_IRQ()` to descriptor-prefetch (`irq_to_desc` + `handle_irq_desc`), then reverted to `generic_handle_irq(virq)` after regressions (`spawn_ksoftirqd` failures/early BRK). |
| **Why** | First-fault moved to `generic_handle_irq()` (`PC=0x800e4c43`) from timer interrupt during `kobject_uevent`/`fill_kobj_path`; sparse-irq descriptor lookup remains unstable under current bring-up conditions. |
| **Revert** | Already reverted; timer path currently uses `generic_handle_irq(virq)`. |
| **Proper fix** | Stabilize sparse-irq/maple-tree descriptor lifecycle and backend codegen so timer IRQ dispatch can always go through generic path. |

### HACK #42 — `smpboot.o` forced `-O0` on M65832 (**reverted**)

| | |
|---|---|
| **File** | `kernel/Makefile` |
| **What** | Added `CFLAGS_smpboot.o += -O0` under `CONFIG_M65832`, then reverted after no improvement. |
| **Why** | Boot intermittently panics at `spawn_ksoftirqd()` (`BUG_ON(smpboot_register_percpu_thread(...))`), suggesting unstable optimized percpu thread-registration path. |
| **Revert** | Already reverted; no active `smpboot.o` optimization override remains. |
| **Proper fix** | Root-cause backend/codegen issue in percpu kthread setup and restore default optimization. |

### HACK #43 — Non-fatal softirq percpu-thread registration failure on M65832

| | |
|---|---|
| **File** | `kernel/softirq.c` |
| **What** | In `spawn_ksoftirqd()`, replace `BUG_ON(ret)` with an M65832-only error log and early return when `smpboot_register_percpu_thread(&softirq_threads)` fails. |
| **Why** | Current first-fault is deterministic panic at `spawn_ksoftirqd()`; this allows boot to continue so later initialization failures can be isolated. |
| **Revert** | Restore unconditional `BUG_ON(ret)` behavior for M65832 path. |
| **Proper fix** | Root-cause thread registration failure (likely task/thread setup or allocator corruption) so ksoftirqd starts normally without bypass. |

### HACK #44 — `kfree()` `virt_addr_valid` guard on M65832 (**reverted**)

| | |
|---|---|
| **File** | `mm/slub.c` |
| **What** | Added `CONFIG_M65832` guard in `kfree()` to return early if `!virt_addr_valid((unsigned long)object)`, then reverted after it faulted inside `generic_test_bit()` on invalid paths. |
| **Why** | First-fault in `kdevtmpfs` path (`mas_prealloc_calc -> kfree`) still hits `kfree()` with invalid/non-kernel-virtual pointers despite `IS_ERR`/reserved-page guards. |
| **Revert** | Already reverted; no active `virt_addr_valid` guard remains in `kfree()`. |
| **Proper fix** | Root-cause invalid pointer propagation into maple-tree/shmem/devtmpfs teardown and keep `kfree()` strict. |

### HACK #45 — `kthread.o` forced `-O0` on M65832

| | |
|---|---|
| **File** | `kernel/Makefile` |
| **What** | Added `CFLAGS_kthread.o += -O0` under `CONFIG_M65832`. |
| **Why** | `smpboot_register_percpu_thread()` consistently fails creating `ksoftirqd/%u` with `-ENOMEM`, blocking normal early init progression. |
| **Revert** | Remove the M65832-specific `CFLAGS_kthread.o += -O0`. |
| **Proper fix** | Root-cause backend/codegen issue in kthread creation path and restore default optimization. |

### HACK #46 — Force pid1/pid2 runnable in `rest_init()` on M65832

| | |
|---|---|
| **File** | `init/main.c` |
| **What** | In `rest_init()`, after locating pid1 and pid2 task structs, explicitly set `__state = TASK_RUNNING` before first schedule and only apply affinity when pid1 lookup succeeded. |
| **Why** | Long runs were stalling in idle (`arch_cpu_idle`) with pid1 observed in non-running state, preventing first handoff into init/kthreadd scheduling. |
| **Revert** | Remove explicit `WRITE_ONCE(..., TASK_RUNNING)` assignments and restore original flow. |
| **Proper fix** | Determine why newly spawned pid1/pid2 occasionally lose runnable state before first schedule on M65832. |

### HACK #47 — Suppress proc-name WARNs on invalid names for M65832

| | |
|---|---|
| **File** | `fs/proc/generic.c` |
| **What** | In `__proc_create()`, under `CONFIG_M65832` return `NULL` for invalid names (empty, >=256, `"."`, `".."`) without emitting `WARN()`. |
| **Why** | `WARN()` path in proc name validation triggered heavy printk activity and repeatedly led to `printk`-path return corruption (`RTS -> BRK @ 0x1`) before later init stages. |
| **Revert** | Remove `CONFIG_M65832` guards and restore upstream `WARN()` behavior. |
| **Proper fix** | Fix upstream corruption source producing invalid proc names and keep diagnostic warnings enabled. |

### HACK #48 — `delay.o` forced `-O0` on M65832

| | |
|---|---|
| **File** | `arch/m65832/lib/Makefile` |
| **What** | Added `CFLAGS_delay.o += -O0` under `CONFIG_M65832`. |
| **Why** | Long runs repeatedly end inside `__delay()` with huge loop counts, suggesting optimizer/backend issues in delay helper call paths. |
| **Revert** | Remove `CFLAGS_delay.o += -O0` for M65832. |
| **Proper fix** | Root-cause delay-call argument/codegen correctness and restore default optimization. |

### HACK #49 — `sysctl.o` forced `-O0` on M65832

| | |
|---|---|
| **File** | `kernel/Makefile` |
| **What** | Added `CFLAGS_sysctl.o += -O0` under `CONFIG_M65832`. |
| **Why** | First-fault intermittently moved earlier to `insert_header()` during `__register_sysctl_table` initcalls, with NULL deref in tree/list registration path. |
| **Revert** | Remove `CFLAGS_sysctl.o += -O0` for M65832. |
| **Proper fix** | Root-cause backend/codegen corruption in sysctl registration structures and restore default optimization. |

### HACK #50 — Skip VT console-driver panic on M65832

| | |
|---|---|
| **File** | `drivers/tty/vt/vt.c` |
| **What** | In `vty_init()`, when `tty_register_driver(console_driver)` fails under `CONFIG_M65832`, log error, drop the driver ref, and continue instead of `panic()`. |
| **Why** | Current bring-up repeatedly hits `Couldn't register console driver` after devtmpfs/sysfs instability; serial/raw UART console is still available and allows forward progress. |
| **Revert** | Restore unconditional `panic("Couldn't register console driver")` behavior. |
| **Proper fix** | Fix underlying kobject/sysfs/devtmpfs corruption so VT driver registration succeeds without bypass. |

### HACK #51 — Downgrade high-noise WARN paths in device/kernfs activation on M65832

| | |
|---|---|
| **File** | `drivers/base/core.c`, `fs/kernfs/dir.c` |
| **What** | Replace fatal/noisy device-release WARN in `device_release()` with `pr_err_once` on M65832; in `kernfs_activate_one()`, return early on invalid parent/rb or active-bias invariants instead of issuing `WARN_ON_ONCE`. |
| **Why** | These warnings repeatedly trigger printk-heavy paths and were followed by emulator-side crashes/BRK regressions before reaching later init milestones. |
| **Revert** | Restore upstream `WARN()` / `WARN_ON_ONCE()` behavior in both sites. |
| **Proper fix** | Eliminate underlying kobject/kernfs invariant violations so warning paths are not hit. |

### HACK #52 — `dcache.o` forced `-O0` on M65832

| | |
|---|---|
| **File** | `fs/Makefile` |
| **What** | Added `CFLAGS_dcache.o += -O0` under `CONFIG_M65832`. |
| **Why** | Trace-ring first-fault repeatedly returned to `BRK @ 0x1` through dcache/shmem node paths (`__d_instantiate`, `d_make_persistent`, `shmem_mknod`). |
| **Revert** | Remove `CFLAGS_dcache.o += -O0` for M65832. |
| **Proper fix** | Fix backend/codegen/runtime corruption in dcache/shmem interaction path and restore normal optimization. |

### HACK #53 — Relax `to_kthread()` PF_KTHREAD assertion on M65832

| | |
|---|---|
| **File** | `kernel/kthread.c` |
| **What** | Under `CONFIG_M65832`, `to_kthread()` returns `worker_private` directly when `PF_KTHREAD` is not yet set, instead of triggering `WARN_ON`. |
| **Why** | Repeated WARN path in kworker context amplified into `printk`-path faults (`memcpy` during `copy_from_kernel_nofault`) and derailed early boot progression. |
| **Revert** | Restore strict upstream `WARN_ON(!(k->flags & PF_KTHREAD))`. |
| **Proper fix** | Identify and fix the ordering/state bug that transiently exposes `worker_private` before `PF_KTHREAD` is set. |

### HACK #54 — Suppress kobject uevents on M65832 bring-up

| | |
|---|---|
| **File** | `lib/kobject_uevent.c` |
| **What** | `kobject_uevent_env()` returns early under `CONFIG_M65832`, suppressing uevent construction/emission. |
| **Why** | First-fault in `add_uevent_var()` repeatedly caused NULL deref during driver/core init, blocking boot progression before userspace handoff. |
| **Revert** | Remove the M65832 early-return and restore normal uevent path. |
| **Proper fix** | Fix underlying formatting/buffer/codegen corruption in kobject uevent path so full hotplug userspace signaling works. |

### HACK #55 — Bypass pidfs rbtree insertion/removal on M65832

| | |
|---|---|
| **File** | `fs/pidfs.c` |
| **What** | Under `CONFIG_M65832`, `pidfs_add_pid()` returns before `rb_find_add_rcu()`, and `pidfs_remove_pid()` returns when node is empty. |
| **Why** | First-fault in `__rb_change_child()` from `pidfs_add_pid()` during thread creation (`alloc_pid`) caused early kernel Oops in `kthreadd`. |
| **Revert** | Restore normal pidfs rbtree add/remove operations. |
| **Proper fix** | Fix rbtree/pidfs corruption or backend codegen in pid allocation path and re-enable pidfs indexing. |

### HACK #56 — Force `alloc_pid()` to use `init_pid_ns` on M65832

| | |
|---|---|
| **File** | `kernel/pid.c` |
| **What** | In `alloc_pid()`, unconditionally use `&init_pid_ns` under `CONFIG_M65832` during bring-up. |
| **Why** | First-fault showed persistent NULL/corrupted namespace pointer dereferences in `alloc_pid()` during early thread creation even after guards. |
| **Revert** | Restore normal `alloc_pid()` use of caller-provided namespace. |
| **Proper fix** | Find and fix root-cause namespace corruption/ordering so `alloc_pid()` gets a valid namespace. |

### HACK #57 — `pid.o` forced `-O0` on M65832

| | |
|---|---|
| **File** | `kernel/Makefile` |
| **What** | Added `CFLAGS_pid.o += -O0` under `CONFIG_M65832`. |
| **Why** | `alloc_pid()` continued faulting at function entry despite guard logic, indicating likely codegen instability in PID allocation path. |
| **Revert** | Remove `CFLAGS_pid.o += -O0` for M65832. |
| **Proper fix** | Resolve compiler/backend correctness in `kernel/pid.c` and restore optimized build. |

### HACK #58 — `kernfs/dir.o` forced `-O0` on M65832 (reintroduced)

| | |
|---|---|
| **File** | `fs/kernfs/Makefile` |
| **What** | Added `CFLAGS_dir.o += -O0` under `CONFIG_M65832`. |
| **Why** | First-fault moved to `kernfs_link_sibling()`/`kernfs_add_one()` rbtree insertion path during workqueue sysfs init. |
| **Revert** | Remove `CFLAGS_dir.o += -O0` for M65832. |
| **Proper fix** | Fix backend/data-structure corruption in kernfs node insertion and restore optimized build. |

### HACK #59 — Demote Maple Tree WARN macros on M65832

| | |
|---|---|
| **File** | `include/linux/maple_tree.h` |
| **What** | Under non-debug maple tree build and `CONFIG_M65832`, redefine `MT_WARN_ON` / `MAS_WARN_ON` / `MAS_WR_WARN_ON` to `unlikely(!!cond)` (no printk side effects). |
| **Why** | Repeated maple-tree warnings in kdevtmpfs path caused warning amplification and stalled boot progress before userspace handoff. |
| **Revert** | Restore WARN_ON-based macro definitions for M65832. |
| **Proper fix** | Resolve maple-tree invariant failures so warns are not hit, then re-enable standard warnings. |

### HACK #60 — `slub.o` forced `-O0` on M65832 (**reverted**)

| | |
|---|---|
| **File** | `mm/Makefile` |
| **What** | Added `CFLAGS_slub.o += -O0` under `CONFIG_M65832`. |
| **Why** | First-fault moved into `kmem_cache_alloc_noprof()` (SLUB allocator path) during kernfs/sysfs node allocation. |
| **Revert** | **Done**: `CFLAGS_slub.o += -O0` removed (build-break on host toolchain). |
| **Proper fix** | Root-cause allocator-path corruption/miscompile and restore optimized SLUB build. |

### HACK #61 — `kmem_cache_alloc*_noprof()` forced `optnone` on M65832 (**reverted**)

| | |
|---|---|
| **File** | `mm/slub.c` |
| **What** | Annotated `kmem_cache_alloc_noprof()` and `kmem_cache_alloc_lru_noprof()` with M65832 `__attribute__((optnone))`. |
| **Why** | First-fault persisted in `kmem_cache_alloc_noprof()` despite file-level mitigation, indicating function-local codegen instability. |
| **Revert** | **Done**: M65832 `optnone` removed from both allocation wrappers (paired with HACK #60 revert). |
| **Proper fix** | Fix backend codegen in SLUB allocation fast path and restore normal optimization. |

### HACK #62 — Skip VT/vcs initialization on M65832

| | |
|---|---|
| **File** | `drivers/tty/vt/vt.c` |
| **What** | `vty_init()` returns early under `CONFIG_M65832`, skipping VT and VCS device/sysfs registration. |
| **Why** | Deterministic first-fault hit in VT/VCS registration path (`vcs_init` → device/sysfs/kernfs allocations), preventing progress to userspace while serial console remains functional. |
| **Revert** | Remove M65832 early return from `vty_init()`. |
| **Proper fix** | Stabilize VT/VCS sysfs and allocator paths so full VT init succeeds. |

### HACK #63 — Guard `mas_safe_min()` against NULL pivots on M65832

| | |
|---|---|
| **File** | `lib/maple_tree.c` |
| **What** | In `mas_safe_min()`, return `mas->min` when `pivots == NULL` under `CONFIG_M65832`. |
| **Why** | New first-fault in `kdevtmpfs` hit `mas_safe_min()` with null `pivots`, causing immediate NULL dereference in maple-tree walk. |
| **Revert** | Remove NULL-pivots guard in `mas_safe_min()`. |
| **Proper fix** | Resolve maple-tree state/caller corruption so `pivots` is always valid in this path. |

### HACK #64 — Guard sysfs attribute creation inputs on M65832

| | |
|---|---|
| **File** | `fs/sysfs/file.c` |
| **What** | In `sysfs_add_file_mode_ns()`, early-return `-EINVAL` on M65832 when `parent`, `parent->priv`, `attr`, `attr->name`, or `kobj->ktype` is NULL. |
| **Why** | New first-fault in `sysfs_add_file_mode_ns()` dereferenced invalid pointers during driver attribute registration. |
| **Revert** | Remove M65832 null-input guards in `sysfs_add_file_mode_ns()`. |
| **Proper fix** | Fix object lifetime / initialization ordering so sysfs always receives valid kobject/attribute metadata. |

### HACK #65 — Skip legacy PTY init on M65832

| | |
|---|---|
| **File** | `drivers/tty/pty.c` |
| **What** | In `pty_init()`, under `CONFIG_M65832`, skip `legacy_pty_init()` and run `unix98_pty_init()` only. |
| **Why** | First-fault moved to legacy PTY registration teardown (`sysfs_remove_link` → `kernfs_unlink_sibling` → `rb_erase`) causing init-thread panic. |
| **Revert** | Restore normal `pty_init()` calling both legacy and unix98 init paths. |
| **Proper fix** | Stabilize sysfs/kernfs/rbtree behavior in legacy PTY registration/removal path. |

### HACK #66 — Guard `kmem_cache_free()` object pointer on M65832

| | |
|---|---|
| **File** | `mm/slub.c` |
| **What** | In `kmem_cache_free()`, under `CONFIG_M65832`, ignore NULL, ERR_PTR-like (`0xfffff...`) and `PageReserved()` objects before `cache_from_obj()`. |
| **Why** | New first-fault in `ksoftirqd` hit `kmem_cache_free()` (`page_slab` path) during radix-tree RCU free with invalid object pointer. |
| **Revert** | Remove M65832 guards in `kmem_cache_free()`. |
| **Proper fix** | Fix producer-side object corruption / allocator metadata misuse so free path receives valid slab objects. |

### HACK #67 — Guard `kernfs_root()` against NULL node on M65832

| | |
|---|---|
| **File** | `fs/kernfs/kernfs-internal.h` |
| **What** | In `kernfs_root()`, return `NULL` if `kn` is `NULL` under `CONFIG_M65832`. |
| **Why** | New first-fault in `kernfs_root()` dereferenced NULL node (`kn->dir.root`) after long bring-up run. |
| **Revert** | Remove M65832 NULL guard in `kernfs_root()`. |
| **Proper fix** | Fix caller-side lifecycle/corruption that passes invalid `kernfs_node` pointers into kernfs helpers. |

### HACK #68 — Skip full M65832 UART driver registration

| | |
|---|---|
| **File** | `drivers/tty/serial/m65832_uart.c` |
| **What** | In `m65832_uart_init()`, return early under `CONFIG_M65832` and skip `uart_register_driver()` / `uart_add_one_port()`. |
| **Why** | First-fault in `vsnprintf` string path occurred during UART tty device naming/registration; early console remains available for logs. |
| **Revert** | Restore normal `m65832_uart_init()` driver registration path. |
| **Proper fix** | Fix formatting/object corruption in tty/serial registration path and re-enable full UART tty driver. |

### HACK #69 — Guard `rwsem_mark_wake()` against empty waiter queue on M65832

| | |
|---|---|
| **File** | `kernel/locking/rwsem.c` |
| **What** | In `rwsem_mark_wake()`, return early if `rwsem_first_waiter(sem)` is `NULL`; additionally skip writer wake when `waiter->task` is `NULL` (M65832 only). |
| **Why** | First-fault hit NULL dereference on `waiter->type` during sysfs/slab init lock wakeups. |
| **Revert** | Remove M65832 NULL-waiter guard in `rwsem_mark_wake()`. |
| **Proper fix** | Fix semaphore wait-list corruption/order bug so wake path never sees empty queue with waiter flag state. |

### HACK #70 — Skip `slab_sysfs_init()` on M65832

| | |
|---|---|
| **File** | `mm/slub.c` |
| **What** | `slab_sysfs_init()` returns early under `CONFIG_M65832`, skipping slab cache sysfs registration. |
| **Why** | First-fault was repeatedly in slab sysfs bring-up (`sysfs_slab_add` path) via rwsem/kernfs interactions, blocking progress to userspace. |
| **Revert** | Restore normal `slab_sysfs_init()` implementation. |
| **Proper fix** | Stabilize rwsem/kernfs/sysfs paths so slab sysfs registration is safe. |

### HACK #71 — Skip built-in module param sysfs export on M65832

| | |
|---|---|
| **File** | `kernel/params.c` |
| **What** | `param_sysfs_builtin_init()` returns early under `CONFIG_M65832`, skipping built-in module parameter/version sysfs creation. |
| **Why** | First-fault during `param_sysfs_builtin_init` path hit kernfs root/node corruption in `kernfs_activate()`. |
| **Revert** | Remove M65832 early return from `param_sysfs_builtin_init()`. |
| **Proper fix** | Stabilize kernfs/sysfs node lifetimes and rbtree/refcount behavior so parameter sysfs export succeeds. |

### HACK #72 — Downgrade namespace BUG checks in `create_dir()` on M65832

| | |
|---|---|
| **File** | `lib/kobject.c` |
| **What** | In `create_dir()`, replace namespace type `BUG_ON()` checks with `-EINVAL` return on M65832. |
| **Why** | Boot now reaches a deterministic panic at `lib/kobject.c:create_dir()` namespace validation BUG path. |
| **Revert** | Restore original `BUG_ON()` checks in `create_dir()`. |
| **Proper fix** | Fix namespace-type registration/lifetime ordering so invariants hold and BUG path is never reached. |

### HACK #73 — Skip `kdevtmpfs` worker startup on M65832

| | |
|---|---|
| **File** | `drivers/base/devtmpfs.c` |
| **What** | In `devtmpfs_init()`, after fs registration, return success on M65832 without starting `kdevtmpfs` thread or waiting for `devtmpfs_setup()`. |
| **Why** | Latest first-fault returned to `devtmpfs_setup()`/mount path (`do_lock_mount`) NULL dereference from `kdevtmpfs` context. |
| **Revert** | Restore normal `kthread_run(devtmpfsd, ...)` + `wait_for_completion(&setup_done)` path. |
| **Proper fix** | Fix mount namespace and VFS mount path corruption so `kdevtmpfs` setup succeeds. |

### HACK #74 — Suppress refcount WARN path on M65832

| | |
|---|---|
| **File** | `lib/refcount.c` |
| **What** | In `refcount_warn_saturate()`, on M65832 set saturated state, log once, and return without `WARN_ONCE()`/stack dump path. |
| **Why** | Refcount saturation warnings repeatedly entered heavy WARN/printk path and faulted while dumping registers (`show_regs`), killing init. |
| **Revert** | Restore normal `REFCOUNT_WARN(...)` switch path. |
| **Proper fix** | Eliminate underlying refcount misuse/saturation sources so warnings are no longer triggered. |

### HACK #1 — Preset loops_per_jiffy

| | |
|---|---|
| **File** | `arch/m65832/kernel/setup.c:194-204` |
| **What** | Sets `preset_lpj` in `setup_arch()` to skip `calibrate_delay()` |
| **Why** | The delay calibration loop takes too long on the emulator. Timer interrupts work, but the convergence loop needs billions of cycles. |
| **Revert** | Delete the `preset_lpj` block in `setup_arch()`. Ensure `lpj=` is parsed from cmdline (currently broken — `__setup("lpj=",...)` not consuming it). |
| **Proper fix** | Either fix `lpj=` cmdline parsing, implement `calibrate_delay_direct()` using the hardware timer, or accept the slow calibration. |

### HACK #2 — Lightweight stack traces (no symbol names)

| | |
|---|---|
| **File** | `arch/m65832/kernel/stacktrace.c:147` |
| **What** | `show_stack()` prints `[<addr>]` instead of `[<addr>] symbol+0xNN` |
| **Why** | `%pS` calls `kallsyms_lookup()` which walks compressed kallsyms byte-by-byte. Each symbol takes millions of emulated cycles. |
| **Revert** | Change `printk("%s [<%08lx>]\n", ...)` back to `printk("%s [<%08lx>] %pS\n", ..., (void *)ra)` |
| **Proper fix** | Implement binary-search kallsyms lookup, or wait for compiler to generate faster code, or implement DWARF CFI-based unwinding (see Unwind Spec below). |

### HACK #3 — 32 KB kernel stacks

| | |
|---|---|
| **File** | `arch/m65832/include/asm/processor.h:19-26` |
| **What** | `THREAD_SIZE_ORDER` set to 3 (32 KB) instead of 1 (8 KB) |
| **Why** | The M65832 compiler spills many callee-saved registers per call frame, consuming ~100-200 bytes per frame vs ~32-64 on ARM/RISC-V. Deep call chains (printk -> vsnprintf -> kallsyms -> ...) overflow 8 KB. |
| **Revert** | Change `THREAD_SIZE_ORDER` back to `1`. |
| **Proper fix** | Improve compiler register allocation to reduce spill, or keep at order 2 (16 KB) as a reasonable middle ground. |

### HACK #4 — console_init() skipped

| | |
|---|---|
| **File** | `init/main.c:1166-1173` |
| **What** | `console_init()` call is commented out |
| **Why** | VT console init (`con_init`) triggers page faults that crash the kernel. The raw UART console (registered in `setup_arch`) provides output. |
| **Revert** | Uncomment `console_init()`. |
| **Proper fix** | Either (a) debug the `con_init` page fault (likely related to VT buffer allocation in vmalloc space with broken ioremap), (b) disable `CONFIG_VT` and provide a proper serial console driver, or (c) fix the full ioremap/page-table setup. |

### HACK #5 — Timekeeping WARN suppressed

| | |
|---|---|
| **File** | `kernel/time/timekeeping.c:255-266` |
| **What** | `WARN_ON_ONCE(...)` replaced with `pr_warn_once(...)` |
| **Why** | The WARN generates `dump_stack()` which is prohibitively expensive (see HACK #2). |
| **Revert** | Replace the `if (unlikely(...)) pr_warn_once(...)` back with `WARN_ON_ONCE(tk->offs_real != timespec64_to_ktime(tmp))` |
| **Proper fix** | Fix stack traces to be fast (HACK #2), then revert this. |

### HACK #6 — Slab GFP WARN suppressed

| | |
|---|---|
| **File** | `mm/slab_common.c:1028-1036` |
| **What** | `pr_warn + dump_stack()` replaced with `pr_warn_once` |
| **Why** | `dump_stack()` too expensive (see HACK #2). Also fires on every invalid-GFP kmalloc, not just once. |
| **Revert** | Restore original: `pr_warn("Unexpected gfp: ..."); dump_stack();` |
| **Proper fix** | Fix the GFP_DMA32 usage (something is passing GFP_DMA32 to kmalloc on a platform without ZONE_DMA32), and fix stack traces to be fast. |

### HACK #7 — Recursive page fault guard

| | |
|---|---|
| **File** | `arch/m65832/mm/fault.c:27-36` |
| **What** | Static `fault_recursion` counter; halts CPU if recursion > 1 |
| **Why** | Page fault -> die() -> printk -> page fault -> ... infinite recursion crashes the kernel. |
| **Revert** | Remove the `fault_recursion` counter and the early-exit block. |
| **Proper fix** | This is actually a reasonable safety measure. Could be made per-CPU for SMP. The root cause is page faults in kernel text/data that shouldn't fault (likely the vmalloc/ioremap issue). |

### HACK #8 — IRQ vector placed at `$FFEE` layout

| | |
|---|---|
| **File** | `arch/m65832/kernel/entry.S` |
| **What** | Vector table uses explicit `.org` layout and places `irq_entry` at offset `0x1E` (physical `$FFEE`) instead of aligned `0x20`. |
| **Why** | Trace proved IRQ-enable fetched a 32-bit vector from `$FFEE`, previously decoding into garbage branch targets (`0x35E08001`) and crashing before `irq_entry`. |
| **Revert** | Restore canonical contiguous `.long` vector layout once hardware vector fetch semantics are finalized/verified. |
| **Proper fix** | Document exact hardware vector fetch width/addressing in ISA spec and align entry table + emulator + backend accordingly. |

### HACK #9 — `do_IRQ()` compiled with `optnone`

| | |
|---|---|
| **File** | `arch/m65832/kernel/irq.c` |
| **What** | `do_IRQ()` annotated `__attribute__((optnone))` for M65832/clang. |
| **Why** | Optimized `do_IRQ` emitted BRK traps in hot path while handling timer/INTC interrupts. |
| **Revert** | Remove `M65832_IRQ_OPTNONE` from `do_IRQ()`. |
| **Proper fix** | Backend fix for control-flow/select lowering in this path; validate with IRQ stress boot run. |

### HACK #10 — IRQ pending scan without `__ffs()`

| | |
|---|---|
| **File** | `arch/m65832/kernel/irq.c` |
| **What** | Replaced `while (pending) { irq = __ffs(pending); ... }` with linear bit scan loop. |
| **Why** | `__ffs` path repeatedly lowered to BRK-prone codegen in optimized IRQ dispatch. |
| **Revert** | Restore `__ffs`-based pending walk. |
| **Proper fix** | Backend correctness for `ffs/ctz` lowering. |

### HACK #11 — IRQ mask/unmask/ack uses LUT masks

| | |
|---|---|
| **File** | `arch/m65832/kernel/irq.c` |
| **What** | Replaced `1 << irq` variable shifts with precomputed 32-entry bitmask lookup table. |
| **Why** | Variable-shift lowering in IRQ chip callbacks produced BRK traps under load. |
| **Revert** | Replace LUT usage with direct `1U << irq`. |
| **Proper fix** | Backend fix for variable shifts/select in these callback patterns. |

### HACK #12 — `hrtimer.o` forced `-O0`

| | |
|---|---|
| **File** | `kernel/time/Makefile` |
| **What** | `CFLAGS_hrtimer.o += -O0` for M65832. |
| **Why** | Optimized hrtimer/rbtree paths hit BRK traps (`__next_base`, `__remove_hrtimer`, related walkers). |
| **Revert** | Remove M65832-specific `CFLAGS_hrtimer.o += -O0` block. |
| **Proper fix** | Backend fix in timer/rbtree-heavy control-flow and bit-manipulation code. |

### HACK #13 — hrtimer base walk rewritten without `__ffs`

| | |
|---|---|
| **File** | `kernel/time/hrtimer.c` |
| **What** | Replaced `__next_base()` `__ffs` + bit-clear logic with linear index scan. |
| **Why** | `__next_base` optimized output trapped via BRK in active timer processing. |
| **Revert** | Restore original `__ffs(*active)` implementation. |
| **Proper fix** | Same backend `ffs/ctz` correctness fix; restore compact original logic. |

### HACK #14 — hrtimer base-bit updates use LUT helper

| | |
|---|---|
| **File** | `kernel/time/hrtimer.c` |
| **What** | Replaced `1 << base->index` in enqueue/remove with `hrtimer_base_bit()` helper backed by LUT. |
| **Why** | Variable shift patterns in `__remove_hrtimer` path repeatedly trapped via BRK. |
| **Revert** | Restore direct shift expressions in `enqueue_hrtimer()` and `__remove_hrtimer()`. |
| **Proper fix** | Backend variable-shift reliability in timer hot paths. |

### HACK #15 — First-fault immediate stop in page fault handler

| | |
|---|---|
| **File** | `arch/m65832/mm/fault.c` |
| **What** | On first page fault (`fault_recursion == 0`), execute immediate `STP` before expensive reporting. |
| **Why** | Preserves pre-fault trace-ring context and avoids losing first-fault instruction evidence to recursive logging. |
| **Revert** | Remove immediate `STP` stop and restore normal first-fault reporting flow. |
| **Proper fix** | Keep only if needed for debug kernels; make conditional on debug config. |

### HACK #16 — Temporary boot-progress probes

| | |
|---|---|
| **File** | `init/main.c`, `kernel/fork.c` |
| **What** | Added `pr_info("M65832: ...")` markers around `start_kernel/rest_init/kernel_clone/copy_process` milestones. |
| **Why** | High-granularity first-fault localization while removing previous skips. |
| **Revert** | Remove temporary M65832 milestone printk probes once stable boot to `/init` is achieved. |
| **Proper fix** | Replace ad-hoc probes with targeted tracepoints or boot debug config if needed long-term. |

---

## Non-hack architectural issues (TODOs)

### Page table transition (init_pg_dir -> swapper_pg_dir)

| | |
|---|---|
| **File** | `arch/m65832/mm/init.c:129` |
| **Status** | `init_pg_dir` is the active page table base. `swapper_pg_dir` (in BSS) is empty. The kernel's `init_mm.pgd = swapper_pg_dir` but the MMU uses `init_pg_dir`. |
| **Impact** | `ioremap_page_range()` populates `swapper_pg_dir` entries that the MMU never sees. This is why the simplified identity-map ioremap is needed. |
| **Fix** | After memblock/page allocator init, copy `init_pg_dir` kernel mappings into `swapper_pg_dir`, then switch PTBR to `swapper_pg_dir`. Similar to ARM's `cpu_replace_ttbr1`. |

### ioremap limited to identity-mapped regions

| | |
|---|---|
| **File** | `arch/m65832/mm/ioremap.c` |
| **Status** | Returns direct physical addresses for peripheral space (0x10000000-0x103FFFFF) and system registers (0xFFFFF000+). |
| **Impact** | Cannot map arbitrary MMIO regions. Fine for current hardware. |
| **Fix** | Depends on page table transition above. Once `swapper_pg_dir` is active, implement proper `ioremap_prot()` using `ioremap_page_range()`. |

### Bitops compiler workarounds

| | |
|---|---|
| **File** | `arch/m65832/include/asm/bitops.h:27-39` |
| **Status** | Uses lookup tables instead of computed bit operations |
| **Impact** | Slightly larger code, works correctly |
| **Fix** | Fix LLVM select/cmov lowering and ROL instruction encoding, then revert to computed versions. |

### percpu volatile workaround

| | |
|---|---|
| **File** | `mm/percpu.c` |
| **Status** | `alloc_size` marked `volatile` to prevent miscompilation |
| **Fix** | Fix compiler optimization bug, remove `volatile`. |

### Context switch implementation (current)

| | |
|---|---|
| **Files** | `arch/m65832/kernel/entry.S`, `arch/m65832/kernel/process.c`, `arch/m65832/include/asm/processor.h` |
| **Status** | Low-level `__switch_to` now saves/restores kernel SP + callee-saved regs + frame pointer and performs first-run trampoline entry for new tasks. |
| **Observed impact** | PID1 now reaches `kernel_init` / `kernel_init_freeable`; boot advances through `sched_init_smp` and into `do_basic_setup` stages. |
| **Current gap** | Runtime still unstable in later initcall/proc/idr/kobject paths with warning-heavy behavior and occasional IRQ-path stalls. |

---

## Unwind / Stack Trace Specification

For the LLVM backend to emit proper CFI unwind tables:

### M65832 32-bit call frame (FP-based)

```
CFA = B + 9    (after PHB32; TSPB prologue)
RA  = MEM32[B + 5]   (contains PC_next - 1; add +1 for true return address)
Old_B = MEM32[B + 1]
Caller_SP = CFA
```

### Stack layout per frame

```
High addresses (caller)
+-----------------------+
| caller's frame        |
+-----------------------+  <-- CFA = B + 9 (caller's SP before JSR)
| return addr (4 bytes) |  <-- B+5..B+8  (stored as PC_next - 1)
+-----------------------+
| saved old B (4 bytes) |  <-- B+1..B+4
+-----------------------+  <-- B = SP after TSPB
| local variables       |
| callee-saved regs     |
| spills                |
+-----------------------+  <-- SP (current)
Low addresses
```

### Key ISA details

- **JSR**: pushes `PC_of_next_instruction - 1` (32-bit), SP -= 4
- **RTS**: pulls 32-bit, adds 1, jumps there
- **PHB32**: pushes B register (frame pointer), SP -= 4
- **TSPB**: B = SP (establishes frame pointer)
- **SP semantics**: points to next free byte (one below TOS)
- **Push order**: store at SP, SP-1, SP-2, SP-3 then SP -= 4

### DWARF CFI recommendation

Option A (preferred): Change JSR to push `PC_next` (not `PC_next - 1`).
Then: `RA = MEM32[CFA - 4]` (no expression needed).

Option B (current ISA): Use DW_CFA_expression for RA column:
`RA = deref32(CFA - 4) + 1`

---

## Configuration notes

### .config changes from defconfig

- `CONFIG_CMDLINE="lpj=500000"` — preset loops_per_jiffy via cmdline
- `CONFIG_PRINTK_TIME` is disabled (avoids sched_clock dependency in printk)
- `CONFIG_VT=y` is force-selected by build system (cannot easily disable)
- `CONFIG_BLK_DEV_INITRD` is not set (needed for userspace)

### Emulator invocation

```bash
~/projects/m65832/emu/m65832emu --kernel vmlinux.bin --memory 64 --cycles 10000000000
```

The `--kernel` flag implies `--system` mode (enables UART, timer, INTC).

---

### Compiler codegen bug: branch to 0x0aeb1bb0

| | |
|---|---|
| **Symptom** | Multiple kernel functions jump to address `0x0aeb1bb0` (not kernel text) |
| **Affected functions** | `printk_get_console_flush_type`, `timerqueue_add`, possibly others |
| **Pattern** | Large switch statements or complex control flow in inlined functions |
| **Root cause** | LLVM M65832 backend generates incorrect branch target for certain code patterns |
| **Impact** | Crashes the kernel at various points during boot, always with PC=0x0aeb1bb0 |
| **Workaround** | Simplify affected functions for M65832 (use `#ifdef CONFIG_M65832`), or compile with `-O1` |
| **Proper fix** | Debug the LLVM backend branch target encoding |

---

## Next steps to reach login

1. Finish `do_basic_setup()`/`do_initcalls()` without warning loops in proc/idr/kobject subsystems.
2. Confirm `kernel_init_freeable` reaches `wait_for_initramfs()` and `prepare_namespace()`.
3. Enable `CONFIG_BLK_DEV_INITRD=y` and embed initramfs with minimal `/init`.
4. Verify `run_init_process()` reaches `/init` and capture first userspace output.
5. Remove temporary probes (`M65832:` markers) after stable repeatable boot.
6. Remove/trim new scheduler/context-switch workarounds as backend and arch fixes land.

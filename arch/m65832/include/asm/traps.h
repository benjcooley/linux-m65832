/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Trap/exception definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_TRAPS_H
#define _ASM_M65832_TRAPS_H

#include <asm/ptrace.h>

/* Exception handlers */
asmlinkage void do_page_fault(struct pt_regs *regs, unsigned long address);
asmlinkage void do_illegal_insn(struct pt_regs *regs);
asmlinkage void do_nmi(struct pt_regs *regs);
asmlinkage void do_IRQ(struct pt_regs *regs);

/* Trap initialization */
void __init trap_init(void);

/* Die function for fatal errors */
void die(const char *str, struct pt_regs *regs, long err);

/* Spinlock for die */
extern spinlock_t die_lock;

#endif /* _ASM_M65832_TRAPS_H */

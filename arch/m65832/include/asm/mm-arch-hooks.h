/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_M65832_MM_ARCH_HOOKS_H
#define _ASM_M65832_MM_ARCH_HOOKS_H

/*
 * M65832 doesn't need any arch-specific memory management hooks.
 */
static inline void arch_pick_mmap_layout(struct mm_struct *mm,
					 struct rlimit *rlim_stack)
{
}

#endif /* _ASM_M65832_MM_ARCH_HOOKS_H */

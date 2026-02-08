/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Current task pointer access for the M65832 architecture.
 *
 * For initial bring-up we use a simple global variable.
 * TODO: Use a dedicated register (R25) once inline asm syntax
 *       is finalized with the LLVM backend.
 */

#ifndef _ASM_M65832_CURRENT_H
#define _ASM_M65832_CURRENT_H

#ifndef __ASSEMBLY__

#include <linux/compiler.h>

struct task_struct;

extern struct task_struct *m65832_current_task;

#define current m65832_current_task

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_CURRENT_H */

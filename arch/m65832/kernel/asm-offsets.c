// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed to extract
 * and format the required data.
 */

#define COMPILE_OFFSETS

#include <linux/kbuild.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/thread_info.h>
#include <asm/ptrace.h>
#include <asm/processor.h>

void asm_offsets(void);

void asm_offsets(void)
{
	/* Thread info structure */
	OFFSET(TI_FLAGS, thread_info, flags);
	OFFSET(TI_PREEMPT_COUNT, thread_info, preempt_count);
	OFFSET(TI_CPU, thread_info, cpu);
	DEFINE(THREAD_INFO_SIZE, sizeof(struct thread_info));

	BLANK();

	/* Thread struct (saved context) - callee-saved registers per ABI */
	OFFSET(THREAD_KSP, thread_struct, ksp);
	OFFSET(THREAD_R16, thread_struct, r16);
	OFFSET(THREAD_R17, thread_struct, r17);
	OFFSET(THREAD_R18, thread_struct, r18);
	OFFSET(THREAD_R19, thread_struct, r19);
	OFFSET(THREAD_R20, thread_struct, r20);
	OFFSET(THREAD_R21, thread_struct, r21);
	OFFSET(THREAD_R22, thread_struct, r22);
	OFFSET(THREAD_R23, thread_struct, r23);
	OFFSET(THREAD_R24, thread_struct, r24);
	OFFSET(THREAD_R25, thread_struct, r25);
	OFFSET(THREAD_B, thread_struct, b);	/* Frame pointer (callee-saved) */
	OFFSET(THREAD_START_PC, thread_struct, start_pc);
	OFFSET(THREAD_START_ARG0, thread_struct, start_arg0);
	OFFSET(THREAD_START_ARG1, thread_struct, start_arg1);
	OFFSET(THREAD_STARTED, thread_struct, started);
	OFFSET(THREAD_FAULT_ADDRESS, thread_struct, fault_address);
	OFFSET(THREAD_FAULT_CODE, thread_struct, fault_code);
	DEFINE(THREAD_STRUCT_SIZE, sizeof(struct thread_struct));

	BLANK();

	/* pt_regs structure offsets */
	DEFINE(PT_SIZE, sizeof(struct pt_regs));
	OFFSET(PT_R0, pt_regs, r0);
	OFFSET(PT_R1, pt_regs, r1);
	OFFSET(PT_R2, pt_regs, r2);
	OFFSET(PT_R3, pt_regs, r3);
	OFFSET(PT_R4, pt_regs, r4);
	OFFSET(PT_R5, pt_regs, r5);
	OFFSET(PT_R6, pt_regs, r6);
	OFFSET(PT_R7, pt_regs, r7);
	OFFSET(PT_R8, pt_regs, r8);
	OFFSET(PT_R9, pt_regs, r9);
	OFFSET(PT_R10, pt_regs, r10);
	OFFSET(PT_R11, pt_regs, r11);
	OFFSET(PT_R12, pt_regs, r12);
	OFFSET(PT_R13, pt_regs, r13);
	OFFSET(PT_R14, pt_regs, r14);
	OFFSET(PT_R15, pt_regs, r15);
	OFFSET(PT_R16, pt_regs, r16);
	OFFSET(PT_R17, pt_regs, r17);
	OFFSET(PT_R18, pt_regs, r18);
	OFFSET(PT_R19, pt_regs, r19);
	OFFSET(PT_R20, pt_regs, r20);
	OFFSET(PT_R21, pt_regs, r21);
	OFFSET(PT_R22, pt_regs, r22);
	OFFSET(PT_R23, pt_regs, r23);
	OFFSET(PT_R24, pt_regs, r24);
	OFFSET(PT_R25, pt_regs, r25);
	OFFSET(PT_R26, pt_regs, r26);
	OFFSET(PT_R27, pt_regs, r27);
	OFFSET(PT_R28, pt_regs, r28);
	OFFSET(PT_R29, pt_regs, r29);
	OFFSET(PT_R30, pt_regs, r30);
	OFFSET(PT_A, pt_regs, a);
	OFFSET(PT_B, pt_regs, b);
	OFFSET(PT_X, pt_regs, x);
	OFFSET(PT_Y, pt_regs, y);
	OFFSET(PT_SP, pt_regs, sp);
	OFFSET(PT_PC, pt_regs, pc);
	OFFSET(PT_STATUS, pt_regs, status);
	OFFSET(PT_ORIG_R0, pt_regs, orig_r0);

	BLANK();

	/* Task struct offsets */
	OFFSET(TASK_THREAD, task_struct, thread);
	OFFSET(TASK_FLAGS, task_struct, flags);

	BLANK();

	/* Miscellaneous constants */
	DEFINE(THREAD_SIZE, THREAD_SIZE);
	DEFINE(PAGE_SIZE_ASM, PAGE_SIZE);

	BLANK();

	/* Status register bits */
	DEFINE(SR_IRQ_DISABLE_ASM, SR_IRQ_DISABLE);
	DEFINE(SR_SUPERVISOR_ASM, SR_SUPERVISOR);
	DEFINE(SR_COMPAT_ASM, SR_COMPAT);
	DEFINE(SR_USER_MODE_ASM, SR_USER_MODE);
	DEFINE(SR_KERNEL_MODE_ASM, SR_KERNEL_MODE);

	BLANK();

	/* Thread info flags */
	DEFINE(TIF_SYSCALL_TRACE_ASM, TIF_SYSCALL_TRACE);
	DEFINE(TIF_SIGPENDING_ASM, TIF_SIGPENDING);
	DEFINE(TIF_NEED_RESCHED_ASM, TIF_NEED_RESCHED);
	DEFINE(_TIF_WORK_MASK_ASM, _TIF_WORK_MASK);
}

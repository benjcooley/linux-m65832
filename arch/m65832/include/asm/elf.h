/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * ELF definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_ELF_H
#define _ASM_M65832_ELF_H

#include <asm/ptrace.h>

/*
 * ELF machine type for M65832
 * Using a custom value in the user-defined range
 */
#define EM_M65832		0x6583

/*
 * ELF register set definitions
 */
typedef unsigned long elf_greg_t;
#define ELF_NGREG		(sizeof(struct pt_regs) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

/* Floating point register set - 16 x 64-bit registers */
typedef double elf_fpreg_t;
#define ELF_NFPREG		16
typedef elf_fpreg_t elf_fpregset_t[ELF_NFPREG];

/*
 * ELF header flags
 */
#define EF_M65832_NONE		0x00000000

/*
 * ELF architecture settings
 */
#define ELF_CLASS		ELFCLASS32
#define ELF_DATA		ELFDATA2LSB	/* Little-endian */
#define ELF_ARCH		EM_M65832

/*
 * ELF platform string
 */
#define ELF_PLATFORM		"m65832"

/*
 * This is used to ensure we don't load something for the wrong architecture
 */
#define elf_check_arch(x)	((x)->e_machine == EM_M65832)

/*
 * These are used to set parameters in the core dumps
 */
#define ELF_EXEC_PAGESIZE	PAGE_SIZE

/*
 * Stack grows down
 */
#define ELF_ET_DYN_BASE		(TASK_SIZE / 3 * 2)

/*
 * ELF core dump support
 */
#define ELF_CORE_COPY_REGS(dest, regs)	\
	*(struct pt_regs *)&(dest) = *(regs)

/*
 * Instruction used to trigger a software breakpoint
 * BRK instruction: $00
 */
#define ELF_BREAKPOINT		{ 0x00 }

/*
 * We don't support hardware watchpoints yet
 */
#define SET_PERSONALITY(ex)	set_personality(PER_LINUX)

/*
 * Relocation types
 */
#define R_M65832_NONE		0
#define R_M65832_8		1	/* 8-bit absolute */
#define R_M65832_16		2	/* 16-bit absolute */
#define R_M65832_24		3	/* 24-bit absolute */
#define R_M65832_32		4	/* 32-bit absolute */
#define R_M65832_PCREL_8	5	/* 8-bit PC-relative */
#define R_M65832_PCREL_16	6	/* 16-bit PC-relative */
#define R_M65832_PCREL_32	7	/* 32-bit PC-relative */

#endif /* _ASM_M65832_ELF_H */

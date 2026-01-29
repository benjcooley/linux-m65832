/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Setup and boot definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_SETUP_H
#define _ASM_M65832_SETUP_H

#include <linux/init.h>

/* Maximum command line size */
#define COMMAND_LINE_SIZE	512

#ifndef __ASSEMBLY__

/*
 * Memory information passed from bootloader
 */
struct m65832_boot_info {
	unsigned long mem_start;	/* Physical start of RAM */
	unsigned long mem_size;		/* Size of RAM in bytes */
	unsigned long initrd_start;	/* Physical start of initrd (if any) */
	unsigned long initrd_size;	/* Size of initrd */
	char cmdline[COMMAND_LINE_SIZE]; /* Kernel command line */
};

extern struct m65832_boot_info boot_info;
extern char cmd_line[COMMAND_LINE_SIZE];

/*
 * Architecture setup functions
 */
void __init setup_arch(char **cmdline_p);
void __init setup_memory(void);
void __init setup_bootmem(void);
void __init paging_init(void);
void __init trap_init(void);

/*
 * Early console
 */
#ifdef CONFIG_M65832_EARLY_PRINTK
void __init setup_early_printk(void);
void early_printk(const char *fmt, ...);
#else
static inline void setup_early_printk(void) { }
#define early_printk(fmt, ...) do { } while (0)
#endif

/*
 * CPU information
 */
extern unsigned long cpu_clock_freq;
extern unsigned long timer_freq;

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_SETUP_H */

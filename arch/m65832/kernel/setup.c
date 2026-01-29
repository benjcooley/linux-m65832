// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Architecture setup and initialization.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/memblock.h>
#include <linux/console.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#include <asm/setup.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/sections.h>

/* Boot information from bootloader */
struct m65832_boot_info boot_info;

/* Kernel command line */
char cmd_line[COMMAND_LINE_SIZE] __initdata;

/* CPU clock frequency (Hz) */
unsigned long cpu_clock_freq = 100000000;	/* Default 100 MHz */
unsigned long timer_freq = 100000000;

/*
 * Early console output (before proper console is set up)
 */
#ifdef CONFIG_M65832_EARLY_PRINTK
static void __iomem *early_uart;

static void early_putchar(char c)
{
	if (!early_uart)
		return;

	/* Wait for TX ready (bit 0 of status register) */
	while (!(readb(early_uart + 4) & 0x01))
		;

	/* Write character */
	writeb(c, early_uart);
}

void __init setup_early_printk(void)
{
	early_uart = ioremap(CONFIG_M65832_EARLYCON_UART_ADDRESS, 16);
}

void early_printk(const char *fmt, ...)
{
	va_list ap;
	char buf[256];
	char *p;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	for (p = buf; *p; p++) {
		if (*p == '\n')
			early_putchar('\r');
		early_putchar(*p);
	}
}
#endif

/*
 * Parse memory information from boot info or device tree
 */
static void __init setup_memory(void)
{
	unsigned long mem_start, mem_size;

	if (boot_info.mem_size) {
		mem_start = boot_info.mem_start;
		mem_size = boot_info.mem_size;
	} else {
		/* Default: 64MB starting at 0 */
		mem_start = 0;
		mem_size = 64 * 1024 * 1024;
	}

	early_printk("M65832: Memory: %luMB @ 0x%08lx\n",
		     mem_size / (1024 * 1024), mem_start);

	/* Register memory with memblock */
	memblock_add(mem_start, mem_size);

	/* Reserve kernel memory */
	memblock_reserve(__pa(_stext), _end - _stext);

	/* Reserve initial page tables */
	/* TODO: Reserve init_pg_dir */
}

/*
 * Main architecture setup function
 * Called from start_kernel() in init/main.c
 */
void __init setup_arch(char **cmdline_p)
{
	early_printk("M65832 Linux starting...\n");

	/* Set up command line */
	if (boot_info.cmdline[0]) {
		strscpy(cmd_line, boot_info.cmdline, COMMAND_LINE_SIZE);
	} else {
#ifdef CONFIG_CMDLINE
		strscpy(cmd_line, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#else
		cmd_line[0] = '\0';
#endif
	}
	*cmdline_p = cmd_line;

	early_printk("Command line: %s\n", cmd_line);

	/* Initialize memory management */
	setup_memory();

	/* Set up initial thread info */
	/* TODO: init_mm setup */

	/* Parse device tree if present */
#ifdef CONFIG_OF
	unflatten_device_tree();
#endif

	/* Print CPU info */
	early_printk("M65832 CPU @ %lu MHz\n", cpu_clock_freq / 1000000);
}

/*
 * CPU initialization - called for each CPU
 */
void __init cpu_init(void)
{
	/* Set up exception vectors */
	/* TODO: trap_init() is called separately */

	/* Configure FPU if present */
#ifdef CONFIG_M65832_FPU
	/* TODO: FPU setup */
#endif
}

/*
 * Architecture calibration for delay loops
 */
void calibrate_delay(void)
{
	/* For now, use a fixed estimate based on CPU frequency */
	loops_per_jiffy = cpu_clock_freq / HZ;
}

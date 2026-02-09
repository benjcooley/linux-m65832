/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux - Linker script
 *
 * Virtual addresses start at PAGE_OFFSET (0x80000000).
 * Physical load addresses = virtual - LOAD_OFFSET.
 * All sections use AT() so objcopy -O binary produces
 * a flat image loadable at physical address 0.
 */

#include <asm-generic/vmlinux.lds.h>
#include <asm/page.h>
#include <asm/thread_info.h>

OUTPUT_ARCH(m65832)
ENTRY(_start)

jiffies = jiffies_64;

#define KERNEL_LINK_ADDR	PAGE_OFFSET
#define LOAD_OFFSET		(KERNEL_LINK_ADDR - PHYS_OFFSET)

SECTIONS
{
	. = KERNEL_LINK_ADDR;

	_text = .;

	/* Head section - must be first */
	.head.text : AT(ADDR(.head.text) - LOAD_OFFSET) {
		_stext = .;
		HEAD_TEXT
	}

	/* Main kernel text */
	.text : AT(ADDR(.text) - LOAD_OFFSET) {
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		CPUIDLE_TEXT
		KPROBES_TEXT
		IRQENTRY_TEXT
		SOFTIRQENTRY_TEXT
		*(.fixup)
		_etext = .;
	}

	/* Exception table */
	EXCEPTION_TABLE(4)

	/* Bug table */
	. = ALIGN(4);
	__bug_table : AT(ADDR(__bug_table) - LOAD_OFFSET) {
		__start___bug_table = .;
		KEEP(*(__bug_table))
		__stop___bug_table = .;
	}

	/* Read-only data - uses generic macro for all subsections */
	RO_DATA(PAGE_SIZE)

	/* Init code and data - freed after boot */
	. = ALIGN(PAGE_SIZE);
	__init_begin = .;

	INIT_TEXT_SECTION(PAGE_SIZE)

	. = ALIGN(16);
	.init.data : AT(ADDR(.init.data) - LOAD_OFFSET) {
		INIT_DATA
	}

	. = ALIGN(4);
	.init.setup : AT(ADDR(.init.setup) - LOAD_OFFSET) {
		__setup_start = .;
		KEEP(*(.init.setup))
		__setup_end = .;
	}

	.initcall.init : AT(ADDR(.initcall.init) - LOAD_OFFSET) {
		__initcall_start = .;
		INIT_CALLS
		__initcall_end = .;
	}

	.con_initcall.init : AT(ADDR(.con_initcall.init) - LOAD_OFFSET) {
		__con_initcall_start = .;
		KEEP(*(.con_initcall.init))
		__con_initcall_end = .;
	}

	. = ALIGN(PAGE_SIZE);
	__init_end = .;

	/* Read-write data */
	_sdata = .;

	. = ALIGN(PAGE_SIZE);
	.data : AT(ADDR(.data) - LOAD_OFFSET) {
		INIT_TASK_DATA(THREAD_SIZE)
		DATA_DATA
		CONSTRUCTORS
	}

	. = ALIGN(4);
	.data.cacheline_aligned : AT(ADDR(.data.cacheline_aligned) - LOAD_OFFSET) {
		*(.data.cacheline_aligned)
	}

	_edata = .;

	/* BSS - not stored in binary, zeroed by head.S */
	BSS_SECTION(4, 4, 4)

	_end = .;

	/*
	 * Exception vector table data (stored in kernel image).
	 * head.S copies this to physical $0000FFD0 during early boot.
	 * Hardware reads vectors from physical memory (bypasses MMU).
	 */
	.vectors : AT(ADDR(.vectors) - LOAD_OFFSET) {
		__vectors_start = .;
		KEEP(*(.vectors))
		__vectors_end = .;
	}

	STABS_DEBUG
	DWARF_DEBUG

	DISCARDS
}

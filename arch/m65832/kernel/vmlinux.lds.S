/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Linker script for the M65832 kernel
 */

#include <asm-generic/vmlinux.lds.h>
#include <asm/page.h>
#include <asm/thread_info.h>

OUTPUT_FORMAT("elf32-m65832")
OUTPUT_ARCH(m65832)
ENTRY(_start)

jiffies = jiffies_64;

/*
 * Kernel virtual address base
 */
#define KERNEL_LINK_ADDR	PAGE_OFFSET

SECTIONS
{
	. = KERNEL_LINK_ADDR;

	/* Head section - must be first */
	.head.text : AT(ADDR(.head.text) - KERNEL_LINK_ADDR) {
		_stext = .;
		HEAD_TEXT
	}

	/* Main kernel text */
	.text : AT(ADDR(.text) - KERNEL_LINK_ADDR) {
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

	/* Read-only data */
	RO_DATA(PAGE_SIZE)

	/* Init code and data - freed after boot */
	. = ALIGN(PAGE_SIZE);
	__init_begin = .;

	INIT_TEXT_SECTION(PAGE_SIZE)

	. = ALIGN(16);
	.init.data : AT(ADDR(.init.data) - KERNEL_LINK_ADDR) {
		INIT_DATA
	}

	. = ALIGN(4);
	.init.setup : AT(ADDR(.init.setup) - KERNEL_LINK_ADDR) {
		__setup_start = .;
		*(.init.setup)
		__setup_end = .;
	}

	.initcall.init : AT(ADDR(.initcall.init) - KERNEL_LINK_ADDR) {
		__initcall_start = .;
		INIT_CALLS
		__initcall_end = .;
	}

	.con_initcall.init : AT(ADDR(.con_initcall.init) - KERNEL_LINK_ADDR) {
		__con_initcall_start = .;
		*(.con_initcall.init)
		__con_initcall_end = .;
	}

	. = ALIGN(PAGE_SIZE);
	__init_end = .;

	/* Read-write data */
	_sdata = .;

	. = ALIGN(PAGE_SIZE);
	.data : AT(ADDR(.data) - KERNEL_LINK_ADDR) {
		DATA_DATA
		CONSTRUCTORS
	}

	. = ALIGN(4);
	.data.cacheline_aligned : AT(ADDR(.data.cacheline_aligned) - KERNEL_LINK_ADDR) {
		*(.data.cacheline_aligned)
	}

	_edata = .;

	/* BSS */
	BSS_SECTION(4, 4, 4)

	_end = .;

	/* Debug info - stripped in release builds */
	STABS_DEBUG
	DWARF_DEBUG

	/* Discard sections we don't need */
	DISCARDS
}

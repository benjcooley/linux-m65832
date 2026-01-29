/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Minimal ELF definitions for host tools on macOS
 * Based on Linux kernel's include/uapi/linux/elf.h
 */
#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>

/* ELF identification indices */
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_OSABI	7
#define EI_PAD		8
#define EI_NIDENT	16

/* ELF magic number */
#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'
#define ELFMAG		"\177ELF"
#define SELFMAG		4

/* ELF classes */
#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2
#define ELFCLASSNUM	3

/* ELF data encoding */
#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2
#define ELFDATANUM	3

/* ELF header types */
#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4

/* Machine types */
#define EM_NONE		0
#define EM_386		3
#define EM_ARM		40
#define EM_X86_64	62
#define EM_AARCH64	183
#define EM_RISCV	243
#define EM_M65832	0x6583	/* M65832 */

/* Program header types */
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_TLS		7

/* Section header types */
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11

/* Section flags */
#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4

/* Symbol binding */
#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2

/* Symbol types */
#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4

/* Symbol visibility */
#define STV_DEFAULT	0
#define STV_INTERNAL	1
#define STV_HIDDEN	2
#define STV_PROTECTED	3

/* Special section indices */
#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_XINDEX	0xffff
#define SHN_HIRESERVE	0xffff

/* Additional section types */
#define SHT_SYMTAB_SHNDX 18

/* More machine types */
#define EM_SPARC	2
#define EM_PPC		20
#define EM_PPC64	21
#define EM_PARISC	15
#define EM_SPARCV9	43

/* Symbol types - arch specific */
#define STT_LOPROC	13
#define STT_HIPROC	15
#define STT_SPARC_REGISTER 13

/* Relocation types - i386 */
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2

/* Relocation types - ARM */
#define R_ARM_NONE		0
#define R_ARM_PC24		1
#define R_ARM_ABS32		2
#define R_ARM_REL32		3
#define R_ARM_CALL		28
#define R_ARM_JUMP24		29
#define R_ARM_THM_PC22		10
#define R_ARM_THM_JUMP24	30
#define R_ARM_THM_MOVW_ABS_NC	47
#define R_ARM_THM_MOVT_ABS	48
#define R_ARM_THM_JUMP19	51
#define R_ARM_MOVW_ABS_NC	43
#define R_ARM_MOVT_ABS		44

/* Relocation types - MIPS */
#define R_MIPS_NONE		0
#define R_MIPS_16		1
#define R_MIPS_32		2
#define R_MIPS_REL32		3
#define R_MIPS_26		4
#define R_MIPS_HI16		5
#define R_MIPS_LO16		6
#define R_MIPS_GPREL16		7
#define R_MIPS_LITERAL		8
#define R_MIPS_GOT16		9
#define R_MIPS_PC16		10
#define R_MIPS_CALL16		11
#define R_MIPS_GPREL32		12
#define R_MIPS_64		18

/* Relocation types - x86_64 */
#define R_X86_64_NONE		0
#define R_X86_64_64		1
#define R_X86_64_PC32		2
#define R_X86_64_GOT32		3
#define R_X86_64_PLT32		4
#define R_X86_64_32		10
#define R_X86_64_32S		11

/* Relocation types - AArch64 */
#define R_AARCH64_NONE			0
#define R_AARCH64_ABS64			257
#define R_AARCH64_ABS32			258
#define R_AARCH64_ABS16			259
#define R_AARCH64_PREL64		260
#define R_AARCH64_PREL32		261
#define R_AARCH64_PREL16		262
#define R_AARCH64_MOVW_UABS_G0		263
#define R_AARCH64_MOVW_UABS_G0_NC	264
#define R_AARCH64_MOVW_UABS_G1		265
#define R_AARCH64_MOVW_UABS_G1_NC	266
#define R_AARCH64_MOVW_UABS_G2		267
#define R_AARCH64_MOVW_UABS_G2_NC	268
#define R_AARCH64_MOVW_UABS_G3		269
#define R_AARCH64_ADR_GOT_PAGE		311
#define R_AARCH64_LD64_GOT_LO12_NC	312
#define R_AARCH64_CALL26		283
#define R_AARCH64_JUMP26		282
#define R_AARCH64_ADR_PREL_PG_HI21	275
#define R_AARCH64_ADR_PREL_PG_HI21_NC	276
#define R_AARCH64_ADD_ABS_LO12_NC	277
#define R_AARCH64_LDST8_ABS_LO12_NC	278
#define R_AARCH64_LDST16_ABS_LO12_NC	284
#define R_AARCH64_LDST32_ABS_LO12_NC	285
#define R_AARCH64_LDST64_ABS_LO12_NC	286

/* Relocation types - PowerPC */
#define R_PPC_NONE		0
#define R_PPC_ADDR32		1
#define R_PPC_ADDR24		2
#define R_PPC_ADDR16		3
#define R_PPC_ADDR16_LO		4
#define R_PPC_ADDR16_HI		5
#define R_PPC_ADDR16_HA		6
#define R_PPC_ADDR14		7
#define R_PPC_REL24		10
#define R_PPC_REL32		26

/* Relocation types - PowerPC64 */
#define R_PPC64_NONE		0
#define R_PPC64_ADDR32		1
#define R_PPC64_ADDR24		2
#define R_PPC64_ADDR16		3
#define R_PPC64_ADDR16_LO	4
#define R_PPC64_ADDR16_HI	5
#define R_PPC64_ADDR16_HA	6
#define R_PPC64_REL24		10
#define R_PPC64_REL32		26
#define R_PPC64_ADDR64		38
#define R_PPC64_TOC16		47
#define R_PPC64_TOC16_LO	48
#define R_PPC64_TOC16_HI	49
#define R_PPC64_TOC16_HA	50
#define R_PPC64_TOC		51
#define R_PPC64_REL64		44

/* Relocation types - SPARC */
#define R_SPARC_NONE		0
#define R_SPARC_32		3
#define R_SPARC_HI22		9
#define R_SPARC_LO10		12
#define R_SPARC_WDISP30		7
#define R_SPARC_64		32

/* Relocation types - RISC-V */
#define R_RISCV_NONE		0
#define R_RISCV_32		1
#define R_RISCV_64		2
#define R_RISCV_BRANCH		16
#define R_RISCV_JAL		17
#define R_RISCV_CALL		18
#define R_RISCV_CALL_PLT	19
#define R_RISCV_PCREL_HI20	23
#define R_RISCV_PCREL_LO12_I	24
#define R_RISCV_PCREL_LO12_S	25
#define R_RISCV_HI20		26
#define R_RISCV_LO12_I		27
#define R_RISCV_LO12_S		28

/* Relocation types - LoongArch */
#define R_LARCH_NONE		0
#define R_LARCH_32		1
#define R_LARCH_64		2
#define R_LARCH_B26		66
#define R_LARCH_ABS_HI20	64
#define R_LARCH_ABS_LO12	65
#define R_LARCH_PCALA_HI20	71
#define R_LARCH_PCALA_LO12	72

/* 32-bit ELF structures */
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;

typedef struct {
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
} Elf32_Phdr;

typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

typedef struct {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	unsigned char	st_info;
	unsigned char	st_other;
	Elf32_Half	st_shndx;
} Elf32_Sym;

typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
} Elf32_Rel;

typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
	Elf32_Sword	r_addend;
} Elf32_Rela;

/* 64-bit ELF structures */
typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Half;
typedef uint64_t Elf64_Off;
typedef int32_t  Elf64_Sword;
typedef int64_t  Elf64_Sxword;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;

typedef struct {
	unsigned char	e_ident[EI_NIDENT];
	Elf64_Half	e_type;
	Elf64_Half	e_machine;
	Elf64_Word	e_version;
	Elf64_Addr	e_entry;
	Elf64_Off	e_phoff;
	Elf64_Off	e_shoff;
	Elf64_Word	e_flags;
	Elf64_Half	e_ehsize;
	Elf64_Half	e_phentsize;
	Elf64_Half	e_phnum;
	Elf64_Half	e_shentsize;
	Elf64_Half	e_shnum;
	Elf64_Half	e_shstrndx;
} Elf64_Ehdr;

typedef struct {
	Elf64_Word	p_type;
	Elf64_Word	p_flags;
	Elf64_Off	p_offset;
	Elf64_Addr	p_vaddr;
	Elf64_Addr	p_paddr;
	Elf64_Xword	p_filesz;
	Elf64_Xword	p_memsz;
	Elf64_Xword	p_align;
} Elf64_Phdr;

typedef struct {
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
} Elf64_Shdr;

typedef struct {
	Elf64_Word	st_name;
	unsigned char	st_info;
	unsigned char	st_other;
	Elf64_Half	st_shndx;
	Elf64_Addr	st_value;
	Elf64_Xword	st_size;
} Elf64_Sym;

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
} Elf64_Rel;

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
	Elf64_Sxword	r_addend;
} Elf64_Rela;

/* Macros for ELF symbol info */
#define ELF32_ST_BIND(val)	(((unsigned char)(val)) >> 4)
#define ELF32_ST_TYPE(val)	((val) & 0xf)
#define ELF32_ST_INFO(bind, type) (((bind) << 4) + ((type) & 0xf))

#define ELF64_ST_BIND(val)	ELF32_ST_BIND(val)
#define ELF64_ST_TYPE(val)	ELF32_ST_TYPE(val)
#define ELF64_ST_INFO(bind, type) ELF32_ST_INFO(bind, type)

/* Macros for relocation info */
#define ELF32_R_SYM(val)	((val) >> 8)
#define ELF32_R_TYPE(val)	((val) & 0xff)
#define ELF32_R_INFO(sym, type)	(((sym) << 8) + ((type) & 0xff))

#define ELF64_R_SYM(val)	((val) >> 32)
#define ELF64_R_TYPE(val)	((val) & 0xffffffff)
#define ELF64_R_INFO(sym, type)	(((uint64_t)(sym) << 32) + (type))

#endif /* _ELF_H */

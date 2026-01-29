/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Thread structure definitions (for assembly access).
 */

#ifndef _ASM_M65832_THREAD_STRUCT_H
#define _ASM_M65832_THREAD_STRUCT_H

/* Offsets into thread_struct for assembly code */
#define THREAD_KSP		0	/* Kernel stack pointer */
#define THREAD_R16		4	/* Saved R16 */
#define THREAD_R17		8	/* Saved R17 */
#define THREAD_R18		12	/* Saved R18 */
#define THREAD_R19		16	/* Saved R19 */
#define THREAD_R20		20	/* Saved R20 */
#define THREAD_R21		24	/* Saved R21 */
#define THREAD_R22		28	/* Saved R22 */
#define THREAD_R23		32	/* Saved R23 */
#define THREAD_R29		36	/* Frame pointer */
#define THREAD_R30		40	/* Return address */

#endif /* _ASM_M65832_THREAD_STRUCT_H */

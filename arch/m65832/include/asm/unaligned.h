/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_M65832_UNALIGNED_H
#define _ASM_M65832_UNALIGNED_H

/*
 * M65832 is little-endian and can handle unaligned accesses in hardware
 * (though they may be slower). Use the LE accessors.
 */
#include <linux/unaligned/le_struct.h>
#include <linux/unaligned/be_byteshift.h>
#include <linux/unaligned/generic.h>

#define get_unaligned	__get_unaligned_le
#define put_unaligned	__put_unaligned_le

#endif /* _ASM_M65832_UNALIGNED_H */

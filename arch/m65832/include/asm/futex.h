/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Futex support for the M65832 architecture.
 */

#ifndef _ASM_M65832_FUTEX_H
#define _ASM_M65832_FUTEX_H

#include <linux/futex.h>
#include <linux/uaccess.h>
#include <asm/errno.h>
#include <asm/cmpxchg.h>

/*
 * Futex operations using M65832 CAS instruction
 */

static inline int
arch_futex_atomic_op_inuser(int op, int oparg, int *oval, u32 __user *uaddr)
{
	int oldval = 0, ret;
	u32 tmp;

	if (!access_ok(uaddr, sizeof(u32)))
		return -EFAULT;

	switch (op) {
	case FUTEX_OP_SET:
		oldval = arch_xchg(uaddr, oparg);
		break;
	case FUTEX_OP_ADD:
		do {
			oldval = *uaddr;
			tmp = oldval + oparg;
		} while (arch_cmpxchg(uaddr, oldval, tmp) != oldval);
		break;
	case FUTEX_OP_OR:
		do {
			oldval = *uaddr;
			tmp = oldval | oparg;
		} while (arch_cmpxchg(uaddr, oldval, tmp) != oldval);
		break;
	case FUTEX_OP_ANDN:
		do {
			oldval = *uaddr;
			tmp = oldval & ~oparg;
		} while (arch_cmpxchg(uaddr, oldval, tmp) != oldval);
		break;
	case FUTEX_OP_XOR:
		do {
			oldval = *uaddr;
			tmp = oldval ^ oparg;
		} while (arch_cmpxchg(uaddr, oldval, tmp) != oldval);
		break;
	default:
		ret = -ENOSYS;
		goto out;
	}

	ret = 0;
	*oval = oldval;

out:
	return ret;
}

static inline int
futex_atomic_cmpxchg_inatomic(u32 *uval, u32 __user *uaddr,
			      u32 oldval, u32 newval)
{
	u32 val;

	if (!access_ok(uaddr, sizeof(u32)))
		return -EFAULT;

	val = arch_cmpxchg(uaddr, oldval, newval);
	*uval = val;

	return 0;
}

#endif /* _ASM_M65832_FUTEX_H */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * User-space memory access for the M65832 architecture.
 */

#ifndef _ASM_M65832_UACCESS_H
#define _ASM_M65832_UACCESS_H

#include <linux/string.h>
#include <asm/page.h>

/*
 * User space memory segment limit
 */
#define KERNEL_DS	((mm_segment_t) { 0 })
#define USER_DS		((mm_segment_t) { PAGE_OFFSET })

#define get_fs()	(current_thread_info()->addr_limit)
#define set_fs(x)	(current_thread_info()->addr_limit = (x))

#define uaccess_kernel()	(get_fs().seg == KERNEL_DS.seg)

/*
 * Check if a user pointer is valid
 */
static inline int __access_ok(unsigned long addr, unsigned long size)
{
	return (addr + size <= current_thread_info()->addr_limit.seg) &&
	       (addr + size >= addr);
}

#define access_ok(addr, size)	__access_ok((unsigned long)(addr), (size))

/*
 * Get a simple variable from user space
 */
#define get_user(x, ptr)						\
({									\
	int __gu_err = 0;						\
	__typeof__(*(ptr)) __gu_val = 0;				\
	if (access_ok(ptr, sizeof(*(ptr)))) {				\
		__gu_val = *(ptr);					\
	} else {							\
		__gu_err = -EFAULT;					\
	}								\
	(x) = __gu_val;							\
	__gu_err;							\
})

#define __get_user(x, ptr)	get_user(x, ptr)

/*
 * Put a simple variable to user space
 */
#define put_user(x, ptr)						\
({									\
	int __pu_err = 0;						\
	if (access_ok(ptr, sizeof(*(ptr)))) {				\
		*(ptr) = (x);						\
	} else {							\
		__pu_err = -EFAULT;					\
	}								\
	__pu_err;							\
})

#define __put_user(x, ptr)	put_user(x, ptr)

/*
 * Copy to/from user space
 */
static inline unsigned long
raw_copy_from_user(void *to, const void __user *from, unsigned long n)
{
	if (access_ok(from, n)) {
		memcpy(to, from, n);
		return 0;
	}
	return n;
}

static inline unsigned long
raw_copy_to_user(void __user *to, const void *from, unsigned long n)
{
	if (access_ok(to, n)) {
		memcpy(to, from, n);
		return 0;
	}
	return n;
}

#define INLINE_COPY_FROM_USER
#define INLINE_COPY_TO_USER

/*
 * Clear user space memory
 */
static inline unsigned long
__clear_user(void __user *to, unsigned long n)
{
	if (access_ok(to, n)) {
		memset(to, 0, n);
		return 0;
	}
	return n;
}

#define clear_user(to, n)	__clear_user(to, n)

/*
 * String length in user space
 */
static inline long
strnlen_user(const char __user *str, long count)
{
	const char *end;
	long len;

	if (!access_ok(str, 1))
		return 0;

	end = memchr(str, 0, count);
	if (end)
		len = end - str + 1;
	else
		len = count;

	return len;
}

/*
 * Copy string from user space
 */
static inline long
strncpy_from_user(char *dst, const char __user *src, long count)
{
	long res = -EFAULT;

	if (access_ok(src, 1)) {
		const char *end = memchr(src, 0, count);
		long len = end ? (end - src) : count;
		memcpy(dst, src, len);
		if (len < count)
			dst[len] = '\0';
		res = len;
	}
	return res;
}

#endif /* _ASM_M65832_UACCESS_H */

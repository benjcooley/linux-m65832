/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * User-space memory access for the M65832 architecture.
 *
 * Modern kernels removed set_fs/get_fs. User access is checked by
 * verifying addresses are below TASK_SIZE.
 */

#ifndef _ASM_M65832_UACCESS_H
#define _ASM_M65832_UACCESS_H

#include <linux/string.h>
#include <linux/thread_info.h>
#include <linux/types.h>
#include <asm/extable.h>
#include <asm/page.h>

/*
 * User space is below PAGE_OFFSET (TASK_SIZE)
 */
#define user_addr_max()	TASK_SIZE

/*
 * Check if a user pointer is valid
 */
static inline int __access_ok(const void __user *addr, unsigned long size)
{
	unsigned long a = (unsigned long)addr;
	/* Check for overflow and that it's in user space */
	return (a + size >= a) && (a + size <= TASK_SIZE);
}

#define access_ok(addr, size)	__access_ok((addr), (size))

/*
 * Get a simple variable from user space
 * For M65832, we do a direct copy with access check
 */
/*
 * Use a char[] buffer to avoid const/type-punning issues.
 * The buffer is copied from user space, then into the destination
 * variable via memcpy, which handles all type combinations safely.
 */
#define get_user(x, ptr)						\
({									\
	int __gu_err = -EFAULT;						\
	if (access_ok((ptr), sizeof(*(ptr)))) {				\
		char __gu_buf[sizeof(*(ptr))];				\
		memcpy(__gu_buf, (const void __user *)(ptr),		\
		       sizeof(*(ptr)));					\
		memcpy(&(x), __gu_buf, sizeof(*(ptr)));			\
		__gu_err = 0;						\
	}								\
	__gu_err;							\
})

#define __get_user(x, ptr)	get_user(x, ptr)

/*
 * Put a simple variable to user space
 */
#define put_user(x, ptr)						\
({									\
	int __pu_err = -EFAULT;						\
	__typeof__(*(ptr)) __user *__pu_ptr = (ptr);			\
	if (access_ok(__pu_ptr, sizeof(*__pu_ptr))) {			\
		*__pu_ptr = (x);					\
		__pu_err = 0;						\
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
 * strncpy_from_user and strnlen_user are provided by
 * generic lib/strncpy_from_user.c and lib/strnlen_user.c
 * (CONFIG_GENERIC_STRNCPY_FROM_USER, CONFIG_GENERIC_STRNLEN_USER)
 */
extern long strncpy_from_user(char *dst, const char __user *src, long count);
extern long strnlen_user(const char __user *str, long count);

#endif /* _ASM_M65832_UACCESS_H */

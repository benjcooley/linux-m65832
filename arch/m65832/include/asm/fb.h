/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_M65832_FB_H
#define _ASM_M65832_FB_H

#include <linux/io.h>
#include <linux/fb.h>
#include <linux/fs.h>

static inline void fb_pgprotect(struct file *file, struct vm_area_struct *vma,
				unsigned long off)
{
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
}

#define fb_is_primary_device(fb)	(0)

#endif /* _ASM_M65832_FB_H */

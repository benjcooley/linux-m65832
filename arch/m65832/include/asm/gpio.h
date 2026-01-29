/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_M65832_GPIO_H
#define _ASM_M65832_GPIO_H

/* M65832 GPIO - use gpiolib */
#define ARCH_NR_GPIOS 0

#ifndef __ASSEMBLY__

#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep
#define gpio_to_irq	__gpio_to_irq

#include <asm-generic/gpio.h>

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_GPIO_H */

# SPDX-License-Identifier: GPL-2.0-only
obj-y += div64.o gcd.o lcm.o int_log.o int_pow.o int_sqrt.o reciprocal_div.o

ifeq ($(CONFIG_M65832),y)
# Bring-up workaround: avoid control-flow corruption in reciprocal/div helpers.
CFLAGS_div64.o += -O0
CFLAGS_reciprocal_div.o += -O0
endif

obj-$(CONFIG_CORDIC)		+= cordic.o
obj-$(CONFIG_PRIME_NUMBERS)	+= prime_numbers.o
obj-$(CONFIG_RATIONAL)		+= rational.o

obj-$(CONFIG_TEST_DIV64)	+= test_div64.o
obj-$(CONFIG_TEST_MULDIV64)	+= test_mul_u64_u64_div_u64.o

obj-y += tests/

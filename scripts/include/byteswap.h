/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Minimal byteswap.h for macOS host tools
 */
#ifndef _BYTESWAP_H
#define _BYTESWAP_H

#include <libkern/OSByteOrder.h>

#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#endif /* _BYTESWAP_H */

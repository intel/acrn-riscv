/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */
#include <types.h>

void *memset(void *base, uint8_t v, size_t n)
{
	void *p = base;

	for (size_t i = 0; i < n; i++) {
		*(uint8_t *)p++ = v;
	}

	return base;
}

void *memset_s(void *base, uint8_t v, size_t n)
{
	if ((base != NULL) && (n != 0U)) {
		memset(base, v, n);
        }

	return base;
}

void memcpy(void *d, const void *s, size_t slen)
{
	for (size_t i = 0; i < slen; i++) {
		*(uint8_t *)d++ = *(uint8_t *)s++;
	}
}

int32_t memcpy_s(void *d, size_t dmax, const void *s, size_t slen)
{
	int32_t ret = -1;

	if ((d != NULL) && (s != NULL) && (dmax >= slen) && ((d > (s + slen)) || (s > (d + dmax)))) {
		if (slen != 0U) {
			memcpy(d, s, slen);
		}
		ret = 0;
	} else {
		(void)memset(d, 0U, dmax);
	}

	return ret;
}

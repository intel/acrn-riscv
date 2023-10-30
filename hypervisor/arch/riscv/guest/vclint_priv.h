/*-
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 * Copyright (c) 2023 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NETAPP, INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NETAPP, INC OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef VCLINT_PRIV_H
#define VCLINT_PRIV_H

/*
 * CLINT Register:		Offset	Description
 */
#define CLINT_OFFSET_MSIP0	0x00U	/* msip for hart 0 */
#define CLINT_OFFSET_MSIP1	0x04U	/* msip for hart 1 */
#define CLINT_OFFSET_MSIP2	0x08U	/* msip for hart 2 */
#define CLINT_OFFSET_MSIP3	0x0CU	/* msip for hart 3 */
#define CLINT_OFFSET_MSIP4	0x10U	/* msip for hart 4 */
#define CLINT_OFFSET_TIMER0	0x4000U	/* mtimecmp for hart 0 */
#define CLINT_OFFSET_TIMER1	0x4008U	/* mtimecmp for hart 1 */
#define CLINT_OFFSET_TIMER2	0x4010U	/* mtimecmp for hart 2 */
#define CLINT_OFFSET_TIMER3	0x4018U	/* mtimecmp for hart 3 */
#define CLINT_OFFSET_TIMER4	0x4020U	/* mtimecmp for hart 4 */
#define CLINT_OFFSET_MTIME	0xBFF8U	/* mtime */

#endif /* VCLINT_PRIV_H */

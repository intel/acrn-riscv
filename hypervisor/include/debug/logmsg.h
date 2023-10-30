/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#ifndef LOGMSG_H
#define LOGMSG_H
#ifdef CONFIG_X86
#include <cpu.h>
#include <assert.h>
#endif
#include <printf.h>

/* Logging severity levels */
#define LOG_FATAL		1U
/* For msg should be write to console and sbuf meanwhile but not fatal error */
#define LOG_ACRN		2U
#define LOG_ERROR		3U
#define LOG_WARNING		4U
#define LOG_INFO		5U
#define LOG_DEBUG		6U

/* Logging flags */
#define LOG_FLAG_STDOUT		0x00000001U
#define LOG_FLAG_MEMORY		0x00000002U
#define LOG_FLAG_NPK		0x00000004U
#define LOG_ENTRY_SIZE	80U
/* Size of buffer used to store a message being logged,
 * should align to LOG_ENTRY_SIZE.
 */
#define LOG_MESSAGE_MAX_SIZE	(4U * LOG_ENTRY_SIZE)

#define DBG_LEVEL_LAPICPT	5U
#if defined(HV_DEBUG)

extern uint16_t console_loglevel;
extern uint16_t mem_loglevel;
extern uint16_t npk_loglevel;

#endif /* HV_DEBUG */

void init_logmsg(uint32_t flags);
void do_logmsg(uint32_t severity, const char *fmt, ...);

/** The well known vprintf() function.
 *
 *  Formats a string and writes it to the console output.
 *
 *  @param fmt A pointer to the NUL terminated format string.
 *  @param args The variable long argument list as va_list.
 *  @return The number of characters actually written or a negative
 *          number if an error occurred.
 */

void vprintf(const char *fmt, va_list args);

#ifndef pr_prefix
#define pr_prefix
#endif

#define pr_fatal(...)						\
	do {							\
		do_logmsg(LOG_FATAL, pr_prefix __VA_ARGS__);	\
	} while (0)

#define pr_acrnlog(...)						\
	do {							\
		do_logmsg(LOG_ACRN, pr_prefix __VA_ARGS__);	\
	} while (0)

#define pr_err(...)						\
	do {							\
		do_logmsg(LOG_ERROR, pr_prefix __VA_ARGS__);	\
	} while (0)

#define pr_warn(...)						\
	do {							\
		do_logmsg(LOG_WARNING, pr_prefix __VA_ARGS__);	\
	} while (0)

#define pr_info(...)						\
	do {							\
		do_logmsg(LOG_INFO, pr_prefix __VA_ARGS__);	\
	} while (0)

#define pr_dbg(...)						\
	do {							\
		do_logmsg(LOG_DEBUG, pr_prefix __VA_ARGS__);	\
	} while (0)

#define dev_dbg(lvl, ...)					\
	do {							\
		do_logmsg((lvl), pr_prefix __VA_ARGS__);	\
	} while (0)

#ifndef CONFIG_ARM
#define panic(...) 							\
	do { pr_fatal("PANIC: %s line: %d\n", __func__, __LINE__);	\
		pr_fatal(__VA_ARGS__); 					\
		while (1) { asm_pause(); }; } while (0)
#endif
#endif /* LOGMSG_H */

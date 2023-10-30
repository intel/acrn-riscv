/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#if defined(CONFIG_ARM) || defined(CONFIG_RISCV)

size_t console_write(const char *s, size_t len);
void console_putc(const char *ch);
char console_getc(void);

#else
#include <vuart.h>

/** Writes a given number of characters to the console.
 *
 *  @param s A pointer to character array to write.
 *  @param len The number of characters to write.
 *
 *  @return The number of characters written or -1 if an error occurred
 *          and no character was written.
 */
size_t console_write(const char *s, size_t len);

/** Writes a single character to the console.
 *
 *  @param ch The character to write.
 *
 *  @preturn The number of characters written or -1 if an error
 *           occurred before any character was written.
 */
void console_putc(const char *ch);
char console_getc(void);
void suspend_console(void);
void resume_console(void);
struct acrn_vuart *vm_console_vuart(struct acrn_vm *vm);

#endif
/** Initializes the console module.
 *
 */
void console_init(void);
void console_setup_timer(void);


#endif /* CONSOLE_H */

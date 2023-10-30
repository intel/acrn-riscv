/******************************************************************************
 * config.h
 * 
 * A Linux-style configuration list.
 */

#ifndef __ACRN_CONFIG_H__
#define __ACRN_CONFIG_H__

#include <asm/defconfig.h>
#include <asm/board.h>

#ifndef __ASSEMBLY__
#include <acrn/compiler.h>

#if defined(CONFIG_ENFORCE_UNIQUE_SYMBOLS) || defined(__clang__)
# define EMIT_FILE asm ( "" )
#else
# define EMIT_FILE asm ( ".file \"" __FILE__ "\"" )
#endif

#endif

#include <asm/config.h>

#define EXPORT_SYMBOL(var)

/*
 * The following log levels are as follows:
 *
 *   ACRNLOG_ERR: Fatal errors, either acrn, Guest or Dom0
 *               is about to crash.
 *
 *   ACRNLOG_WARNING: Something bad happened, but we can recover.
 *
 *   ACRNLOG_INFO: Interesting stuff, but not too noisy.
 *
 *   ACRNLOG_DEBUG: Use where ever you like. Lots of noise.
 *
 *
 * Since we don't trust the guest operating system, we don't want
 * it to allow for DoS by causing the HV to print out a lot of
 * info, so where ever the guest has control of what is printed
 * we use the ACRNLOG_GUEST to distinguish that the output is
 * controlled by the guest.
 *
 * To make it easier on the typing, the above log levels all
 * have a corresponding _G_ equivalent that appends the
 * ACRNLOG_GUEST. (see the defines below).
 *
 */
#define ACRNLOG_ERR     "<0>"
#define ACRNLOG_WARNING "<1>"
#define ACRNLOG_INFO    "<2>"
#define ACRNLOG_DEBUG   "<3>"

#define ACRNLOG_GUEST   "<G>"

#define ACRNLOG_G_ERR     ACRNLOG_GUEST ACRNLOG_ERR
#define ACRNLOG_G_WARNING ACRNLOG_GUEST ACRNLOG_WARNING
#define ACRNLOG_G_INFO    ACRNLOG_GUEST ACRNLOG_INFO
#define ACRNLOG_G_DEBUG   ACRNLOG_GUEST ACRNLOG_DEBUG

/*
 * Some code is copied directly from Linux.
 * Match some of the Linux log levels to acrn.
 */
#define KERN_ERR       ACRNLOG_ERR
#define KERN_CRIT      ACRNLOG_ERR
#define KERN_EMERG     ACRNLOG_ERR
#define KERN_WARNING   ACRNLOG_WARNING
#define KERN_NOTICE    ACRNLOG_INFO
#define KERN_INFO      ACRNLOG_INFO
#define KERN_DEBUG     ACRNLOG_DEBUG

/* Linux 'checker' project. */
#define __iomem
#define __user
#define __force
#define __bitwise

#define KB(_kb)     (_AC(_kb, ULL) << 10)
#define MB(_mb)     (_AC(_mb, ULL) << 20)
#define GB(_gb)     (_AC(_gb, ULL) << 30)

#define IS_ALIGNED(val, align) (((val) & ((align) - 1)) == 0)

#define __STR(...) #__VA_ARGS__
#define STR(...) __STR(__VA_ARGS__)

/* allow existing code to work with Kconfig variable */
#define NR_CPUS CONFIG_NR_CPUS

#ifndef CONFIG_DEBUG
#define NDEBUG
#endif

#ifndef ZERO_BLOCK_PTR
/* Return value for zero-size allocation, distinguished from NULL. */
#define ZERO_BLOCK_PTR ((void *)-1L)
#endif

#endif /* __ACRN_CONFIG_H__ */

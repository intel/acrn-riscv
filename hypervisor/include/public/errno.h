/*
 * There are two expected ways of including this header.
 *
 * 1) The "default" case (expected from tools etc).
 *
 * Simply #include <public/errno.h>
 *
 * In this circumstance, normal header guards apply and the includer shall get
 * an enumeration in the ACRN_xxx namespace, appropriate for C or assembly.
 *
 * 2) The special case where the includer provides a ACRN_ERRNO() in scope.
 *
 * In this case, no inclusion guards apply and the caller is responsible for
 * their ACRN_ERRNO() being appropriate in the included context.  The header
 * will unilaterally #undef ACRN_ERRNO().
 */

#ifndef ACRN_ERRNO

/*
 * Includer has not provided a custom ACRN_ERRNO().  Arrange for normal header
 * guards, an automatic enum (for C code) and constants in the ACRN_xxx
 * namespace.
 */
#ifndef __ACRN_PUBLIC_ERRNO_H__
#define __ACRN_PUBLIC_ERRNO_H__

#define ACRN_ERRNO_DEFAULT_INCLUDE

#ifndef __ASSEMBLY__

#define ACRN_ERRNO(name, value) ACRN_##name = value,
enum acrn_errno {

#elif __ACRN_INTERFACE_VERSION__ < 0x00040700

#define ACRN_ERRNO(name, value) .equ ACRN_##name, value

#endif /* __ASSEMBLY__ */

#endif /* __ACRN_PUBLIC_ERRNO_H__ */
#endif /* !ACRN_ERRNO */

/* ` enum neg_errnoval {  [ -Efoo for each Efoo in the list below ]  } */
/* ` enum errnoval { */

#ifdef ACRN_ERRNO

/*
 * Values originating from x86 Linux. Please consider using respective
 * values when adding new definitions here.
 *
 * The set of identifiers to be added here shouldn't extend beyond what
 * POSIX mandates (see e.g.
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html)
 * with the exception that we support some optional (XSR) values
 * specified there (but no new ones should be added).
 */

ACRN_ERRNO(EPERM,	 1)	/* Operation not permitted */
ACRN_ERRNO(ENOENT,	 2)	/* No such file or directory */
ACRN_ERRNO(ESRCH,	 3)	/* No such process */
#ifdef __ACRN__ /* Internal only, should never be exposed to the guest. */
ACRN_ERRNO(EINTR,	 4)	/* Interrupted system call */
#endif
ACRN_ERRNO(EIO,		 5)	/* I/O error */
ACRN_ERRNO(ENXIO,	 6)	/* No such device or address */
ACRN_ERRNO(E2BIG,	 7)	/* Arg list too long */
ACRN_ERRNO(ENOEXEC,	 8)	/* Exec format error */
ACRN_ERRNO(EBADF,	 9)	/* Bad file number */
ACRN_ERRNO(ECHILD,	10)	/* No child processes */
ACRN_ERRNO(EAGAIN,	11)	/* Try again */
ACRN_ERRNO(EWOULDBLOCK,	11)	/* Operation would block.  Aliases EAGAIN */
ACRN_ERRNO(ENOMEM,	12)	/* Out of memory */
ACRN_ERRNO(EACCES,	13)	/* Permission denied */
ACRN_ERRNO(EFAULT,	14)	/* Bad address */
ACRN_ERRNO(EBUSY,	16)	/* Device or resource busy */
ACRN_ERRNO(EEXIST,	17)	/* File exists */
ACRN_ERRNO(EXDEV,	18)	/* Cross-device link */
ACRN_ERRNO(ENODEV,	19)	/* No such device */
ACRN_ERRNO(EISDIR,	21)	/* Is a directory */
ACRN_ERRNO(EINVAL,	22)	/* Invalid argument */
ACRN_ERRNO(ENFILE,	23)	/* File table overflow */
ACRN_ERRNO(EMFILE,	24)	/* Too many open files */
ACRN_ERRNO(ENOSPC,	28)	/* No space left on device */
ACRN_ERRNO(EROFS,	30)	/* Read-only file system */
ACRN_ERRNO(EMLINK,	31)	/* Too many links */
ACRN_ERRNO(EDOM,		33)	/* Math argument out of domain of func */
ACRN_ERRNO(ERANGE,	34)	/* Math result not representable */
ACRN_ERRNO(EDEADLK,	35)	/* Resource deadlock would occur */
ACRN_ERRNO(EDEADLOCK,	35)	/* Resource deadlock would occur. Aliases EDEADLK */
ACRN_ERRNO(ENAMETOOLONG,	36)	/* File name too long */
ACRN_ERRNO(ENOLCK,	37)	/* No record locks available */
ACRN_ERRNO(ENOSYS,	38)	/* Function not implemented */
ACRN_ERRNO(ENOTEMPTY,	39)	/* Directory not empty */
ACRN_ERRNO(ENODATA,	61)	/* No data available */
ACRN_ERRNO(ETIME,	62)	/* Timer expired */
ACRN_ERRNO(EBADMSG,	74)	/* Not a data message */
ACRN_ERRNO(EOVERFLOW,	75)	/* Value too large for defined data type */
ACRN_ERRNO(EILSEQ,	84)	/* Illegal byte sequence */
#ifdef __ACRN__ /* Internal only, should never be exposed to the guest. */
ACRN_ERRNO(ERESTART,	85)	/* Interrupted system call should be restarted */
#endif
ACRN_ERRNO(ENOTSOCK,	88)	/* Socket operation on non-socket */
ACRN_ERRNO(EMSGSIZE,	90)	/* Message too large. */
ACRN_ERRNO(EOPNOTSUPP,	95)	/* Operation not supported on transport endpoint */
ACRN_ERRNO(EADDRINUSE,	98)	/* Address already in use */
ACRN_ERRNO(EADDRNOTAVAIL, 99)	/* Cannot assign requested address */
ACRN_ERRNO(ENOBUFS,	105)	/* No buffer space available */
ACRN_ERRNO(EISCONN,	106)	/* Transport endpoint is already connected */
ACRN_ERRNO(ENOTCONN,	107)	/* Transport endpoint is not connected */
ACRN_ERRNO(ETIMEDOUT,	110)	/* Connection timed out */
ACRN_ERRNO(ECONNREFUSED,	111)	/* Connection refused */

#undef ACRN_ERRNO
#endif /* ACRN_ERRNO */
/* ` } */

/* Clean up from a default include.  Close the enum (for C). */
#ifdef ACRN_ERRNO_DEFAULT_INCLUDE
#undef ACRN_ERRNO_DEFAULT_INCLUDE
#ifndef __ASSEMBLY__
};
#endif

#endif /* ACRN_ERRNO_DEFAULT_INCLUDE */

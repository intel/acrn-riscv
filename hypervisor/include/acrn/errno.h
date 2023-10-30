#ifndef __ACRN_ERRNO_H__
#define __ACRN_ERRNO_H__

#ifndef __ASSEMBLY__

#define ACRN_ERRNO(name, value) name = value,
enum {
#include <public/errno.h>
};

#else /* !__ASSEMBLY__ */

#define ACRN_ERRNO(name, value) .equ name, value
#include <public/errno.h>

#endif /* __ASSEMBLY__ */

#endif /*  __ACRN_ERRNO_H__ */

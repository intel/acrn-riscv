#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */

#include <acrn/types.h>

extern char _start[], _end[], start[], _boot[], _vkernel[];
#define is_kernel(p) ({                         \
    char *__p = (char *)(unsigned long)(p);     \
    (__p >= _start) && (__p < _end);            \
})

extern char _stext[], _etext[];
#define is_kernel_text(p) ({                    \
    char *__p = (char *)(unsigned long)(p);     \
    (__p >= _stext) && (__p < _etext);          \
})

extern const char _srodata[], _erodata[];
#define is_kernel_rodata(p) ({                  \
    const char *__p = (const char *)(unsigned long)(p);     \
    (__p >= _srodata) && (__p < _erodata);      \
})

extern char _sinittext[], _einittext[];
#define is_kernel_inittext(p) ({                \
    char *__p = (char *)(unsigned long)(p);     \
    (__p >= _sinittext) && (__p < _einittext);  \
})

#define min_t(type,x,y) \
        ({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#define max_t(type,x,y) \
        ({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })



#endif /* _LINUX_KERNEL_H */


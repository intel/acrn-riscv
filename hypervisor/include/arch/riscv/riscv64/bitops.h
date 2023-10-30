#ifndef __RISCV_RISCV64_BITOPS_H__
#define __RISCV_RISCV64_BITOPS_H__

static always_inline unsigned long __ffs(unsigned long x)
{
//	asm volatile ("ctz %0, %1" : "=r"(x) : "r"(x));

        return x;
}

#define ffz(x)  __ffs(~(x))

static inline int flsl(unsigned long x)
{
        asm volatile ("clz %0, %1" : "=r"(x) : "r"(x));

        return BITS_PER_LONG - 1 - x;
}

#ifndef find_next_bit
extern unsigned long find_next_bit(const unsigned long *addr, unsigned long
		size, unsigned long offset);
#endif

#ifndef find_next_zero_bit
extern unsigned long find_next_zero_bit(const unsigned long *addr, unsigned
		long size, unsigned long offset);
#endif

#define find_first_bit(addr, size) find_next_bit((addr), (size), 0)
#define find_first_zero_bit(addr, size) find_next_zero_bit((addr), (size), 0)

#endif /* _RISCV_RISCV64_BITOPS_H */

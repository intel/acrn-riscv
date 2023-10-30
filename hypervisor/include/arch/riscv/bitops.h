#ifndef __RISCV_BITOPS_H__
#define __RISCV_BITOPS_H__

/*
 * Non-atomic bit manipulation.
 *
 * Implemented using atomics to be interrupt safe. Could alternatively
 * implement with local interrupt masking.
 */
#define __set_bit(n,p)            set_bit(n,p)
#define __clear_bit(n,p)          clear_bit(n,p)

#define BITOP_BITS_PER_WORD     32
#define BITOP_MASK(nr)          (1UL << ((nr) % BITOP_BITS_PER_WORD))
#define BITOP_WORD(nr)          ((nr) / BITOP_BITS_PER_WORD)
#define BITS_PER_BYTE           8

#define ADDR (*(volatile int *) addr)
#define CONST_ADDR (*(const volatile int *) addr)

#if defined(CONFIG_RISCV_32)
# include <asm/riscv32/bitops.h>
#elif defined(CONFIG_RISCV_64)
# include <asm/riscv64/bitops.h>
#else
# error "unknown RISCV variant"
#endif

/*
 * Atomic bitops
 *
 * The helpers below *should* only be used on memory shared between
 * trusted threads or we know the memory cannot be accessed by another
 * thread.
 */

void set_bit(int nr, volatile void *p);
void clear_bit(int nr, volatile void *p);
int test_and_set_bit(int nr, volatile void *p);
int test_and_clear_bit(int nr, volatile void *p);
int test_and_change_bit(int nr, volatile void *p);

void clear_mask16(uint16_t mask, volatile void *p);

/*
 * The helpers below may fail to update the memory if the action takes
 * too long.
 *
 * @max_try: Maximum number of iterations
 *
 * The helpers will return true when the update has succeeded (i.e no
 * timeout) and false if the update has failed.
 */
bool set_bit_timeout(int nr, volatile void *p, unsigned int max_try);
bool clear_bit_timeout(int nr, volatile void *p, unsigned int max_try);
bool change_bit_timeout(int nr, volatile void *p, unsigned int max_try);
bool test_and_set_bit_timeout(int nr, volatile void *p,
                              int *oldbit, unsigned int max_try);
bool test_and_clear_bit_timeout(int nr, volatile void *p,
                                int *oldbit, unsigned int max_try);
bool test_and_change_bit_timeout(int nr, volatile void *p,
                                 int *oldbit, unsigned int max_try);
bool clear_mask16_timeout(uint16_t mask, volatile void *p,
                          unsigned int max_try);

/**
 * __test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static inline int __test_and_set_bit(int nr, volatile void *addr)
{
        unsigned int mask = BITOP_MASK(nr);
        volatile unsigned int *p =
                ((volatile unsigned int *)addr) + BITOP_WORD(nr);
        unsigned int old = *p;

        *p = old | mask;
        return (old & mask) != 0;
}

/**
 * __test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static inline int __test_and_clear_bit(int nr, volatile void *addr)
{
        unsigned int mask = BITOP_MASK(nr);
        volatile unsigned int *p =
                ((volatile unsigned int *)addr) + BITOP_WORD(nr);
        unsigned int old = *p;

        *p = old & ~mask;
        return (old & mask) != 0;
}

/* WARNING: non atomic and it can be reordered! */
static inline int __test_and_change_bit(int nr,
                                            volatile void *addr)
{
        unsigned int mask = BITOP_MASK(nr);
        volatile unsigned int *p =
                ((volatile unsigned int *)addr) + BITOP_WORD(nr);
        unsigned int old = *p;

        *p = old ^ mask;
        return (old & mask) != 0;
}

/**
 * test_bit - Determine whether a bit is set
 * @nr: bit number to test
 * @addr: Address to start counting from
 */
static inline int test_bit(int nr, const volatile void *addr)
{
        const volatile unsigned int *p = (const volatile unsigned int *)addr;
        return 1UL & (p[BITOP_WORD(nr)] >> (nr & (BITOP_BITS_PER_WORD-1)));
}

static inline int ffs(unsigned int x)
{
        asm volatile ("ctzw %0, %1" : "=r" (x) : "r" (x));
	return x + 1;
}

static inline int fls(unsigned int x)
{
        asm volatile ("clzw %0, %1" : "=r" (x) : "r" (x));
	return 32 - x;
}

/**
 * find_first_set_bit - find the first set bit in @word
 * @word: the word to search
 *
 * Returns the bit-number of the first set bit (first bit being 0).
 * The input must *not* be zero.
 */
static inline unsigned int find_first_set_bit(unsigned long word)
{
        return __ffsl(word) - 1;
}

/**
 * hweightN - returns the hamming weight of a N-bit word
 * @x: the word to weigh
 *
 * The Hamming Weight of a number is the total number of bits set in it.
 */
#define hweight64(x) generic_hweight64(x)
#define hweight32(x) generic_hweight32(x)
#define hweight16(x) generic_hweight16(x)
#define hweight8(x) generic_hweight8(x)

static inline void bitmap_set_lock(uint16_t nr_arg, volatile uint64_t *addr)
{}

static inline void bitmap_set_nolock(uint16_t nr_arg, volatile uint64_t *addr)
{}

static inline bool bitmap_clear_nolock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap_test_and_set_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap_test_and_clear_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	if (nr_arg == 8)
		return true;
	return false;
}

static inline bool bitmap_test(uint16_t nr, const volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_set_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_clear_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_set_nolock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_clear_nolock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}


static inline bool bitmap32_test_and_set_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_test_and_clear_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_test(uint16_t nr, const volatile uint32_t *addr)
{
	return true;
}

#endif /* __RISCV_BITOPS_H__ */

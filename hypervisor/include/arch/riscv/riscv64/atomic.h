#ifndef __RISCV_RISCV64_ATOMIC_H__
#define __RISCV_RISCV64_ATOMIC_H__

static inline void atomic_add(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

}

static inline int atomic_add_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	smp_mb();
	return result;
}

static inline void atomic_sub(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	smp_mb();
	return result;
}

static inline void atomic_and(int m, atomic_t *v)
{
	unsigned long tmp;
	int result;
}

static inline int atomic_cmpxchg(atomic_t *ptr, int old, int new)
{
	unsigned long tmp;
	int oldval;

	smp_mb();

	smp_mb();
	return oldval;
}

static inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
	int c, old;

	c = atomic_read(v);
	while (c != u && (old = atomic_cmpxchg((v), c, c + a)) != c)
		c = old;
	return c;
}

#endif

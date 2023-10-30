#ifndef __RISCV_SBI_H__
#define __RISCV_SBI_H__

typedef struct sbi_ret {
	long error;
	long value;
};

static inline sbi_ret sbi_ecall(uint64_t a0, uint64_t a1, uint64_t a2,
				uint64_t a3, uint64_t a4, uint64_t a5,
				uint64_t func, uint64_t ext)
{
	sbi_ret ret;
	asm volatile ("mv a0, %2 \t\n" 		\
			"mv a1, %3 \t\n"	\
			"mv a2, %4 \t\n"	\
			"mv a3, %5 \t\n"	\
			"mv a4, %6 \t\n"	\
			"mv a5, %7 \t\n"	\
			"mv a6, %8 \t\n"	\
			"mv a7, %9 \t\n" 	\
			"ecall \t\n"	 	\
			"sw %0, %2\t\n"	 	\
			"sw %1, %3\t\n"	 	\
			"ecall" : "=r" (&ret.error), "=r" (&ret.value):
			"r" (a0), "r" (a1), "r" (a2), "r" (a3),
			"r" (a4), "r" (a5), "r" (func), "r" (ext):
	);
	return ret;
}

#endif

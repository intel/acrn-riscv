#include "uart.h"

extern void user(void);
extern void app(void);
extern char stack[];

void kernel(void)
{
	char *msg = "hello kernel\n";
//	void *p = user - 0x40080000;
//	void *p = user - 0x40000000;
	void *p = app - 0x40000000;
//	void *s = stack - 0x40000000;
//	void *p = user;

	printk(msg);
/*
	asm volatile(
		"li t0, 0xC0000\n\t" \
		"csrw sstatus, t0\n\t" \
		"csrw sepc, %0\n\t" \
		"sret":: "r"(p)
	);
*/

//	asm volatile("move sp, %0\n\t"::"r" (s));
	asm volatile("li t0, 0xC0000\n\t"::);
	asm volatile("csrw sstatus, t0\n\t"::);
	asm volatile("csrw sepc, %0\n\t"::"r" (p));
	asm volatile("sret"::);
}

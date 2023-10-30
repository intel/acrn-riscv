#include "uart.h"
#include "app.h"


static void idle()
{
//	printk("this is guest\n");
	asm volatile ("ecall"::);
}

void __hc_userfunc guest()
{
	/* check if it's in U mode */
	//asm volatile ("csrr t0, sstatus"::);

	while (1) {
		idle();
	}
}

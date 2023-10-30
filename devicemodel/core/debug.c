#include <stdio.h>
#include <stdlib.h>

inline long acrn_hypercall1(unsigned long hcall_id, unsigned long param1)
{

	/* x86-64 System V ABI register usage */
	register signed long    result asm("rax");
	register unsigned long  r8 asm("r8")  = hcall_id;

	/* Execute vmcall */
	asm volatile(".byte 0x0F,0x01,0xC1\n"
			: "=r"(result)
			: "D"(param1), "r"(r8));

	/* Return result to caller */
	return result;
}

void new_vm_run(unsigned long vmid)
{
	acrn_hypercall1(0x80UL<<24|0x12UL, vmid);
}

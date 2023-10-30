#include <asm/sbi.h>

void sbi_set_timer(uint64_t deadline, uint32_t cpu)
{
	writeq_relaxed(deadline, CLINT_MTIMECMP(cpu));
}

uint64_t sbi_get_time()
{
	return readq_relaxed(CLINT_MTIME);
}

uint64_t sbi_send_ipi()
{
	return readq_relaxed(CLINT_MTIME);
}

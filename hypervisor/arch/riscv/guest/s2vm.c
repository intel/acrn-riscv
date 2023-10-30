#include <acrn/init.h>
#include <acrn/types.h>
#include <acrn/kernel.h>
#include <acrn/pfn.h>
#include <acrn/sizes.h>
#include <acrn/mm.h>
#include <asm/riscv64/flushtlb.h>
#include <asm/pgtable.h>
#include <asm/setup.h>
#include <debug/logmsg.h>
#include <asm/guest/s2vm.h>
#include <acrn/cpumask.h>
#include <asm/notify.h>
#include <asm/smp.h>
#include <asm/guest/vm.h>

unsigned int s2vm_inital_level;

void *get_s2pt_entry(struct acrn_vm *vm)
{
	void *s2ptp = vm->arch_vm.s2ptp;

	return s2ptp;
}

uint64_t local_gpa2hpa(struct acrn_vm *vm, uint64_t gpa, uint32_t *size)
{
	uint64_t hpa = INVALID_HPA;
	void *s2ptp;
	const uint64_t *pgentry;
	uint64_t pg_size = 0UL;

	s2ptp = get_s2pt_entry(vm);

	pgentry = lookup_address((uint64_t *)s2ptp, gpa, &pg_size, &vm->arch_vm.s2pt_mem_ops);

	if (pgentry != NULL) {
		hpa = ((*pgentry & (~S2PT_PFN_HIGH_MASK)) & (~(pg_size - 1UL))) | (gpa & (pg_size - 1UL));
	}

	if ((size != NULL) && (hpa != INVALID_HPA)) {
		*size = (uint32_t)pg_size;
	}

	return hpa;
}

uint64_t gpa2hpa(struct acrn_vm *vm, uint64_t gpa)
{
	return local_gpa2hpa(vm, gpa, NULL);
}

#define SATP_MODE_SV48 0x9000000000000000U
static uint64_t generate_satp(uint16_t vmid, uint64_t addr)
{
	return SATP_MODE_SV48 | ((uint64_t)vmid << 44U) | addr;
}

void setup_virt_paging(void)
{
	s2vm_inital_level = 0;
}

static void s2pt_flush_guest(struct acrn_vm *vm)
{
	unsigned long flags = 0;
	uint64_t osatp;
	uint64_t s2pt_satp = vm->arch_vm.s2pt_satp;

	osatp = cpu_csr_read(hgatp);
	if (osatp != s2pt_satp)
	{
		local_irq_save(flags);

		//cpu_csr_write(hgatp, s2pt_satp);
		isb();
	}

	flush_guest_tlb();

	if (osatp != cpu_csr_read(hgatp))
	{
		//cpu_csr_write(hgatp, s2pt_satp);
		isb();
		local_irq_restore(flags);
	}
}

static int s2pt_setup_satp(struct acrn_vm *vm)
{
	uint64_t satp;

	switch (s2vm_inital_level) {
		case 0:
			satp = hva2hpa(vm->arch_vm.s2pt_mem_ops.info->s2pt.vpn3_base);
			break;
		default:
			pr_fatal("Not support inital level !!");
	}

	vm->arch_vm.s2pt_satp = generate_satp(vm->vm_id, satp);
	spin_lock(&vm->s2pt_lock);
	s2pt_flush_guest(vm);
	spin_unlock(&vm->s2pt_lock);

	return 0;
}

int s2pt_init(struct acrn_vm *vm)
{
	int rc = 0;

	spin_lock_init(&vm->s2pt_lock);

	s2pt_setup_satp(vm);

	return rc;
}

void s2pt_add_mr(struct acrn_vm *vm, uint64_t *pml4_page,
	uint64_t hpa, uint64_t gpa, uint64_t size, uint64_t prot_orig)
{
	uint64_t prot = prot_orig;

	spin_lock(&vm->s2pt_lock);
	mmu_add(pml4_page, hpa, gpa, size, prot, &vm->arch_vm.s2pt_mem_ops);
	spin_unlock(&vm->s2pt_lock);

	s2pt_flush_guest(vm);
}

void s2pt_modify_mr(struct acrn_vm *vm, uint64_t *pml4_page,
		uint64_t gpa, uint64_t size,
		uint64_t prot_set, uint64_t prot_clr)
{
	uint64_t local_prot = prot_set;

	spin_lock(&vm->s2pt_lock);
	mmu_modify_or_del(pml4_page, gpa, size, local_prot, prot_clr, &(vm->arch_vm.s2pt_mem_ops), MR_MODIFY);
	spin_unlock(&vm->s2pt_lock);
	s2pt_flush_guest(vm);
}

void s2pt_del_mr(struct acrn_vm *vm, uint64_t *pml4_page, uint64_t gpa, uint64_t size)
{

	spin_lock(&vm->s2pt_lock);
	mmu_modify_or_del(pml4_page, gpa, size, 0UL, 0UL, &vm->arch_vm.s2pt_mem_ops, MR_DEL);
	spin_unlock(&vm->s2pt_lock);
	s2pt_flush_guest(vm);
}

/**
 * @pre vm != NULL && cb != NULL.
 */
void walk_s2pt_table(struct acrn_vm *vm, pge_handler cb)
{
	const struct memory_ops *mem_ops = &vm->arch_vm.s2pt_mem_ops;
	uint64_t *vpn3, *vpn2, *vpn1, *pte;
	uint64_t i, j, k, m;

	for (i = 0UL; i < PTRS_PER_VPN3; i++) {
		vpn3 = vpn3_offset((uint64_t *)get_s2pt_entry(vm), i << VPN3_SHIFT);
		if (mem_ops->pgentry_present(*vpn3) == 0UL) {
			continue;
		}
		for (j = 0UL; j < PTRS_PER_VPN2; j++) {
			vpn2 = vpn2_offset(vpn3, j << VPN2_SHIFT);
			if (mem_ops->pgentry_present(*vpn2) == 0UL) {
				continue;
			}
			if (vpn2_large(*vpn2) != 0UL) {
				cb(vpn2, VPN2_SIZE);
				continue;
			}
			for (k = 0UL; k < PTRS_PER_VPN1; k++) {
				vpn1 = vpn1_offset(vpn2, k << VPN1_SHIFT);
				if (mem_ops->pgentry_present(*vpn1) == 0UL) {
					continue;
				}
				if (vpn1_large(*vpn1) != 0UL) {
					cb(vpn1, VPN1_SIZE);
					continue;
				}
				for (m = 0UL; m < PTRS_PER_PTE; m++) {
					pte = pte_offset(vpn1, m << PTE_SHIFT);
					if (mem_ops->pgentry_present(*pte) != 0UL) {
						cb(pte, PTE_SIZE);
					}
				}
			}
		}
	}
}

void s2vm_restore_state(struct acrn_vcpu *vcpu)
{
	uint8_t *last_vcpu_ran;
	struct acrn_vm *vm = vcpu->vm;
	uint64_t satp = vm->arch_vm.s2pt_satp;

	//if ( is_idle_vcpu(n) )
	  //  return;
}

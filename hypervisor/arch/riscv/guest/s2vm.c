/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <asm/init.h>
#include <asm/mem.h>
#include <asm/tlb.h>
#include <asm/pgtable.h>
#include <asm/setup.h>
#include <debug/logmsg.h>
#include <asm/guest/s2vm.h>
#include <asm/notify.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
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

	/**
	 * If specified parameter size is not NULL and
	 * the HPA of parameter gpa is found, pg_size shall
	 * be returned through parameter size.
	 */
	if ((size != NULL) && (hpa != INVALID_HPA)) {
		*size = (uint32_t)pg_size;
	}

	return hpa;
}

/* using return value INVALID_HPA as error code */
uint64_t gpa2hpa(struct acrn_vm *vm, uint64_t gpa)
{
	return local_gpa2hpa(vm, gpa, NULL);
}

static inline uint64_t generate_satp(uint16_t vmid, uint64_t addr)
{
	return SATP_MODE_SV48 | ((uint64_t)vmid << 44U) | (addr >> 12);
}

void setup_virt_paging(void)
{
	s2vm_inital_level = 0;
}

/*
 * Force a synchronous TLB flush.
 */
void s2pt_flush_guest(struct acrn_vm *vm)
{
	unsigned long flags = 0;
	uint64_t osatp;
	uint64_t s2pt_satp = vm->arch_vm.s2pt_satp;

	osatp = cpu_csr_read(hgatp);
	if (osatp != s2pt_satp)
	{
		local_irq_save(&flags);

		cpu_csr_write(hgatp, s2pt_satp);

		/* Ensure hsatp is synchronized before flushing the TLBs */
		isb();
	}

	flush_guest_tlb_local();
	if (smp_ops)
		smp_ops->hfence(cpu_online_map, 0, 0);

	if (osatp != s2pt_satp)
	{
		cpu_csr_write(hgatp, s2pt_satp);
		/* Ensure hsatp is back in place before continuing. */
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
			return -1;
	}
	vm->arch_vm.s2pt_satp = generate_satp(vm->vm_id, satp);
	/*
	 * Make sure that all TLBs corresponding to the new VMID are flushed
	 * before using it
	 */
	spin_lock(&vm->s2pt_lock);
	s2pt_flush_guest(vm);
	spin_unlock(&vm->s2pt_lock);

	return 0;
}

int s2pt_init(struct acrn_vm *vm)
{
	int rc = 0;

	spinlock_init(&vm->s2pt_lock);

	s2pt_setup_satp(vm);

	return rc;
}

void s2pt_add_mr(struct acrn_vm *vm, uint64_t *vpn3_page,
	uint64_t hpa, uint64_t gpa, uint64_t size, uint64_t prot_orig)
{
	uint64_t prot = prot_orig;

	pr_info("%s, vm[%d] hpa: 0x%016lx gpa: 0x%016lx size: 0x%016lx prot: 0x%016x\n",
			__func__, vm->vm_id, hpa, gpa, size, prot);

	spin_lock(&vm->s2pt_lock);
	mmu_add(vpn3_page, hpa, gpa, size, prot, &vm->arch_vm.s2pt_mem_ops);
	spin_unlock(&vm->s2pt_lock);

	s2pt_flush_guest(vm);
}

void s2pt_modify_mr(struct acrn_vm *vm, uint64_t *vpn3_page,
		uint64_t gpa, uint64_t size,
		uint64_t prot_set, uint64_t prot_clr)
{
	uint64_t local_prot = prot_set;

	pr_dbg("%s,vm[%d] gpa 0x%lx size 0x%lx\n", __func__, vm->vm_id, gpa, size);

	spin_lock(&vm->s2pt_lock);

	mmu_modify_or_del(vpn3_page, gpa, size, local_prot, prot_clr, &(vm->arch_vm.s2pt_mem_ops), MR_MODIFY);

	spin_unlock(&vm->s2pt_lock);

	s2pt_flush_guest(vm);
}
/**
 * @pre [gpa,gpa+size) has been mapped into host physical memory region
 */
void s2pt_del_mr(struct acrn_vm *vm, uint64_t *vpn3_page, uint64_t gpa, uint64_t size)
{
	pr_info("%s,vm[%d] gpa 0x%lx size 0x%lx\n", __func__, vm->vm_id, gpa, size);

	spin_lock(&vm->s2pt_lock);

	mmu_modify_or_del(vpn3_page, gpa, size, 0UL, 0UL, &vm->arch_vm.s2pt_mem_ops, MR_DEL);

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
			if (vpn_large(*vpn2) != 0UL) {
				cb(vpn2, VPN2_SIZE);
				continue;
			}
			for (k = 0UL; k < PTRS_PER_VPN1; k++) {
				vpn1 = vpn1_offset(vpn2, k << VPN1_SHIFT);
				if (mem_ops->pgentry_present(*vpn1) == 0UL) {
					continue;
				}
				if (vpn_large(*vpn1) != 0UL) {
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
			/*
			 * Walk through the whole page tables of one VM is a time-consuming
			 * operation. Preemption is not support by hypervisor scheduling
			 * currently, so the walk through page tables operation might occupy
			 * CPU for long time what starve other threads.
			 *
			 * Give chance to release CPU to make other threads happy.
			 */

			/*FIXME when schedule is ready
			if (need_reschedule(get_pcpu_id())) {
				schedule();
			}
			*/
		}
	}
}

void s2vm_restore_state(struct acrn_vcpu *vcpu)
{
#if 0
	uint8_t *last_vcpu_ran;
	struct acrn_vm *vm = vcpu->vm;
	uint64_t satp = vm->arch_vm.s2pt_satp;

	//if ( is_idle_vcpu(n) )
	  //  return;
	WRITE_SYSREG(vcpu->arch.sctlr, SCTLR_EL1);
	WRITE_SYSREG(vcpu->arch.hcr_el2, HCR_EL2);

	/*
	 * ARM64_WORKAROUND_AT_SPECULATE: VTTBR_EL2 should be restored after all
	 * registers associated to EL1/EL0 translations regime have been
	 * synchronized.
	 */
	asm volatile("isb");
	WRITE_SYSREG64(satp, VTTBR_EL2);

	//last_vcpu_ran = &p2m->last_vcpu_ran[smp_processor_id()];

	///*
	// * While we are restoring an out-of-context translation regime
	// * we still need to ensure:
	// *  - VTTBR_EL2 is synchronized before flushing the TLBs
	// *  - All registers for EL1 are synchronized before executing an AT
	// *  instructions targeting S1/S2.
	// */
	isb();

	flush_guest_tlb_local();
	///*
	// * Flush local TLB for the domain to prevent wrong TLB translation
	// * when running multiple vCPU of the same domain on a single pCPU.
	// */
	//if ( *last_vcpu_ran != INVALID_VCPU_ID && *last_vcpu_ran != n->vcpu_id )
	//	flush_guest_tlb_local();

	//*last_vcpu_ran = n->vcpu_id;
#endif
}

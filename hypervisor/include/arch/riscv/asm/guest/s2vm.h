/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_S2VM_H__
#define __RISCV_S2VM_H__

#include <asm/lib/spinlock.h>
#include <asm/current.h>
#include <asm/pgtable.h>

#define S2PT_PFN_HIGH_MASK      0xFFFF000000000000UL

struct acrn_vm;
struct acrn_vcpu;

typedef void (*pge_handler)(uint64_t *pgentry, uint64_t size);
#ifndef CONFIG_MACRN
extern void setup_virt_paging(void);
extern uint64_t local_gpa2hpa(struct acrn_vm *vm, uint64_t gpa, uint32_t *size);
extern uint64_t gpa2hpa(struct acrn_vm *vm, uint64_t gpa);
extern int s2pt_init(struct acrn_vm *vm);
extern void s2pt_add_mr(struct acrn_vm *vm, uint64_t *pml4_page, uint64_t hpa,
			uint64_t gpa, uint64_t size, uint64_t prot_orig);
extern void s2pt_del_mr(struct acrn_vm *vm, uint64_t *pml4_page, uint64_t gpa, uint64_t size);
extern void s2pt_modify_mr(struct acrn_vm *vm, uint64_t *vpn3_page, uint64_t gpa,
				uint64_t size, uint64_t prot_set, uint64_t prot_clr);
extern void s2vm_restore_state(struct acrn_vcpu *vcpu);
extern void s2pt_flush_guest(struct acrn_vm *vm);
#else
static inline void setup_virt_paging(void) {}
static inline uint64_t local_gpa2hpa(struct acrn_vm *vm, uint64_t gpa, uint32_t *size)
{
	return gpa;
}
static inline uint64_t gpa2hpa(struct acrn_vm *vm, uint64_t gpa)
{
	return gpa;
}

static inline int s2pt_init(struct acrn_vm *vm)
{
	return 0;
}
static inline void s2pt_add_mr(struct acrn_vm *vm, uint64_t *pml4_page, uint64_t hpa,
			uint64_t gpa, uint64_t size, uint64_t prot_orig) {}
static inline void s2pt_del_mr(struct acrn_vm *vm, uint64_t *pml4_page, uint64_t gpa, uint64_t size) {}
static inline void s2pt_modify_mr(struct acrn_vm *vm, uint64_t *vpn3_page, uint64_t gpa,
				uint64_t size, uint64_t prot_set, uint64_t prot_clr) {}
static inline void s2vm_restore_state(struct acrn_vcpu *vcpu) {}
static inline void s2pt_flush_guest(struct acrn_vm *vm) {}
#endif

#endif /* __RISCV_S2VM_H__ */

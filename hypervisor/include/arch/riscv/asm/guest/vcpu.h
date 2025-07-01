/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_VCPU_H__
#define __RISCV_VCPU_H__

#ifndef __ASSEMBLY__

#include <acrn_common.h>
#include <schedule.h>
#include <event.h>
#include <io_req.h>
#include <asm/cpu.h>
#include <asm/vmx.h>
#include <asm/guest/guest_memory.h>
#include <asm/guest/vclint.h>

#define ACRN_REQUEST_EXCP			0U
#define ACRN_REQUEST_EVENT			1U
#define ACRN_REQUEST_EXTINT			2U
#define ACRN_REQUEST_NMI			3U
#define ACRN_REQUEST_EOI_EXIT_BITMAP_UPDATE	4U
#define ACRN_REQUEST_EPT_FLUSH			5U
#define ACRN_REQUEST_TRP_FAULT			6U
#define ACRN_REQUEST_VPID_FLUSH			7U
#define ACRN_REQUEST_INIT_VMCS			8U
#define ACRN_REQUEST_WAIT_WBINVD		9U

#define foreach_vcpu(idx, vm, t_vcpu)				\
	for ((idx) = 0U, (t_vcpu) = &((vm)->hw.vcpu[(idx)]);	\
		(idx) < (vm)->hw.created_vcpus;			\
		(idx)++, (t_vcpu) = &((vm)->hw.vcpu[(idx)]))	\
		if (t_vcpu->state != VCPU_OFFLINE)

enum vcpu_state {
	VCPU_OFFLINE = 0U,
	VCPU_INIT,
	VCPU_RUNNING,
	VCPU_ZOMBIE,
};

enum vm_cpu_mode {
	CPU_MODE_64BIT,
};

#define	VCPU_EVENT_IOREQ		0
#define	VCPU_EVENT_VIRTUAL_INTERRUPT	1
#define	VCPU_EVENT_SYNC_WBINVD		2
#define	VCPU_EVENT_NUM			3

enum reset_mode;

/* 2 worlds: 0 for Normal World, 1 for Secure World */
#define NR_WORLD	2
#define NORMAL_WORLD	0
#define SECURE_WORLD	1

#define NUM_WORLD_CSRS		2U
#define NUM_COMMON_CSRS		17U
#define NUM_GUEST_CSRS		(NUM_WORLD_CSRS + NUM_COMMON_CSRS)

#define EOI_EXIT_BITMAP_SIZE	256U

struct guest_cpu_context {
	struct run_context run_ctx;
	struct ext_context ext_ctx;

	/* per world CSRs, need isolation between secure and normal world */
	uint32_t world_csrs[NUM_WORLD_CSRS];
};

struct csr_store_entry {
	uint32_t csr_index;
	uint32_t reserved;
	uint64_t value;
} __aligned(8);

enum {
	CSR_AREA_TSC_AUX = 0,
	CSR_AREA_COUNT,
};

struct csr_store_area {
	struct csr_store_entry guest[CSR_AREA_COUNT];
	struct csr_store_entry host[CSR_AREA_COUNT];
	uint32_t count;	/* actual count of entries to be loaded/restored during VMEntry/VMExit */
};

struct acrn_vcpu_arch {
	struct guest_cpu_context contexts[NR_WORLD];
	struct cpu_info cpu_info;

	/* CSR bitmap region for this vcpu, MUST be 4-Kbyte aligned */
	uint8_t csr_bitmap[PAGE_SIZE];
	int32_t cur_context;

	/* common CSRs, world_csrs[] is a subset of it */
	uint64_t guest_csrs[NUM_GUEST_CSRS];

	uint16_t vpid;

	/* Holds the information needed for IRQ/exception handling. */
	struct {
		/* The number of the exception to raise. */
		uint32_t exception;

		/* The error number for the exception. */
		uint32_t error;
	} exception_info;

	uint8_t lapic_mask;
	uint32_t nrexits;

	/* VCPU context state information */
	uint64_t exit_reason;
	uint64_t exit_qualification;
	uint32_t inst_len;

	/* Information related to secondary / AP VCPU start-up */
	enum vm_cpu_mode cpu_mode;
	uint8_t nr_sipi;

	/* interrupt injection information */
	uint64_t pending_req;

	struct csr_store_area csr_area;

	/* EOI_EXIT_BITMAP buffer, for the bitmap update */
	uint64_t eoi_exit_bitmap[EOI_EXIT_BITMAP_SIZE >> 6U];
} __aligned(8);

struct sbi_mpxy_shm {
	uint64_t *base;
	uint64_t size;
};

struct acrn_vcpu {
	uint64_t host_sp;
	/* Architecture specific definitions for this VCPU */
	struct acrn_vcpu_arch arch;
	uint8_t stack[STACK_SIZE] __aligned(8);
	uint16_t vcpu_id;	/* virtual identifier for VCPU */
	uint16_t pcpu_id;
	struct acrn_vm *vm;		/* Reference to the VM this VCPU belongs to */

	volatile enum vcpu_state state;	/* State of this VCPU */

	struct thread_object thread_obj;
	bool launched; /* Whether the vcpu is launched on target pcpu */

	//struct instr_emul_ctxt inst_ctxt;
	struct io_request req; /* used by io/ept emulation */
	struct sbi_mpxy_shm mpxy; /* used by tee communication */

	uint64_t reg_cached;
	uint64_t reg_updated;

	struct sched_event events[VCPU_EVENT_NUM];
} __aligned(8);

extern struct acrn_vcpu idle_vcpu[NR_CPUS];

struct vcpu_dump {
	struct acrn_vcpu *vcpu;
	char *str;
	uint32_t str_max;
};

struct guest_mem_dump {
	struct acrn_vcpu *vcpu;
	uint64_t gva;
	uint64_t len;
};

static inline bool is_vcpu_bsp(const struct acrn_vcpu *vcpu)
{
	return (vcpu->vcpu_id == BSP_CPU_ID);
}

static inline enum vm_cpu_mode get_vcpu_mode(const struct acrn_vcpu *vcpu)
{
	return vcpu->arch.cpu_mode;
}

/* do not update Guest RIP for next VM Enter */
static inline void vcpu_retain_ip(struct acrn_vcpu *vcpu)
{
	(vcpu)->arch.inst_len = 0U;
}

extern struct acrn_vplic *vcpu_vplic(struct acrn_vcpu *vcpu);
extern struct acrn_vclint *vcpu_vclint(const struct acrn_vcpu *vcpu);
extern uint16_t pcpuid_from_vcpu(const struct acrn_vcpu *vcpu);
extern void default_idle(__unused struct thread_object *obj);
extern void vcpu_thread(struct thread_object *obj);
extern int32_t vmx_vmrun(const struct acrn_vcpu *vcpu);
extern uint64_t vcpu_get_gpreg(const struct acrn_vcpu *vcpu, uint32_t reg);
extern void vcpu_set_gpreg(struct acrn_vcpu *vcpu, uint32_t reg, uint64_t val);
extern uint64_t vcpu_get_rip(struct acrn_vcpu *vcpu);
extern void vcpu_set_rip(struct acrn_vcpu *vcpu, uint64_t val);
extern uint64_t vcpu_get_sp(const struct acrn_vcpu *vcpu);
extern void vcpu_set_sp(struct acrn_vcpu *vcpu, uint64_t val);
extern uint64_t vcpu_get_status(struct acrn_vcpu *vcpu);
extern void vcpu_set_status(struct acrn_vcpu *vcpu, uint64_t val);
extern uint64_t vcpu_get_guest_csr(const struct acrn_vcpu *vcpu, uint32_t csr);
extern void vcpu_set_guest_csr(struct acrn_vcpu *vcpu, uint32_t csr, uint64_t val);
extern void vcpu_set_vmcs_eoi_exit(const struct acrn_vcpu *vcpu);
extern void vcpu_reset_eoi_exit_bitmaps(struct acrn_vcpu *vcpu);
extern void vcpu_set_eoi_exit_bitmap(struct acrn_vcpu *vcpu, uint32_t vector);
extern void vcpu_clear_eoi_exit_bitmap(struct acrn_vcpu *vcpu, uint32_t vector);
extern void set_vcpu_regs(struct acrn_vcpu *vcpu, struct cpu_regs *vcpu_regs);
extern void reset_vcpu_regs(struct acrn_vcpu *vcpu);
extern void init_vcpu_protect_mode_regs(struct acrn_vcpu *vcpu, uint64_t vgdt_base_gpa);
extern void set_vcpu_startup_entry(struct acrn_vcpu *vcpu, uint64_t entry);

static inline bool is_long_mode(struct acrn_vcpu *vcpu)
{
	return true;
}

static inline bool is_paging_enabled(struct acrn_vcpu *vcpu)
{
	return true;
}

static inline bool is_pae(struct acrn_vcpu *vcpu)
{
	return false;
}

extern struct acrn_vcpu *get_running_vcpu(uint16_t pcpu_id);
extern struct acrn_vcpu *get_ever_run_vcpu(uint16_t pcpu_id);
extern int create_vcpu(struct acrn_vm *vm, uint16_t vcpu_id);
extern int32_t run_vcpu(struct acrn_vcpu *vcpu);
extern void offline_vcpu(struct acrn_vcpu *vcpu);
extern void reset_vcpu(struct acrn_vcpu *vcpu, enum reset_mode mode);
extern void zombie_vcpu(struct acrn_vcpu *vcpu, enum vcpu_state new_state);
extern void launch_vcpu(struct acrn_vcpu *vcpu);
extern void kick_vcpu(struct acrn_vcpu *vcpu);
extern int32_t prepare_vcpu(struct acrn_vm *vm, uint16_t pcpu_id);
extern uint64_t vcpumask2pcpumask(struct acrn_vm *vm, uint64_t vdmask);
extern bool is_lapic_pt_enabled(struct acrn_vcpu *vcpu);
extern void vcpu_set_state(struct acrn_vcpu *vcpu, enum vcpu_state new_state);

#endif /* __ASSEMBLY__ */

#endif /* __RISCV_VCPU_H__ */

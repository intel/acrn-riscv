/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_VM_H__
#define __RISCV_VM_H__

#ifndef __ASSEMBLY__

#include <asm/lib/bits.h>
#include <asm/lib/spinlock.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/vclint.h>
#include <asm/guest/vplic.h>
#include <vpic.h>
#include <errno.h>
#include <asm/guest/vio.h>
#include <asm/guest/vuart.h>
#include <vpci.h>
#include <asm/vm_config.h>

enum reset_mode {
	POWER_ON_RESET,		/* reset by hardware Power-on */
	COLD_RESET,		/* hardware cold reset */
	WARM_RESET,		/* behavior slightly differ from cold reset, that some MSRs might be retained. */
	INIT_RESET,		/* reset by INIT */
	SOFTWARE_RESET,		/* reset by software disable<->enable */
};

struct vm_hw_info {
	/* vcpu array of this VM */
	struct acrn_vcpu vcpu[MAX_VCPUS_PER_VM];
	uint16_t created_vcpus;	/* Number of created vcpus */
	uint64_t cpu_affinity;	/* Actual pCPUs this VM runs on. The set bits represent the pCPU IDs */
} __aligned(PAGE_SIZE);

struct sw_module_info {
	/* sw modules like ramdisk, bootargs, firmware, etc. */
	void *src_addr;			/* HVA */
	void *load_addr;		/* GPA */
	uint32_t size;
};

struct sw_kernel_info {
	void *kernel_src_addr;		/* HVA */
	void *kernel_load_addr;		/* GPA */
	void *kernel_entry_addr;	/* GPA */
	uint32_t kernel_size;
};

struct kernel_info {
	/* kernel entry point */
	paddr_t entry;
	paddr_t mem_start_gpa;
	paddr_t mem_size_gpa;
	paddr_t kernel_addr;
	paddr_t kernel_len;
	paddr_t text_offset; /* 64-bit Image only */
};

struct dtb_info {
	paddr_t dtb_start_gpa;
	paddr_t dtb_size_gpa;
	paddr_t dtb_addr;
	paddr_t dtb_len;
};

struct vm_sw_info {
	enum os_kernel_type kernel_type;	/* Guest kernel type */
	/* Kernel information (common for all guest types) */
	struct kernel_info kernel_info;
	struct dtb_info dtb_info;
	struct sw_module_info bootargs_info;
	struct sw_module_info ramdisk_info;
	/* HVA to IO shared page */
	void *io_shared_page;
	void *asyncio_sbuf;
	/* If enable IO completion polling mode */
	bool is_polling_ioreq;
};

struct vm_pm_info {
	uint8_t			px_cnt;		/* count of all Px states */
	struct acrn_pstate_data	px_data[MAX_PSTATE];
	uint8_t			cx_cnt;		/* count of all Cx entries */
	struct acrn_cstate_data	cx_data[MAX_CSTATE];
	struct pm_s_state_data	*sx_state_data;	/* data for S3/S5 implementation */
};

/* Enumerated type for VM states */
enum vm_state {
	VM_POWERED_OFF = 0,
	VM_CREATED,	/* VM created / awaiting start (boot) */
	VM_RUNNING,	/* VM running */
	VM_READY_TO_POWEROFF,     /* RTVM only, it is trying to poweroff by itself */
	VM_PAUSED,	/* VM paused */
};

enum vm_vlapic_mode {
	VM_VLAPIC_DISABLED = 0U,
	VM_VLAPIC_XAPIC,
	VM_VLAPIC_X2APIC,
	VM_VLAPIC_TRANSITION
};

struct vm_arch {
	/* I/O bitmaps A and B for this VM, MUST be 4-Kbyte aligned */
	uint8_t io_bitmap[PAGE_SIZE*2];

	void *s2ptp;
	void *sworld_s2ptp;
	uint64_t s2pt_satp;
	struct memory_ops s2pt_mem_ops;

	struct acrn_vpic vpic;      /* Virtual PIC */
	enum vm_vlapic_mode vlapic_mode; /* Represents vLAPIC mode across vCPUs*/

	/* reference to virtual platform to come here (as needed) */
} __aligned(PAGE_SIZE);

struct acrn_vm {
	/* vm_state_lock MUST be the first field in VM structure it will not clear zero when creating VM
	 * this lock initialization depends on the clear BSS section
	 */
	spinlock_t vm_state_lock;/* Spin-lock used to protect vm/vcpu state transition for a VM */
	struct vm_arch arch_vm; /* Reference to this VM's arch information */
	struct vm_hw_info hw;	/* Reference to this VM's HW information */
	struct vm_sw_info sw;	/* Reference to SW associated with this VM */
	struct vm_pm_info pm;	/* Reference to this VM's arch information */
	uint16_t vm_id;		    /* Virtual machine identifier */
	enum vm_state state;	/* VM state */

	/* per vm clint */
	struct acrn_vclint vclint;
	struct acrn_vplic vplic;
	struct acrn_vuart vuart[MAX_VUART_NUM_PER_VM];		/* Virtual UART */
	struct asyncio_desc	aio_desc[ACRN_ASYNCIO_MAX];
	struct list_head aiodesc_queue;
	enum vpic_wire_mode wire_mode;
	struct iommu_domain *iommu;	/* iommu domain of this VM */
	spinlock_t asyncio_lock; /* Spin-lock used to protect asyncio add/remove for a VM */
	spinlock_t vlapic_mode_lock;	/* Spin-lock used to protect vlapic_mode modifications for a VM */
	spinlock_t s2pt_lock;	/* Spin-lock used to protect ept add/modify/remove for a VM */
	spinlock_t emul_mmio_lock;	/* Used to protect emulation mmio_node concurrent access for a VM */
	uint16_t nr_emul_mmio_regions;	/* max index of the emulated mmio_region */
	struct mem_io_node emul_mmio[CONFIG_MAX_EMULATED_MMIO_REGIONS];

	struct vm_io_handler_desc emul_pio[EMUL_PIO_IDX_MAX];

	uint8_t uuid[16];
//	struct secure_world_control sworld_control;

	/* Secure World's snapshot
	 * Currently, Secure World is only running on vcpu[0],
	 * so the snapshot only stores the vcpu0's run_context
	 * of secure world.
	 */
	struct guest_cpu_context sworld_snapshot;

//	uint32_t vcpuid_entry_nr, vcpuid_level, vcpuid_xlevel;
//	struct vcpuid_entry vcpuid_entries[MAX_VM_VCPUID_ENTRIES];
	struct acrn_vpci vpci;
	uint8_t vrtc_offset;

	uint64_t intr_inject_delay_delta; /* delay of intr injection */
} __aligned(PAGE_SIZE);

static inline uint64_t vm_active_cpus(const struct acrn_vm *vm)
{
	uint64_t dmask = 0UL;
	uint16_t i;
	const struct acrn_vcpu *vcpu;

	foreach_vcpu(i, vm, vcpu) {
		bitmap_set_nolock(vcpu->vcpu_id, &dmask);
	}

	return dmask;
}

static inline struct acrn_vcpu *vcpu_from_vid(struct acrn_vm *vm, uint16_t vcpu_id)
{
	return &(vm->hw.vcpu[vcpu_id]);
}

static inline struct acrn_vcpu *vcpu_from_pid(struct acrn_vm *vm, uint16_t pcpu_id)
{
	uint16_t i;
	struct acrn_vcpu *vcpu, *target_vcpu = NULL;

	foreach_vcpu(i, vm, vcpu) {
		if (pcpuid_from_vcpu(vcpu) == pcpu_id) {
			target_vcpu = vcpu;
			break;
		}
	}

	return target_vcpu;
}

/* Convert relative vm id to absolute vm id */
static inline uint16_t rel_vmid_2_vmid(uint16_t sos_vmid, uint16_t rel_vmid) {
	return (sos_vmid + rel_vmid);
}

/* Convert absolute vm id to relative vm id */
static inline uint16_t vmid_2_rel_vmid(uint16_t sos_vmid, uint16_t vmid) {
	return (vmid - sos_vmid);
}

static inline bool need_shutdown_vm(uint16_t pcpu_id)
{
	return false;
}

extern void make_shutdown_vm_request(uint16_t pcpu_id);
extern int32_t shutdown_vm(struct acrn_vm *vm);
extern void pause_vm(struct acrn_vm *vm);
extern void resume_vm_from_s3(struct acrn_vm *vm, uint32_t wakeup_vec);
extern void start_vm(struct acrn_vm *vm);
extern void start_sos_vm(void);
extern int32_t reset_vm(struct acrn_vm *vm);
extern int32_t create_vm(struct acrn_vm *vm);
extern void prepare_vm(uint16_t vm_id, struct acrn_vm_config *vm_config);
extern void launch_vms(uint16_t pcpu_id);
extern bool is_poweroff_vm(const struct acrn_vm *vm);
extern bool is_created_vm(const struct acrn_vm *vm);
extern bool is_paused_vm(const struct acrn_vm *vm);
extern bool is_service_vm(const struct acrn_vm *vm);
extern bool is_postlaunched_vm(const struct acrn_vm *vm);
extern bool is_valid_postlaunched_vmid(uint16_t vm_id);
extern bool is_prelaunched_vm(const struct acrn_vm *vm);
extern uint16_t get_vmid_by_uuid(const uint8_t *uuid);
extern struct acrn_vm *get_vm_from_vmid(uint16_t vm_id);

extern struct acrn_vm *sos_vm;
extern struct acrn_vm *uos_vm;
static inline struct acrn_vm *get_sos_vm(void)
{
	return sos_vm; 
}

extern void create_sos_vm_e820(struct acrn_vm *vm);
extern void create_prelaunched_vm_e820(struct acrn_vm *vm);
extern int32_t direct_boot_sw_loader(struct acrn_vm *vm);

typedef int32_t (*vm_sw_loader_t)(struct acrn_vm *vm);
extern vm_sw_loader_t vm_sw_loader;

extern void vrtc_init(struct acrn_vm *vm);
extern bool is_lapic_pt_configured(const struct acrn_vm *vm);
extern bool is_rt_vm(const struct acrn_vm *vm);
extern bool is_pi_capable(const struct acrn_vm *vm);
extern bool has_rt_vm(void);
extern struct acrn_vm *get_highest_severity_vm(bool runtime);
extern bool vm_hide_mtrr(const struct acrn_vm *vm);
uint64_t read_vmtrr(const struct acrn_vcpu *vcpu, uint32_t msr);
void write_vmtrr(struct acrn_vcpu *vcpu, uint32_t csr, uint64_t value);
extern void update_vm_vlapic_state(struct acrn_vm *vm);
extern enum vm_vlapic_mode check_vm_vlapic_mode(const struct acrn_vm *vm);
extern void get_vm_lock(struct acrn_vm *vm);
extern void put_vm_lock(struct acrn_vm *vm);
extern void update_vm_vclint_state(struct acrn_vm *vm);
#endif /* __ASSEMBLY__ */

#endif /* __RISCV_VM_H__ */

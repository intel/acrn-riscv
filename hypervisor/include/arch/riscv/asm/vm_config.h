/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_VM_CONFIG_H__
#define __RISCV_VM_CONFIG_H__

#include <types.h>
#include <pci.h>
#include <acrn_common.h>
#include <acrn_hv_defs.h>
#include <schedule.h>
#include <asm/guest/vclint.h>

#define PRE_VM_NUM		1
#define SOS_VM_NUM		1
#define MAX_POST_VM_NUM		4
#define CONFIG_MAX_VM_NUM	(PRE_VM_NUM + SOS_VM_NUM + MAX_POST_VM_NUM)

#define MAX_PCPU_NUM		8U
#define AFFINITY_CPU(n)		(1UL << (n))
#define MAX_VCPUS_PER_VM	VCLINT_LVT_MAX
#define MAX_VUART_NUM_PER_VM	1U
#define MAX_VM_OS_NAME_LEN	32U
#define MAX_MOD_TAG_LEN		32U

#ifdef CONFIG_SCHED_NOOP
#define SOS_IDLE		""
#else
#define SOS_IDLE		"idle=halt "
#endif

#define PCI_DEV_TYPE_PTDEV	(1U << 0U)
#define PCI_DEV_TYPE_HVEMUL	(1U << 1U)
#define PCI_DEV_TYPE_SOSEMUL	(1U << 2U)

#define MAX_MMIO_DEV_NUM	2U

#define CONFIG_SOS_VM		.load_order = SOS_VM,	\
				.uuid = SOS_VM_UUID,	\
				.severity = SEVERITY_SOS,	\
				.pci_dev_num = 0U,	\
				.pci_devs = sos_pci_devs

#define CONFIG_SAFETY_VM(idx)	.load_order = PRE_LAUNCHED_VM,	\
				.uuid = SAFETY_VM_UUID##idx,	\
				.severity = SEVERITY_SAFETY_VM

#define CONFIG_PRE_STD_VM(idx)	.load_order = PRE_LAUNCHED_VM,	\
				.uuid = PRE_STANDARD_VM_UUID##idx,	\
				.severity = SEVERITY_STANDARD_VM

#define CONFIG_PRE_RT_VM(idx)	.load_order = PRE_LAUNCHED_VM,	\
				.uuid = PRE_RTVM_UUID##idx,	\
				.severity = SEVERITY_RTVM

#define CONFIG_POST_STD_VM(idx)	.load_order = POST_LAUNCHED_VM,	\
				.uuid = POST_STANDARD_VM_UUID##idx,	\
				.severity = SEVERITY_STANDARD_VM

#define CONFIG_POST_RT_VM(idx)	.load_order = POST_LAUNCHED_VM,	\
				.uuid = POST_RTVM_UUID##idx,	\
				.severity = SEVERITY_RTVM

#define CONFIG_KATA_VM(idx)	.load_order = POST_LAUNCHED_VM,	\
				.uuid = KATA_VM_UUID##idx,	\
				.severity = SEVERITY_STANDARD_VM

/* ACRN guest severity */
enum acrn_vm_severity {
	SEVERITY_SAFETY_VM = 0x40U,
	SEVERITY_RTVM = 0x30U,
	SEVERITY_SERVICE_VM = 0x20U,
	SEVERITY_STANDARD_VM = 0x10U,
};

struct acrn_vm_mem_config {
	uint64_t start_hpa;	/* the start HPA of VM memory configuration, for pre-launched VMs only */
	uint64_t size;		/* VM memory size configuration */
	uint64_t start_hpa2;	/* Start of second HPA for non-contiguous allocations in VM memory configuration,
				   for pre-launched VMs only */
	uint64_t size_hpa2;	/* Size of second HPA for non-contiguous allocations in VM memory configuration */
};

struct target_vuart {
	uint8_t vm_id;		/* target VM id */
	uint8_t vuart_id;	/* target vuart index in a VM */
};

enum vuart_type {
	VUART_MMIO = 0,		/* MMIO vuart */
	VUART_PCI,		/* PCI vuart, may removed */
};

union vuart_addr {
	uint64_t base;		/* addr for mmio type */
	struct {		/* addr for pci type */
		uint8_t f : 3;	/* BITs 0-2 */
		uint8_t d : 5;	/* BITs 3-7 */
		uint8_t b;	/* BITs 8-15 */
	} bdf;
};

struct vuart_config {
	enum vuart_type type;		/* MMIO or PCI  */
	union vuart_addr addr;		/* mmio addr if in mmio type, or bdf addr if in pci type */
	uint16_t irq;
	struct target_vuart t_vuart;	/* target vuart */
} __aligned(8);

enum os_kernel_type {
	KERNEL_BZIMAGE = 1,
	KERNEL_ZEPHYR,
};

#define MAX_BOOTARGS_SIZE		2048U
/* The modules in multiboot are for kernel and ramdisk of pre-launched VMs and SOS VM */
#define MAX_MODULE_NUM			(2U * PRE_VM_NUM + 2U * SOS_VM_NUM)

/* extended flags for acrn multiboot info from multiboot2  */
#define	MULTIBOOT_INFO_HAS_EFI_MMAP	0x00010000U
#define	MULTIBOOT_INFO_HAS_EFI64	0x00020000U

struct acrn_vm_os_config {
	char name[MAX_VM_OS_NAME_LEN];			/* OS name, useful for debug */
	enum os_kernel_type kernel_type;		/* used for kernel specifc loading method */
	char kernel_mod_tag[MAX_MOD_TAG_LEN];		/* multiboot module tag for kernel */
	char ramdisk_mod_tag[MAX_MOD_TAG_LEN];		/* multiboot module tag for ramdisk */
	char bootargs[MAX_BOOTARGS_SIZE];		/* boot args/cmdline */
	uint64_t kernel_load_addr;
	uint64_t kernel_entry_addr;
	uint64_t kernel_ramdisk_addr;
} __aligned(8);

struct acrn_vm_pci_dev_config {
	uint32_t emu_type;				/* the type how the device is emulated. */
	union pci_bdf vbdf;				/* virtual BDF of PCI device */
	union pci_bdf pbdf;				/* physical BDF of PCI device */
	struct target_vuart t_vuart;
	uint16_t vuart_idx;
	uint64_t vbar_base[PCI_BAR_COUNT];		/* vbar base address of PCI device */
	struct pci_pdev *pdev;				/* the physical PCI device if it's a PT device */
	const struct pci_vdev_ops *vdev_ops;		/* operations for PCI CFG read/write */
} __aligned(8);

struct acrn_vm_config {
	enum acrn_vm_load_order load_order;		/* specify the load order of VM */
	char name[MAX_VM_OS_NAME_LEN];			/* VM name identifier, useful for debug. */
	uint8_t reserved[2];				/* Temporarily reserve it so that don't need to update
							 * the users of get_platform_info frequently.
							 */
	uint8_t severity;				/* severity of the VM */
	uint64_t cpu_affinity;				/* The set bits represent the pCPUs the vCPUs of
							 * the VM may run on.
							 */
	uint64_t guest_flags;				/* VM flags that we want to configure for guest
							 * Now we have two flags:
							 *	GUEST_FLAG_SECURE_WORLD_ENABLED
							 *	GUEST_FLAG_LAPIC_PASSTHROUGH
							 * We could add more guest flags in future;
							 */

	struct sched_params sched_params;		/* Scheduler params for vCPUs of this VM */
	uint16_t companion_vm_id;			/* The companion VM id for this VM */
	struct acrn_vm_mem_config memory;		/* memory configuration of VM */
	uint16_t pci_dev_num;				/* indicate how many PCI devices in VM */
	struct acrn_vm_pci_dev_config *pci_devs;	/* point to PCI devices BDF list */
	struct acrn_vm_os_config os_config;		/* OS information the VM */

	/*
	 * below are varaible length members (per build).
	 * SOS can get the vm_configs[] array through hypercall, but SOS may not
	 * need to parse these members.
	 */
	uint16_t clos[MAX_VCPUS_PER_VM];		/* Class of Service, effective only if CONFIG_RDT_ENABLED
							 * is defined on CAT capable platforms
							 */

	struct vuart_config vuart[MAX_VUART_NUM_PER_VM];/* vuart configuration for VM */

	bool pt_tpm2;
	struct acrn_mmiodev mmiodevs[MAX_MMIO_DEV_NUM];
} __aligned(8);

extern struct acrn_vm_config vm_configs[CONFIG_MAX_VM_NUM];
static inline struct acrn_vm_config *get_vm_config(uint16_t vm_id)
{
	return &vm_configs[vm_id];
}
extern uint8_t get_vm_severity(uint16_t vm_id);
extern bool vm_has_matched_uuid(uint16_t vmid, const uint8_t *uuid);
extern bool sanitize_vm_config(void);

#define UNASSIGNED_VBDF        0xFFFFU

#endif /* __RISCV_VM_CONFIG_H__ */

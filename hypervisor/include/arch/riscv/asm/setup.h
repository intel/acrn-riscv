/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_SETUP_H__
#define __RISCV_SETUP_H__

#define MIN_FDT_ALIGN 8
#define MAX_FDT_SIZE SZ_2M
#define NR_MEM_BANKS 128
#define MAX_MODULES 32 /* Current maximum useful modules */

typedef enum {
	BOOTMOD_ACRN,
	BOOTMOD_FDT,
	BOOTMOD_KERNEL,
	BOOTMOD_RAMDISK,
	BOOTMOD_XSM,
	BOOTMOD_GUEST_DTB,
	BOOTMOD_UNKNOWN
}  bootmodule_kind;

struct membank {
	uint64_t start;
	uint64_t size;
};

struct meminfo {
	int nr_banks;
	struct membank bank[NR_MEM_BANKS];
};

/*
 * The domU flag is set for kernels and ramdisks of "acrn,domain" nodes.
 * The purpose of the domU flag is to avoid getting confused in
 * kernel_probe, where we try to guess which is the dom0 kernel and
 * initrd to be compatible with all versions of the multiboot spec. 
 */
#define BOOTMOD_MAX_CMDLINE 1024
struct bootmodule {
	bootmodule_kind kind;
	bool domU;
	uint64_t start;
	uint64_t size;
};

/* DT_MAX_NAME is the node name max length according the DT spec */
#define DT_MAX_NAME 41
struct bootcmdline {
	bootmodule_kind kind;
	bool domU;
	uint64_t start;
	char dt_name[DT_MAX_NAME];
	char cmdline[BOOTMOD_MAX_CMDLINE];
};

struct bootmodules {
	int nr_mods;
	struct bootmodule module[MAX_MODULES];
};

struct bootcmdlines {
	unsigned int nr_mods;
	struct bootcmdline cmdline[MAX_MODULES];
};

struct bootinfo {
	struct meminfo mem;
	struct meminfo reserved_mem;
	struct bootmodules modules;
	struct bootcmdlines cmdlines;
#ifdef CONFIG_ACPI
	struct meminfo acpi;
#endif
};

extern struct bootinfo bootinfo;

typedef uint64_t domid_t;
extern domid_t max_init_domid;

extern void copy_from_paddr(void *dst, uint64_t paddr, unsigned long len);
extern size_t estimate_efi_size(int mem_nr_banks);
extern int acpi_make_efi_nodes(void *fdt, struct membank tbl_add[]);
extern void discard_initial_modules(void);
extern void dt_unreserved_regions(uint64_t s, uint64_t e,
			   void (*cb)(uint64_t, uint64_t), int first);

extern size_t boot_fdt_info(const void *fdt, uint64_t paddr);
extern const char *boot_fdt_cmdline(const void *fdt);
extern struct bootmodule *add_boot_module(bootmodule_kind kind, uint64_t start,
					uint64_t size, bool domU);
extern struct bootmodule *boot_module_find_by_kind(bootmodule_kind kind);
extern struct bootmodule * boot_module_find_by_addr_and_kind(bootmodule_kind kind,
					uint64_t start);
extern void add_boot_cmdline(const char *name, const char *cmdline,
					bootmodule_kind kind, uint64_t start, bool domU);
extern struct bootcmdline *boot_cmdline_find_by_kind(bootmodule_kind kind);
extern struct bootcmdline * boot_cmdline_find_by_name(const char *name);
extern const char *boot_module_kind_as_string(bootmodule_kind kind);

extern uint32_t hyp_traps_vector[];
extern void device_tree_get_reg(const uint32_t **cell, uint32_t address_cells,
				 uint32_t size_cells, uint64_t *start, uint64_t *size);

extern uint32_t device_tree_get_uint32_t(const void *fdt, int node,
					const char *prop_name, uint32_t dflt);

#endif /* __RISCV_SETUP_H__ */

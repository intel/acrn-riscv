/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_PGTABLE_H__
#define __RISCV_PGTABLE_H__

#define SATP_MODE_SV48	0x9000000000000000
#define SATP_PPN_MASK	0x00000FFFFFFFFFFF

#define PG_TABLE_SHIFT			9
#define PG_TABLE_ENTRIES		(_AC(1,U) << PG_TABLE_SHIFT)
#define PG_TABLE_ENTRY_MASK		(PG_TABLE_ENTRIES - 1)

#define  BIT0   0x00000001
#define  BIT1   0x00000002
#define  BIT2   0x00000004
#define  BIT3   0x00000008
#define  BIT4   0x00000010
#define  BIT5   0x00000020
#define  BIT6   0x00000040
#define  BIT7   0x00000080
#define  BIT8   0x00000100
#define  BIT9   0x00000200
#define  BIT10  0x00000400
#define  BIT11  0x00000800
#define  BIT12  0x00001000
#define  BIT13  0x00002000
#define  BIT14  0x00004000
#define  BIT15  0x00008000

#define PTE_ENTRY_COUNT				  512
#define PTE_ADDR_MASK_BLOCK_ENTRY		(0xFFFFFFFFFULL << 10)

#define PAGE_ATTR_MASK				  (((uint64_t)0x3) << 61)
#define PAGE_ATTR_PMA				   (0x0)
#define PAGE_ATTR_UC					(((uint64_t)0x1) << 61)
#define PAGE_ATTR_IO					(((uint64_t)0x2) << 61)
#define PAGE_ATTR_RSV				   (((uint64_t)0x3) << 61)

#define PAGE_CONF_MASK 0xff
#define PAGE_V  BIT0
#define PAGE_R  BIT1
#define PAGE_W  BIT2
#define PAGE_X  BIT3
#define PAGE_U  BIT4
#define PAGE_G  BIT5
#define PAGE_A  BIT6
#define PAGE_D  BIT7

#define PAGE_TYPE_MASK			0xf
#define PAGE_TYPE_TABLE 		(0x0 | PAGE_V)

#define PAGE_NO_RW  (PAGE_V | PAGE_R | PAGE_W)
#define PAGE_RW_RW  (PAGE_V | PAGE_U | PAGE_R | PAGE_W)
#define PAGE_NO_RO  (PAGE_V | PAGE_R)
#define PAGE_RO_RO  (PAGE_V | PAGE_U | PAGE_R)

#define PAGE_ATTRIBUTES_MASK  (PAGE_CONF_MASK | PAGE_ATTR_MASK)

#define DEFINE_PAGE_TABLES(name, nr)					\
pgtable_t __aligned(PAGE_SIZE) name[PG_TABLE_ENTRIES * (nr)]

#define DEFINE_PAGE_TABLE(name) DEFINE_PAGE_TABLES(name, 1)

/* for Sv48, vpn0 shift is 12 */
#define PTE_SHIFT		(PAGE_SHIFT)
#define PTRS_PER_PTE		(PG_TABLE_ENTRIES)
#define PTE_SIZE		(1UL << PTE_SHIFT)
#define PTE_MASK		(~(PTE_SIZE - 1UL))

/* for Sv48, vpn1 shift is 21 */
#define VPN1_SHIFT		(PTE_SHIFT + PG_TABLE_SHIFT)
#define PTRS_PER_VPN1		(PG_TABLE_ENTRIES)
#define VPN1_SIZE		(1UL << VPN1_SHIFT)
#define VPN1_MASK		(~(VPN1_SIZE - 1UL))

/* for Sv48, vpn2 shift is 30 */
#define VPN2_SHIFT		(VPN1_SHIFT + PG_TABLE_SHIFT)
#define PTRS_PER_VPN2		(PG_TABLE_ENTRIES)
#define VPN2_SIZE		(1UL << VPN2_SHIFT)
#define VPN2_MASK		(~(VPN2_SIZE - 1UL))

/* for Sv48, vpn3 shift is 39 */
#define VPN3_SHIFT		(VPN2_SHIFT + PG_TABLE_SHIFT)
#define PTRS_PER_VPN3		(PG_TABLE_ENTRIES)
#define VPN3_SIZE		(1UL << VPN3_SHIFT)
#define VPN3_MASK		(~(VPN3_SIZE - 1UL))


/* TODO: PAGE_MASK & PHYSICAL_MASK */
#define VPN3_PFN_MASK		0x0000FFFFFFFFF000UL
#define VPN2_PFN_MASK		0x0000FFFFFFFFF000UL
#define VPN1_PFN_MASK		0x0000FFFFFFFFF000UL

#define PPT_PFN_HIGH_MASK	0xFFFF000000000000UL
#define INVALID_HPA		(0x1UL << 52U)

#ifndef __ASSEMBLY__

#include <asm/init.h>
#include <asm/page.h>
#include <asm/mem.h>

struct page {
	uint8_t contents[PAGE_SIZE];
} __aligned(PAGE_SIZE);

enum _page_table_level {
	/**
	 * @brief The PML4 level in the page tables
	 */
	VPN3 = 0,
	/**
	 * @brief The Page-Directory-Pointer-Table level in the page tables
	 */
	VPN2 = 1,
	/**
	 * @brief The Page-Directory level in the page tables
	 */
	VPN1 = 2,
	/**
	 * @brief The Page-Table level in the page tables
	 */
	VPN0 = 3,
};

typedef struct __packed {
	uint64_t v:1;
	uint64_t r:1;
	uint64_t w:1;
	uint64_t x:1;
	uint64_t u:1;
	uint64_t g:1;
	uint64_t a:1;
	uint64_t d:1;
	uint64_t rsw:2;
	uint64_t base:44; /* Base address of block or next table */
	uint64_t rsv:7;
	uint64_t pbmt:2;
	uint64_t n:1;
} pgtable_pt_t;

/*
 * Walk is the common bits of p2m and pt entries which are needed to
 * simply walk the table (e.g. for debug).
 */
typedef struct __packed {
	uint64_t v:1;
	uint64_t r:1;
	uint64_t w:1;
	uint64_t x:1;
	uint64_t pad2:6;
	uint64_t base:44; /* Base address of block or next table */
	uint64_t pad1:10;
} pgtable_walk_t;

typedef union {
	uint64_t bits;
	pgtable_pt_t pt;
	pgtable_walk_t walk;
} pgtable_t;

union pgtable_pages_info {
	struct {
		uint64_t top_address_space;
		struct page *vpn3_base;
		struct page *vpn2_base;
		struct page *vpn1_base;
		struct page *vpn0_base;
	} ppt;
	struct {
		uint64_t top_address_space;
		struct page *vpn3_base;
		struct page *vpn2_base;
		struct page *vpn1_base;
		struct page *vpn0_base;
	} s2pt;
};

struct memory_ops {
	union pgtable_pages_info *info;
	bool (*large_page_support)(enum _page_table_level level);
	uint64_t (*get_default_access_right)(void);
	uint64_t (*pgentry_present)(uint64_t pte);
	struct page *(*get_pml4_page)(const union pgtable_pages_info *info);
	struct page *(*get_pdpt_page)(const union pgtable_pages_info *info, uint64_t gpa);
	struct page *(*get_pd_page)(const union pgtable_pages_info *info, uint64_t gpa);
	struct page *(*get_pt_page)(const union pgtable_pages_info *info, uint64_t gpa);
	void *(*get_sworld_memory_base)(const union pgtable_pages_info *info);
	void (*clflush_pagewalk)(const void *p);
	void (*tweak_exe_right)(uint64_t *entry);
	void (*recover_exe_right)(uint64_t *entry);
};

static inline uint64_t round_page_up(uint64_t addr)
{
	return (((addr + (uint64_t)PAGE_SIZE) - 1UL) & PAGE_MASK);
}

static inline uint64_t round_page_down(uint64_t addr)
{
	return (addr & PAGE_MASK);
}

static inline uint64_t round_vpn1_up(uint64_t val)
{
	return (((val + (uint64_t)VPN1_SIZE) - 1UL) & VPN1_MASK);
}

static inline uint64_t round_vpn1_down(uint64_t val)
{
	return (val & VPN1_MASK);
}

static inline uint64_t vpn3_index(uint64_t address)
{
	return (address >> VPN3_SHIFT) & (PTRS_PER_VPN3 - 1UL);
}

static inline uint64_t vpn2_index(uint64_t address)
{
	return (address >> VPN2_SHIFT) & (PTRS_PER_VPN2 - 1UL);
}

static inline uint64_t vpn1_index(uint64_t address)
{
	return (address >> VPN1_SHIFT) & (PTRS_PER_VPN1 - 1UL);
}

static inline uint64_t pte_index(uint64_t address)
{
	return (address >> PTE_SHIFT) & (PTRS_PER_PTE - 1UL);
}

static inline uint64_t acrn_vpn1_index(uint64_t address)
{
	return address >> VPN1_SHIFT;
}

static inline uint64_t *vpn_to_vaddr(const uint64_t *vpn)
{
	pgtable_walk_t *p = (pgtable_walk_t *)vpn;

	return hpa2hva((p->base << PTE_SHIFT) & VPN1_PFN_MASK);
}

static inline uint64_t *vpn3_offset(uint64_t *pml4_page, uint64_t addr)
{
	return pml4_page + vpn3_index(addr);
}

static inline uint64_t *vpn2_offset(const uint64_t *vpn3, uint64_t addr)
{
	return vpn_to_vaddr(vpn3) + vpn2_index(addr);
}

static inline uint64_t *vpn1_offset(const uint64_t *vpn2, uint64_t addr)
{
	return vpn_to_vaddr(vpn2) + vpn1_index(addr);
}

static inline uint64_t *pte_offset(const uint64_t *vpn1, uint64_t addr)
{
	return vpn_to_vaddr(vpn1) + pte_index(addr);
}

static inline uint64_t get_pgentry(const uint64_t *pte)
{
	return *pte;
}

static inline void set_pgentry(uint64_t *ptep, uint64_t pte, const struct memory_ops *mem_ops)
{
	*ptep = pte;
	mem_ops->clflush_pagewalk(ptep);
}

static inline uint64_t vpn_large(uint64_t vpn)
{
	return (vpn & PAGE_V) && ((vpn & PAGE_TYPE_MASK) != PAGE_TYPE_TABLE);
}

extern void mmu_add(uint64_t *pml4_page, uint64_t paddr_base, uint64_t vaddr_base,
		uint64_t size, uint64_t prot, const struct memory_ops *mem_ops);

extern void mmu_modify_or_del(uint64_t *pml4_page, uint64_t vaddr_base, uint64_t size,
		uint64_t prot_set, uint64_t prot_clr, const struct memory_ops *mem_ops, uint32_t type);

extern const uint64_t *lookup_address(uint64_t *vpn3_page, uint64_t addr, uint64_t *pg_size,
					const struct memory_ops *mem_ops);

#define pgtable_get_mfn(pte)	((pte).walk.base)
#define pgtable_set_mfn(pte, mfn)  ((pte).walk.base = mfn)

uint64_t *satp_to_vpn3_page(uint64_t satp);

extern const struct memory_ops ppt_mem_ops;
extern uint64_t init_satp;

/*
 * Memory Type
 */
#define MT_PMA		0x0
#define MT_UC		0x1
#define MT_IO		0x2
#define MT_RSV		0x3

static inline pgtable_t mfn_to_acrn_entry(mfn_t mfn, uint8_t attr, bool table)
{
	volatile pgtable_t e = (pgtable_t) {
		.pt = {
			.v = 1,
			.r = 1,
			.w = 1,
			.x = 0,
		}};

	if (table) {
		e.pt.r = 0;
		e.pt.w = 0;
	}

	switch (attr)
	{
	case MT_IO:
		e.pt.pbmt |= PAGE_ATTR_IO;
		break;
	case MT_UC:
		e.pt.pbmt |= PAGE_ATTR_UC;
		break;
	default:
		e.pt.pbmt |= PAGE_ATTR_PMA;
		break;
	}

	pgtable_set_mfn(e, mfn);

	return e;
}

#define EPT_RD			(1UL << 0U)
#define EPT_WR			(1UL << 1U)
#define EPT_EXE			(1UL << 2U)
#define EPT_RWX			(EPT_RD | EPT_WR | EPT_EXE)

/**
 * @brief EPT memory type is specified in bits 5:3 of the EPT paging-structure entry.
 */
#define EPT_MT_SHIFT		3U

/**
 * @brief EPT memory type is uncacheable.
 */
#define EPT_UNCACHED		(0UL << EPT_MT_SHIFT)

/**
 * @brief EPT memory type is write combining.
 */
#define EPT_WC			(1UL << EPT_MT_SHIFT)

/**
 * @brief EPT memory type is write through.
 */
#define EPT_WT			(4UL << EPT_MT_SHIFT)

/**
 * @brief EPT memory type is write protected.
 */
#define EPT_WP			(5UL << EPT_MT_SHIFT)

/**
 * @brief EPT memory type is write back.
 */
#define EPT_WB			(6UL << EPT_MT_SHIFT)

/** The flag that indicates that the page fault was caused by a non present
 * page.
 */
#define PAGE_FAULT_P_FLAG	  0x00000001U
/** The flag that indicates that the page fault was caused by a write access. */
#define PAGE_FAULT_WR_FLAG	 0x00000002U
/** The flag that indicates that the page fault was caused in user mode. */
#define PAGE_FAULT_US_FLAG	 0x00000004U
/** The flag that indicates that the page fault was caused by a reserved bit
 * violation.
 */
#define PAGE_FAULT_RSVD_FLAG   0x00000008U
/** The flag that indicates that the page fault was caused by an instruction
 * fetch.
 */
#define PAGE_FAULT_ID_FLAG	 0x00000010U

extern void init_s2pt_mem_ops(struct memory_ops *mem_ops, uint16_t vm_id);
#endif /* __ASSEMBLY__ */

#endif /* __RISCV_PGTABLE_H__ */

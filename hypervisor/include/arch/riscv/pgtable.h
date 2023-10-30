/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file pgtable.h
 *
 * @brief Address translation and page table operations
 */
#ifndef __RISCV_PGTABLE_H__
#define __RISCV_PGTABLE_H__

/*
 * These numbers add up to a 48-bit input address space.
 *
 * On 32-bit the zeroeth level does not exist, therefore the total is
 * 39-bits. The RISCVv7-A architecture actually specifies a 40-bit input
 * address space for the p2m, with an 8K (1024-entry) top-level table.
 * However acrn only supports 16GB of RAM on 32-bit RISCV systems and
 * therefore 39-bits are sufficient.
 */
#define SATP_MODE_SV48	0x9000000000000000

#define PG_TABLE_SHIFT      9
#define PG_TABLE_ENTRIES    (_AC(1,U) << PG_TABLE_SHIFT)
#define PG_TABLE_ENTRY_MASK (PG_TABLE_ENTRIES - 1)

#define THIRD_SHIFT    (PAGE_SHIFT)
#define THIRD_ORDER    (THIRD_SHIFT - PAGE_SHIFT)
#define THIRD_SIZE     (_AT(paddr_t, 1) << THIRD_SHIFT)
#define THIRD_MASK     (~(THIRD_SIZE - 1))
#define SECOND_SHIFT   (THIRD_SHIFT + PG_TABLE_SHIFT)
#define SECOND_ORDER   (SECOND_SHIFT - PAGE_SHIFT)
#define SECOND_SIZE    (_AT(paddr_t, 1) << SECOND_SHIFT)
#define SECOND_MASK    (~(SECOND_SIZE - 1))
#define FIRST_SHIFT    (SECOND_SHIFT + PG_TABLE_SHIFT)
#define FIRST_ORDER    (FIRST_SHIFT - PAGE_SHIFT)
#define FIRST_SIZE     (_AT(paddr_t, 1) << FIRST_SHIFT)
#define FIRST_MASK     (~(FIRST_SIZE - 1))
#define ZEROETH_SHIFT  (FIRST_SHIFT + PG_TABLE_SHIFT)
#define ZEROETH_ORDER  (ZEROETH_SHIFT - PAGE_SHIFT)
#define ZEROETH_SIZE   (_AT(paddr_t, 1) << ZEROETH_SHIFT)
#define ZEROETH_MASK   (~(ZEROETH_SIZE - 1))

/* Calculate the offsets into the pagetables for a given VA */
#define zeroeth_linear_offset(va) ((va) >> ZEROETH_SHIFT)
#define first_linear_offset(va) ((va) >> FIRST_SHIFT)
#define second_linear_offset(va) ((va) >> SECOND_SHIFT)
#define third_linear_offset(va) ((va) >> THIRD_SHIFT)

//#define TABLE_OFFSET_OP(offs) (_AT(unsigned int, offs) & PG_TABLE_ENTRY_MASK)
#define TABLE_OFFSET_OP(offs) offs
#define first_table_offset(va)  TABLE_OFFSET_OP(first_linear_offset(va))
#define second_table_offset(va) TABLE_OFFSET_OP(second_linear_offset(va))
#define third_table_offset(va)  TABLE_OFFSET_OP(third_linear_offset(va))
#define zeroeth_table_offset(va)  TABLE_OFFSET_OP(zeroeth_linear_offset(va))

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
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

#define PTE_ENTRY_COUNT                  512
#define PTE_ADDR_MASK_BLOCK_ENTRY        (0xFFFFFFFFFULL << 10)

#define PAGE_ATTR_MASK                  (((uint64_t)0x3) << 61)
#define PAGE_ATTR_PMA                   (0x0)
#define PAGE_ATTR_UC                    (((uint64_t)0x1) << 61)
#define PAGE_ATTR_IO                    (((uint64_t)0x2) << 61)
#define PAGE_ATTR_RSV                   (((uint64_t)0x3) << 61)

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
/*
typedef struct {
	uint64_t phy_base;
	uint64_t virt_base;
	uint64_t length;
	riscv_momory_region_attr_t attrs;
} mem_region_desc_t;
*/

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS 5

#define MACH_VIRT_PERIPH_BASE  0x10000000
#define MACH_VIRT_PERIPH_SIZE  SIZE_512MB

#define DEFINE_PAGE_TABLES(name, nr)                    \
pgtable_t __aligned(PAGE_SIZE) name[PG_TABLE_ENTRIES * (nr)]

#define DEFINE_PAGE_TABLE(name) DEFINE_PAGE_TABLES(name, 1)

#define VPN3_SHIFT		39U
#define PTRS_PER_VPN3		512UL
#define VPN3_SIZE		(1UL << VPN3_SHIFT)
#define VPN3_MASK		(~(VPN3_SIZE - 1UL))

#define VPN2_SHIFT		30U
#define PTRS_PER_VPN2		512UL
#define VPN2_SIZE		(1UL << VPN2_SHIFT)
#define VPN2_MASK		(~(VPN2_SIZE - 1UL))

#define VPN1_SHIFT		21U
#define PTRS_PER_VPN1		512UL
#define VPN1_SIZE		(1UL << VPN1_SHIFT)
#define VPN1_MASK		(~(VPN1_SIZE - 1UL))

#define PTE_SHIFT		12U
#define PTRS_PER_PTE		512UL
#define PTE_SIZE		(1UL << PTE_SHIFT)
#define PTE_MASK		(~(PTE_SIZE - 1UL))

/* TODO: PAGE_MASK & PHYSICAL_MASK */
#define VPN3_PFN_MASK		0x0000FFFFFFFFF000UL
#define VPN2_PFN_MASK		0x0000FFFFFFFFF000UL
#define VPN1_PFN_MASK		0x0000FFFFFFFFF000UL


#ifndef __ASSEMBLY__

#include <acrn/page-defs.h>
#include <acrn/kernel.h>

extern paddr_t phys_offset;
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

union pgtable_pages_info {
	struct {
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

static inline void *hpa2hva(uint64_t hpa)
{
	if ( !is_kernel(hpa - phys_offset) )
		return (void *)hpa;
	else
		return (void *)(hpa - phys_offset);
}

static inline uint64_t hva2hpa(const void *va)
{
	if ( !is_kernel(va) )
		return (uint64_t)va;
	else
		return (uint64_t)va + phys_offset;
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

static inline uint64_t *vpn3_page_vaddr(uint64_t vpn3)
{
	return hpa2hva(vpn3 & VPN3_PFN_MASK);
}

static inline uint64_t *vpn2_page_vaddr(uint64_t vpn2)
{
	return hpa2hva(vpn2 & VPN2_PFN_MASK);
}

static inline uint64_t *vpn1_page_vaddr(uint64_t vpn1)
{
	return hpa2hva(vpn1 & VPN1_PFN_MASK);
}

static inline uint64_t *vpn3_offset(uint64_t *pml4_page, uint64_t addr)
{
	return pml4_page + vpn3_index(addr);
}

static inline uint64_t *vpn2_offset(const uint64_t *vpn3, uint64_t addr)
{
	return vpn3_page_vaddr(*vpn3) + vpn2_index(addr);
}

static inline uint64_t *vpn1_offset(const uint64_t *vpn2, uint64_t addr)
{
	return vpn2_page_vaddr(*vpn2) + vpn1_index(addr);
}

static inline uint64_t *pte_offset(const uint64_t *vpn1, uint64_t addr)
{
	return vpn1_page_vaddr(*vpn1) + pte_index(addr);
}

/*
 * pgentry may means vpn3/vpn2/vpn1/pte
 */
static inline uint64_t get_pgentry(const uint64_t *pte)
{
	return *pte;
}

/*
 * pgentry may means vpn3/vpn2/vpn1/pte
 */
static inline void set_pgentry(uint64_t *ptep, uint64_t pte, const struct memory_ops *mem_ops)
{
	*ptep = pte;
	mem_ops->clflush_pagewalk(ptep);
}

static inline uint64_t vpn1_large(uint64_t vpn1)
{
	return ! ((vpn1 & PAGE_TYPE_MASK) == PAGE_TYPE_TABLE);
}

static inline uint64_t vpn2_large(uint64_t vpn2)
{
	return ! ((vpn2 & PAGE_TYPE_MASK) == PAGE_TYPE_TABLE);
}

void mmu_add(uint64_t *pml4_page, uint64_t paddr_base, uint64_t vaddr_base,
		uint64_t size, uint64_t prot, const struct memory_ops *mem_ops);

void mmu_modify_or_del(uint64_t *pml4_page, uint64_t vaddr_base, uint64_t size,
		uint64_t prot_set, uint64_t prot_clr, const struct memory_ops *mem_ops, uint32_t type);
/**
 * @}
 */


typedef struct __packed {
    unsigned long v:1;
    unsigned long r:1;
    unsigned long w:1;
    unsigned long x:1;
    unsigned long u:1;
    unsigned long g:1;
    unsigned long a:1;
    unsigned long d:1;
    unsigned long rsw:2;
    unsigned long long base:44; /* Base address of block or next table */
    unsigned long rsv:7;
    unsigned long pbmt:2;
    unsigned long n:1;
} pgtable_pt_t;

/* Permission mask: xn, write, read */
#define P2M_PERM_MASK (0x00400000000000C0ULL)
#define P2M_CLEAR_PERM(pte) ((pte).bits & ~P2M_PERM_MASK)

/*
 * Walk is the common bits of p2m and pt entries which are needed to
 * simply walk the table (e.g. for debug).
 */
typedef struct __packed {
    unsigned long v:1;
    unsigned long r:1;
    unsigned long w:1;
    unsigned long x:1;
    unsigned long pad2:6;
    unsigned long long base:44; /* Base address of block or next table */
    unsigned long pad1:10;
} pgtable_walk_t;

typedef union {
    uint64_t bits;
    pgtable_pt_t pt;
    pgtable_walk_t walk;
} pgtable_t;


#define pgtable_get_mfn(pte)    (_mfn((pte).walk.base))
#define pgtable_set_mfn(pte, mfn)  ((pte).walk.base = mfn_x(mfn))

/*
 * AArch64 supports pages with different sizes (4K, 16K, and 64K). To enable
 * page table walks for various configurations, the following helpers enable
 * walking the translation table with varying page size granularities.
 */

#define PG_TABLE_SHIFT_4K           (9)
#define PG_TABLE_SHIFT_16K          (11)
#define PG_TABLE_SHIFT_64K          (13)

#define pgtable_entries(gran)      (_AC(1,U) << PG_TABLE_SHIFT_##gran)
#define pgtable_entry_mask(gran)   (pgtable_entries(gran) - 1)

#define third_shift(gran)       (PAGE_SHIFT_##gran)
#define third_size(gran)        ((paddr_t)1 << third_shift(gran))

#define second_shift(gran)      (third_shift(gran) + PG_TABLE_SHIFT_##gran)
#define second_size(gran)       ((paddr_t)1 << second_shift(gran))

#define first_shift(gran)       (second_shift(gran) + PG_TABLE_SHIFT_##gran)
#define first_size(gran)        ((paddr_t)1 << first_shift(gran))

/* Note that there is no zeroeth lookup level with a 64K granule size. */
#define zeroeth_shift(gran)     (first_shift(gran) + PG_TABLE_SHIFT_##gran)
#define zeroeth_size(gran)      ((paddr_t)1 << zeroeth_shift(gran))

#define TABLE_OFFSET(offs, gran)      (offs & pgtable_entry_mask(gran))
#define TABLE_OFFSET_HELPERS(gran)                                          \
static inline paddr_t third_table_offset_##gran##K(paddr_t va)              \
{                                                                           \
    return TABLE_OFFSET((va >> third_shift(gran##K)), gran##K);             \
}                                                                           \
                                                                            \
static inline paddr_t second_table_offset_##gran##K(paddr_t va)             \
{                                                                           \
    return TABLE_OFFSET((va >> second_shift(gran##K)), gran##K);            \
}                                                                           \
                                                                            \
static inline paddr_t first_table_offset_##gran##K(paddr_t va)              \
{                                                                           \
    return TABLE_OFFSET((va >> first_shift(gran##K)), gran##K);             \
}                                                                           \
                                                                            \
static inline paddr_t zeroeth_table_offset_##gran##K(paddr_t va)            \
{                                                                           \
    /* Note that there is no zeroeth lookup level with 64K granule sizes. */\
    if ( gran == 64 )                                                       \
        return 0;                                                           \
    else                                                                    \
        return TABLE_OFFSET((va >> zeroeth_shift(gran##K)), gran##K);       \
}                                                                           \


TABLE_OFFSET_HELPERS(4);
TABLE_OFFSET_HELPERS(16);
TABLE_OFFSET_HELPERS(64);

/* Generate an array @var containing the offset for each level from @addr */
#define DECLARE_OFFSETS(var, addr)          \
    const unsigned int var[4] = {           \
        zeroeth_table_offset(addr),         \
        first_table_offset(addr),           \
        second_table_offset(addr),          \
        third_table_offset(addr)            \
    }

extern const struct memory_ops ppt_mem_ops;
extern uint64_t init_satp;

/**
 * @defgroup ept_mem_access_right EPT Memory Access Right
 *
 * This is a group that includes EPT Memory Access Right Definitions.
 *
 * @{
 */

/**
 * @brief EPT memory access right is read-only.
 */
#define EPT_RD			(1UL << 0U)

/**
 * @brief EPT memory access right is read/write.
 */
#define EPT_WR			(1UL << 1U)

/**
 * @brief EPT memory access right is executable.
 */
#define EPT_EXE			(1UL << 2U)

/**
 * @brief EPT memory access right is read/write and executable.
 */
#define EPT_RWX			(EPT_RD | EPT_WR | EPT_EXE)

/**
 * @}
 */
/* End of ept_mem_access_right */

/**
 * @defgroup ept_mem_type EPT Memory Type
 *
 * This is a group that includes EPT Memory Type Definitions.
 *
 * @{
 */

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

/**
 * @}
 */
/* End of ept_mem_type */

#define EPT_MT_MASK		(7UL << EPT_MT_SHIFT)
#define EPT_VE			(1UL << 63U)
/* EPT leaf entry bits (bit 52 - bit 63) should be maksed  when calculate PFN */
#define EPT_PFN_HIGH_MASK	0xFFF0000000000000UL

/**
 * @brief Memory Management
 *
 * @defgroup acrn_mem ACRN Memory Management
 * @{
 */
/** The flag that indicates that the page fault was caused by a non present
 * page.
 */
#define PAGE_FAULT_P_FLAG	0x00000001U
/** The flag that indicates that the page fault was caused by a write access. */
#define PAGE_FAULT_WR_FLAG	0x00000002U
/** The flag that indicates that the page fault was caused in user mode. */
#define PAGE_FAULT_US_FLAG	0x00000004U
/** The flag that indicates that the page fault was caused by a reserved bit
 * violation.
 */
#define PAGE_FAULT_RSVD_FLAG	0x00000008U
/** The flag that indicates that the page fault was caused by an instruction
 * fetch.
 */
#define PAGE_FAULT_ID_FLAG	0x00000010U



#endif /* __ASSEMBLY__ */


#endif /* __RISCV_PGTABLE_H__ */

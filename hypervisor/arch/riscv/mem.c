#include <acrn/init.h>
#include <acrn/types.h>
#include <acrn/pfn.h>
#include <acrn/sizes.h>
#include <acrn/mm.h>
#include <acrn/kernel.h>
#include <asm/flushtlb.h>
#include <asm/pgtable.h>
#include <asm/setup.h>
#include <asm/board.h>

#include <debug/logmsg.h>
#include <asm/board.h>
#include <asm/defconfig.h>


/*
 * Macros to define page-tables:
 *  - DEFINE_BOOT_PAGE_TABLE is used to define page-table that are used
 *  in assembly code before BSS is zeroed.
 *  - DEFINE_PAGE_TABLE{,S} are used to define one or multiple
 *  page-tables to be used after BSS is zeroed (typically they are only used
 *  in C).
 */
#define DEFINE_BOOT_PAGE_TABLE(name)										  \
pgtable_t __aligned(PAGE_SIZE) __section(".data.page_aligned") name[PG_TABLE_ENTRIES]


#define DEFINE_BOOT_PAGE_TABLEs(name, num)										  \
pgtable_t __aligned(PAGE_SIZE) __section(".data.page_aligned") name[PG_TABLE_ENTRIES * num]

#define LEVEL3_PAGE_NUM  (CONFIG_TEXT_SIZE >> 21)

DEFINE_BOOT_PAGE_TABLE(boot_pgtable);
DEFINE_BOOT_PAGE_TABLE(boot_first);
DEFINE_BOOT_PAGE_TABLE(boot_first_id);
DEFINE_BOOT_PAGE_TABLE(boot_second_id);
DEFINE_BOOT_PAGE_TABLE(boot_third_id);
DEFINE_BOOT_PAGE_TABLE(boot_second);
DEFINE_BOOT_PAGE_TABLEs(boot_third, LEVEL3_PAGE_NUM);
DEFINE_BOOT_PAGE_TABLE(acrn_fixmap);

DEFINE_PAGE_TABLE(acrn_pgtable);
DEFINE_PAGE_TABLE(acrn_first);

/* Common pagetable leaves */
/* Second level page tables.
 *
 * The second-level table is 8 contiguous pages long, and covers all
 * addresses from 0 to 0x1ffffffff. Offsets into it are calculated
 * with second_linear_offset(), not second_table_offset().
 */
DEFINE_PAGE_TABLES(acrn_second, 8);

/* First level page table used to map Xen itself with the XN bit set
 * as appropriate. */
DEFINE_PAGE_TABLES(acrn_acrnmap, LEVEL3_PAGE_NUM);

/* Non-boot CPUs use this to find the correct pagetables. */
uint64_t init_satp;

paddr_t phys_offset;

extern void switch_satp(uint64_t satp);

/*
 * @brief  Copies at most slen bytes from src address to dest address, up to dmax.
 *
 *   INPUTS
 *
 * @param[in] d		pointer to Destination address
 * @param[in] dmax	 maximum  length of dest
 * @param[in] s		pointer to Source address
 * @param[in] slen	 maximum number of bytes of src to copy
 *
 * @return 0 for success and -1 for runtime-constraint violation.
 */
static int32_t memcpy_s(void *d, size_t dmax, const void *s, size_t slen)
{
	int32_t ret = -1;

	if ((d != NULL) && (s != NULL) && (dmax >= slen) && ((d > (s + slen)) || (s > (d + dmax)))) {
		if (slen != 0U) {
			memcpy(d, s, slen);
		}
		ret = 0;
	} else {
		(void)memset(d, 0U, dmax);
	}

	return ret;
}

static inline pgtable_t mfn_to_acrn_entry(mfn_t mfn, unsigned attr, bool table)
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

	switch ( attr )
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

	ASSERT(!(mfn_to_maddr(mfn) & ~PADDR_MASK));

	pgtable_set_mfn(e, mfn);

	return e;
}

static inline pgtable_t pte_of_acrnaddr(vaddr_t va, bool table)
{
	paddr_t ma = va + phys_offset;

	return mfn_to_acrn_entry(maddr_to_mfn(ma), MT_PMA, table);
}

static inline pgtable_t pte_of_ioaddr(vaddr_t va, bool table)
{
	paddr_t ma = va + phys_offset;

	return mfn_to_acrn_entry(maddr_to_mfn(ma), MT_IO, table);
}

static void acrn_pt_enforce_wnx(void)
{
	isb();
	flush_acrn_tlb_local();
}

static void __init map_hv(unsigned long boot_phys_offset)
{
	uint64_t satp;
	pgtable_t pte, *p;
	int i, j;

	phys_offset = boot_phys_offset;

	p = (void *) acrn_pgtable;
	p[0] = pte_of_acrnaddr((uintptr_t)acrn_first, true);
	p = (void *) acrn_first;

	for ( i = 0; i < 8; i++)
	{
		p[i] = pte_of_acrnaddr((uintptr_t)(acrn_second+i*PG_TABLE_ENTRIES), true);
	}

	for ( i = 0; i < 8 * 512; i++ ) {
		int t = second_table_offset(ACRN_VIRT_START);
		vaddr_t va = i << VPN1_SHIFT;
		if (i >= t)
			break;
		pte = pte_of_ioaddr(va, false);
		acrn_second[i] = pte;
	}

	for (j = 0; j < LEVEL3_PAGE_NUM; j++) {
		pte = pte_of_acrnaddr((uintptr_t)(acrn_acrnmap + j * PG_TABLE_ENTRIES), true);
		acrn_second[second_table_offset(ACRN_VIRT_START + (j << VPN1_SHIFT))] = pte;
		for ( i = 0; i < PG_TABLE_ENTRIES; i++ )
		{
			vaddr_t va = ACRN_VIRT_START + (i << PAGE_SHIFT) + (j << VPN1_SHIFT);

			pte = pte_of_acrnaddr(va, false);
			if ( is_kernel_text(va) || is_kernel_inittext(va) )
			{
				pte.pt.w = 1;
				pte.pt.x = 1;
			}
			if ( is_kernel_rodata(va) )
				pte.pt.w = 1;
			acrn_acrnmap[i + j * PG_TABLE_ENTRIES] = pte;
		}
	}

	pte = pte_of_acrnaddr((vaddr_t)acrn_fixmap, false);
	acrn_second[second_table_offset(FIXMAP_ADDR(0))] = pte;

	satp = (uintptr_t) acrn_pgtable + phys_offset;
	init_satp = (satp >> 12) | SATP_MODE_SV48;

	switch_satp(init_satp);
	acrn_pt_enforce_wnx();
}

static void __init map_mem(void)
{
	mmu_add((uint64_t *)acrn_pgtable, BOARD_HV_DEVICE_START,
		BOARD_HV_DEVICE_START, BOARD_HV_DEVICE_SIZE,
		PAGE_V | PAGE_ATTR_IO,
		&ppt_mem_ops);
/*
	mmu_add((uint64_t *)acrn_pgtable, BOARD_HV_RAM_START, BOARD_HV_RAM_START, BOARD_HV_RAM_SIZE,
		PAGE_V | PAGE_ATTR_PMA | PAGE_U,
		&ppt_mem_ops);
*/
}

void __init setup_pagetables(unsigned long boot_phys_offset)
{
	map_hv(boot_phys_offset);

	map_mem();
}

static void clear_table(void *table)
{
	clear_page(table);
	clean_and_invalidate_dcache_va_range(table, PAGE_SIZE);
}

void clear_fixmap_pagetable(void)
{
	clear_table(acrn_fixmap);
}

static void clear_boot_pagetables(void)
{
	clear_table(boot_pgtable);
	clear_table(boot_first);
	clear_table(boot_first_id);
	clear_table(boot_second);
	clear_table(boot_third);
}

int init_secondary_pagetables(int cpu)
{
	clear_boot_pagetables();
	clean_dcache(init_satp);

	return 0;
}

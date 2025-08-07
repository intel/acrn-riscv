/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SVPN1X-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <rtl.h>
#include <debug/logmsg.h>
#include <asm/init.h>
#include <asm/mem.h>
#include <asm/tlb.h>
#include <asm/pgtable.h>
#include <asm/setup.h>
#include <asm/board.h>
#include <asm/cache.h>
#include <asm/page.h>

#ifndef CONFIG_MACRN

#define DEFINE_BOOT_PAGE_TABLE(name)										\
pgtable_t __aligned(PAGE_SIZE) __attribute__((__section__(".data.page_aligned"))) name[PG_TABLE_ENTRIES]

#define DEFINE_BOOT_PAGE_TABLEs(name, num)									\
pgtable_t __aligned(PAGE_SIZE) __attribute__((__section__(".data.page_aligned"))) name[PG_TABLE_ENTRIES * num]

#define VPN1_PAGE_NUM  (CONFIG_TEXT_SIZE >> 21)

DEFINE_BOOT_PAGE_TABLE(acrn_fixmap);

DEFINE_PAGE_TABLE(acrn_vpn3);
DEFINE_PAGE_TABLE(acrn_vpn2);
DEFINE_PAGE_TABLES(acrn_vpn1, 8);
DEFINE_PAGE_TABLES(acrn_vpn0, VPN1_PAGE_NUM);

/* Non-boot CPUs use this to find the correct pagetables. */
uint64_t init_satp;

static inline pgtable_t pte_of_acrnaddr(uint64_t va, bool table)
{
	uint64_t ma = va + phys_offset;

	return mfn_to_acrn_entry(maddr_to_mfn(ma), MT_PMA, table);
}

static inline pgtable_t pte_of_ioaddr(uint64_t va, bool table)
{
	uint64_t ma = va + phys_offset;

	return mfn_to_acrn_entry(maddr_to_mfn(ma), MT_IO, table);
}

static void map_hv(uint64_t boot_phys_offset)
{
	uint64_t satp;
	pgtable_t pte, *p;
	int i, j;

	phys_offset = boot_phys_offset;
	p = (void *)acrn_vpn3;
	p[0] = pte_of_acrnaddr((uint64_t)acrn_vpn2, true);
	p = (void *)acrn_vpn2;

	for (i = 0; i < 8; i++)
	{
		p[i] = pte_of_acrnaddr((uint64_t)(acrn_vpn1+i*PG_TABLE_ENTRIES), true);
	}

	/* Map 0 ~ (ACRN_VIRT_START - 1) as IO address space */
	for (i = 0; i < 8 * 512; i++) {
		int t = acrn_vpn1_index(ACRN_VIRT_START);
		uint64_t va = i << VPN1_SHIFT;
		if (i >= t)
			break;
		pte = pte_of_ioaddr(va, false);
		acrn_vpn1[i] = pte;
	}

	for (j = 0; j < VPN1_PAGE_NUM; j++) {
		pte = pte_of_acrnaddr((uint64_t)(acrn_vpn0 + j * PG_TABLE_ENTRIES), true);
		acrn_vpn1[acrn_vpn1_index(ACRN_VIRT_START + (j << VPN1_SHIFT))] = pte;
		for ( i = 0; i < PG_TABLE_ENTRIES; i++ )
		{
			uint64_t va = ACRN_VIRT_START + (i << PAGE_SHIFT) + (j << VPN1_SHIFT);

			pte = pte_of_acrnaddr(va, false);
			if (is_hv_text(va))
			{
				pte.pt.w = 0;
				pte.pt.x = 1;
			}
			acrn_vpn0[i + j * PG_TABLE_ENTRIES] = pte;
		}
	}

	pte = pte_of_acrnaddr((uint64_t)acrn_fixmap, false);
	acrn_vpn1[acrn_vpn1_index(FIXMAP_ADDR(0))] = pte;

	satp = (uint64_t)acrn_vpn3 + phys_offset;
	init_satp = (satp >> 12) | SATP_MODE_SV48;

	switch_satp(init_satp);
}

static void map_mem(void)
{
	mmu_add((uint64_t *)acrn_vpn3, BOARD_HV_DEVICE_START,
		BOARD_HV_DEVICE_START, BOARD_HV_DEVICE_SIZE,
		PAGE_V | PAGE_ATTR_IO,
		&ppt_mem_ops);

	mmu_add((uint64_t *)acrn_vpn3, BOARD_HV_RAM_START, BOARD_HV_RAM_START, BOARD_HV_RAM_SIZE,
		PAGE_V | PAGE_ATTR_PMA | PAGE_U,
		&ppt_mem_ops);
}

static void clear_table(void *table)
{
	clear_page(table);
	flush_dcache_range(table, PAGE_SIZE);
}

void clear_fixmap_pagetable(void)
{
	clear_table(acrn_fixmap);
}

static void clear_boot_pagetables(void)
{
}

int init_secondary_pagetables(int cpu)
{
	clear_boot_pagetables();
	clean_dcache(init_satp);

	return 0;
}

#else /* CONFIG_MACRN */

static void map_hv(uint64_t boot_phys_offset)
{
	phys_offset = boot_phys_offset;
}

static void map_mem(void)
{
}

#endif

uint64_t phys_offset;
void setup_mem(uint64_t boot_phys_offset)
{
	map_hv(boot_phys_offset);
	map_mem();
}

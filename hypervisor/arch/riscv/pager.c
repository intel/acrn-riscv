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
#include <asm/init.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/vm_config.h>

#define VPN3_PAGE_NUM(size)	1UL
#define VPN2_PAGE_NUM(size)	(((size) + VPN3_SIZE - 1UL) >> VPN3_SHIFT)
#define VPN1_PAGE_NUM(size)	(((size) + VPN2_SIZE - 1UL) >> VPN2_SHIFT)
#define VPN0_PAGE_NUM(size)	(((size) + VPN1_SIZE - 1UL) >> VPN1_SHIFT)

#ifndef CONFIG_MACRN
static struct page vm_vpn3_pages[CONFIG_MAX_VM_NUM][VPN3_PAGE_NUM(CONFIG_GUEST_ADDRESS_SPACE_SIZE)] __aligned(PAGE_SIZE << 2);
static struct page vm_vpn2_pages[CONFIG_MAX_VM_NUM][VPN2_PAGE_NUM(CONFIG_GUEST_ADDRESS_SPACE_SIZE)] __aligned(PAGE_SIZE);
static struct page vm_vpn1_pages[CONFIG_MAX_VM_NUM][VPN1_PAGE_NUM(CONFIG_GUEST_ADDRESS_SPACE_SIZE)] __aligned(PAGE_SIZE);
static struct page vm_vpn0_pages[CONFIG_MAX_VM_NUM][VPN0_PAGE_NUM(CONFIG_GUEST_ADDRESS_SPACE_SIZE)] __aligned(PAGE_SIZE);

static union pgtable_pages_info s2pt_pages_info[CONFIG_MAX_VM_NUM];
#endif

static inline bool large_page_support(enum _page_table_level level)
{
	if (level == VPN1 || level == VPN2)
		return true;
	else
		return false; 
}

static inline uint64_t ppt_get_default_access_right(void)
{
	return PAGE_V;
}

static inline void ppt_clflush_pagewalk(const void* entry __attribute__((unused)))
{
}

static inline uint64_t ppt_pgentry_present(uint64_t pte)
{
	return pte & PAGE_V;
}

static inline struct page *ppt_get_vpn3_page(const union pgtable_pages_info *info)
{
	struct page *vpn3_page = info->ppt.vpn3_base;
	(void)memset(vpn3_page, 0U, PAGE_SIZE);
	return vpn3_page;
}

static inline struct page *ppt_get_vpn2_page(const union pgtable_pages_info *info, uint64_t gpa)
{
	struct page *vpn2_page = info->ppt.vpn2_base + ((gpa & VPN3_MASK) >> VPN3_SHIFT);
	(void)memset(vpn2_page, 0U, PAGE_SIZE);
	return vpn2_page;
}

static inline struct page *ppt_get_vpn1_page(const union pgtable_pages_info *info, uint64_t gpa)
{

	struct page *vpn1_page = info->ppt.vpn1_base + ((gpa &  VPN2_MASK) >> VPN2_SHIFT);
	(void)memset(vpn1_page, 0U, PAGE_SIZE);
	return vpn1_page;
}

static inline struct page *ppt_get_vpn0_page(const union pgtable_pages_info *info, uint64_t gpa)
{

	struct page *vpn0_page = info->ppt.vpn0_base + ((gpa &  VPN1_MASK) >> VPN1_SHIFT);
	(void)memset(vpn0_page, 0U, PAGE_SIZE);
	return vpn0_page;
}

uint64_t *satp_to_vpn3_page(uint64_t satp)
{
	return (uint64_t *)((satp & SATP_PPN_MASK) << 12);
}

static inline void nop_tweak_exe_right(uint64_t *entry __attribute__((unused))) {}
static inline void nop_recover_exe_right(uint64_t *entry __attribute__((unused))) {}

#ifndef CONFIG_MACRN
extern pgtable_t acrn_vpn3[];
extern pgtable_t acrn_vpn2[];
extern pgtable_t acrn_vpn1[];

static union pgtable_pages_info ppt_pages_info = {
	.ppt = {
		.vpn3_base = (struct page *)acrn_vpn3,
		.vpn2_base = (struct page *)acrn_vpn2,
		.vpn1_base = (struct page *)acrn_vpn1,
	}
};
#else
static union pgtable_pages_info ppt_pages_info = {
	.ppt = {
		.vpn3_base = NULL,
		.vpn2_base = NULL,
		.vpn1_base = NULL,
	}
};
#endif

const struct memory_ops ppt_mem_ops = {
	.info = &ppt_pages_info,
	.large_page_support = large_page_support,
	.get_default_access_right = ppt_get_default_access_right,
	.pgentry_present = ppt_pgentry_present,
	.get_pml4_page = ppt_get_vpn3_page,
	.get_pdpt_page = ppt_get_vpn2_page,
	.get_pd_page = ppt_get_vpn1_page,
	.get_pt_page = ppt_get_vpn0_page,
	.clflush_pagewalk = ppt_clflush_pagewalk,
	.tweak_exe_right = nop_tweak_exe_right,
	.recover_exe_right = nop_recover_exe_right,
};

#ifndef CONFIG_MACRN
static inline struct page *s2pt_get_vpn3_page(const union pgtable_pages_info *info)
{
	struct page *vpn3_page = info->s2pt.vpn3_base;
	(void)memset(vpn3_page, 0U, PAGE_SIZE);
	return vpn3_page;
}

static inline struct page *s2pt_get_vpn2_page(const union pgtable_pages_info *info, uint64_t gpa)
{
	struct page *vpn2_page = info->s2pt.vpn2_base + ((gpa & VPN3_MASK) >> VPN3_SHIFT);
	(void)memset(vpn2_page, 0U, PAGE_SIZE);
	return vpn2_page;
}

static inline struct page *s2pt_get_vpn1_page(const union pgtable_pages_info *info, uint64_t gpa)
{

	struct page *vpn1_page = info->s2pt.vpn1_base + ((gpa & VPN2_MASK) >> VPN2_SHIFT);
	(void)memset(vpn1_page, 0U, PAGE_SIZE);
	return vpn1_page;
}

static inline struct page *s2pt_get_vpn0_page(const union pgtable_pages_info *info, uint64_t gpa)
{
	struct page *vpn0_page = info->s2pt.vpn0_base + ((gpa & VPN1_MASK) >> VPN1_SHIFT);
	(void)memset(vpn0_page, 0U, PAGE_SIZE);
	return vpn0_page;
}

static inline void s2pt_clflush_pagewalk(const void* entry)
{
}

static inline uint64_t s2pt_get_default_access_right(void)
{
	return PAGE_V;
}

static inline uint64_t s2pt_pgentry_present(uint64_t pte)
{
	return pte & PAGE_V;
}

void init_s2pt_mem_ops(struct memory_ops *mem_ops, uint16_t vm_id)
{
	s2pt_pages_info[vm_id].s2pt.top_address_space = CONFIG_GUEST_ADDRESS_SPACE_SIZE;
	s2pt_pages_info[vm_id].s2pt.vpn3_base = vm_vpn3_pages[vm_id];
	s2pt_pages_info[vm_id].s2pt.vpn2_base = vm_vpn2_pages[vm_id];
	s2pt_pages_info[vm_id].s2pt.vpn1_base = vm_vpn1_pages[vm_id];
	s2pt_pages_info[vm_id].s2pt.vpn0_base = vm_vpn0_pages[vm_id];

	mem_ops->info = &s2pt_pages_info[vm_id];
	mem_ops->get_default_access_right = s2pt_get_default_access_right;
	mem_ops->pgentry_present = s2pt_pgentry_present;
	mem_ops->get_pml4_page = s2pt_get_vpn3_page;
	mem_ops->get_pdpt_page = s2pt_get_vpn2_page;
	mem_ops->get_pd_page = s2pt_get_vpn1_page;
	mem_ops->get_pt_page = s2pt_get_vpn0_page;
	mem_ops->clflush_pagewalk = s2pt_clflush_pagewalk;
	mem_ops->large_page_support = large_page_support;
	mem_ops->tweak_exe_right = nop_tweak_exe_right;
	mem_ops->recover_exe_right = nop_recover_exe_right;
}
#endif

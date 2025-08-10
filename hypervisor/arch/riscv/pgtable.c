/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <util.h>
#include <asm/init.h>
#include <asm/mem.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <debug/logmsg.h>
#include <acrn_hv_defs.h>

static inline pgtable_t pte_of_memaddr(uint64_t va, bool table)
{
	uint64_t ma = va + phys_offset;

	return mfn_to_acrn_entry(maddr_to_mfn(ma), MT_PMA, table);
}

/*
 * pgentry of vpn3/vpn2/vpn1
 */
static inline void construct_pgentry(uint64_t *ptep, void *pd_page, uint64_t prot, const struct memory_ops *mem_ops)
{
	*ptep = pte_of_memaddr((uint64_t)pd_page, true).bits;
	//*ptep |= PAGE_U;
	mem_ops->clflush_pagewalk(ptep);
	//set_pgentry(vpn1, (hva2hpa(pd_page) >> 2) | (prot & ~(PAGE_R | PAGE_W | PAGE_X)), mem_ops);
	//pr_dbg("table %lx (virt:%lx)", hva2hpa(pd_page) | (prot | PAGE_TABLE), pd_page);
}

/* the vpn0 entry */
static inline void construct_pte(uint64_t *ptep, uint64_t paddr, uint64_t prot, const struct memory_ops *mem_ops)
{
	*ptep = pte_of_memaddr((uint64_t)paddr, false).bits;
	*ptep |= PAGE_U | PAGE_X;
	mem_ops->clflush_pagewalk(ptep);
	//set_pgentry(ptep, (paddr >> 2) | prot, mem_ops);
	//pr_dbg("non level-3 pte: %lx", paddr | (prot & ~PAGE_TABLE));
}

/*
 * Split a large page table into next level page table.
 *
 * @pre: level could only VPN2 or VPN1
 */
static void split_large_page(uint64_t *pte, enum _page_table_level level,
		uint64_t vaddr, const struct memory_ops *mem_ops)
{
	uint64_t *pbase;
	uint64_t ref_paddr, paddr, paddrinc;
	uint64_t i, ref_prot;

	switch (level) {
	case VPN2:
		ref_paddr = (*pte) & VPN2_PFN_MASK;
		paddrinc = VPN1_SIZE;
		ref_prot = (*pte) & ~VPN2_PFN_MASK;
		pbase = (uint64_t *)mem_ops->get_pd_page(mem_ops->info, vaddr);
		break;
	default:	/* VPN1 */
		ref_paddr = (*pte) & VPN1_PFN_MASK;
		paddrinc = PTE_SIZE;
		ref_prot = (*pte) & ~VPN1_PFN_MASK;
		mem_ops->recover_exe_right(&ref_prot);
		pbase = (uint64_t *)mem_ops->get_pt_page(mem_ops->info, vaddr);
		break;
	}

	pr_dbg("%s, paddr: 0x%lx, pbase: 0x%lx", __func__, ref_paddr, pbase);

	paddr = ref_paddr;
	for (i = 0UL; i < PTRS_PER_PTE; i++) {
		if (level == VPN1) {
			construct_pgentry(pbase + i, (void *)paddr, ref_prot, mem_ops);
		} else {
			construct_pte(pbase + i, paddr, ref_prot, mem_ops);
		}
		paddr += paddrinc;
	}

	ref_prot = mem_ops->get_default_access_right();
	construct_pgentry(pte, (void *)pbase, ref_prot, mem_ops);

	/* TODO: flush the TLB */
}

static inline void local_modify_or_del_pte(uint64_t *pte,
		uint64_t prot_set, uint64_t prot_clr, uint32_t type, const struct memory_ops *mem_ops)
{
	uint64_t new_pte = *pte;
	if (type == MR_MODIFY) {
		new_pte &= ~prot_clr;
		new_pte |= prot_set;
		set_pgentry(pte, new_pte, mem_ops);
	} else if (type == MR_DEL) {
		set_pgentry(pte, 0, mem_ops);
	} else {
		pr_warn("not support such modify type: %d, DO NOTHING!!", type);
	}
}

/*
 * In PT level,
 * type: MR_MODIFY
 * modify [vaddr_start, vaddr_end) memory type or page access right.
 * type: MR_DEL
 * delete [vaddr_start, vaddr_end) MT PT mapping
 */
static void modify_or_del_pte(const uint64_t *vpn1, uint64_t vaddr_start, uint64_t vaddr_end,
		uint64_t prot_set, uint64_t prot_clr, const struct memory_ops *mem_ops, uint32_t type)
{
	uint64_t *pt_page = vpn_to_vaddr(vpn1);
	uint64_t vaddr = vaddr_start;
	uint64_t index = pte_index(vaddr);

	pr_dbg("%s, vaddr: [0x%lx - 0x%lx]", __func__, vaddr, vaddr_end);
	for (; index < PTRS_PER_PTE; index++) {
		uint64_t *pte = pt_page + index;

		if ((mem_ops->pgentry_present(*pte) == 0UL)) {
			if (type == MR_MODIFY) {
				pr_dbg("%s, vaddr: 0x%lx pte is not present.", __func__, vaddr);
			}
		} else {
			local_modify_or_del_pte(pte, prot_set, prot_clr, type, mem_ops);
		}

		vaddr += PTE_SIZE;
		if (vaddr >= vaddr_end) {
			break;
		}
	}
}

/*
 * In PD level,
 * type: MR_MODIFY
 * modify [vaddr_start, vaddr_end) memory type or page access right.
 * type: MR_DEL
 * delete [vaddr_start, vaddr_end) MT PT mapping
 */
static void modify_or_del_vpn1(const uint64_t *vpn2, uint64_t vaddr_start, uint64_t vaddr_end,
		uint64_t prot_set, uint64_t prot_clr, const struct memory_ops *mem_ops, uint32_t type)
{
	uint64_t *pd_page = vpn_to_vaddr(vpn2);
	uint64_t vaddr = vaddr_start;
	uint64_t index = vpn1_index(vaddr);

	pr_dbg("%s, vaddr: [0x%lx - 0x%lx]", __func__, vaddr, vaddr_end);
	for (; index < PTRS_PER_VPN1; index++) {
		uint64_t *vpn1 = pd_page + index;
		uint64_t vaddr_next = (vaddr & VPN1_MASK) + VPN1_SIZE;

		if (mem_ops->pgentry_present(*vpn1) == 0UL) {
			if (type == MR_MODIFY) {
				pr_dbg("%s, addr: 0x%lx vpn1 is not present.", __func__, vaddr);
			}
		} else {
			if (vpn_large(*vpn1) != 0UL) {
				if ((vaddr_next > vaddr_end) || (!mem_aligned_check(vaddr, VPN1_SIZE))) {
					split_large_page(vpn1, VPN1, vaddr, mem_ops);
				} else {
					local_modify_or_del_pte(vpn1, prot_set, prot_clr, type, mem_ops);
					if (vaddr_next < vaddr_end) {
						vaddr = vaddr_next;
						continue;
					}
					break;	/* done */
				}
			}
			modify_or_del_pte(vpn1, vaddr, vaddr_end, prot_set, prot_clr, mem_ops, type);
		}
		if (vaddr_next >= vaddr_end) {
			break;	/* done */
		}
		vaddr = vaddr_next;
	}
}

/*
 * In PDPT level,
 * type: MR_MODIFY
 * modify [vaddr_start, vaddr_end) memory type or page access right.
 * type: MR_DEL
 * delete [vaddr_start, vaddr_end) MT PT mapping
 */
static void modify_or_del_vpn2(const uint64_t *vpn3, uint64_t vaddr_start, uint64_t vaddr_end,
		uint64_t prot_set, uint64_t prot_clr, const struct memory_ops *mem_ops, uint32_t type)
{
	uint64_t *vpn2_page = vpn_to_vaddr(vpn3);
	uint64_t vaddr = vaddr_start;
	uint64_t index = vpn2_index(vaddr);

	pr_dbg("%s, vaddr: [0x%lx - 0x%lx]", __func__, vaddr, vaddr_end);
	for (; index < PTRS_PER_VPN2; index++) {
		uint64_t *vpn2 = vpn2_page + index;
		uint64_t vaddr_next = (vaddr & VPN2_MASK) + VPN2_SIZE;

		if (mem_ops->pgentry_present(*vpn2) == 0UL) {
			if (type == MR_MODIFY) {
				pr_dbg("%s, vaddr: 0x%lx vpn2 is not present.", __func__, vaddr);
			}
		} else {
			if (vpn_large(*vpn2) != 0UL) {
				if ((vaddr_next > vaddr_end) ||
						(!mem_aligned_check(vaddr, VPN2_SIZE))) {
					split_large_page(vpn2, VPN3, vaddr, mem_ops);
				} else {
					local_modify_or_del_pte(vpn2, prot_set, prot_clr, type, mem_ops);
					if (vaddr_next < vaddr_end) {
						vaddr = vaddr_next;
						continue;
					}
					break;	/* done */
				}
			}
			modify_or_del_vpn1(vpn2, vaddr, vaddr_end, prot_set, prot_clr, mem_ops, type);
		}
		if (vaddr_next >= vaddr_end) {
			break;	/* done */
		}
		vaddr = vaddr_next;
	}
}

/*
 * type: MR_MODIFY
 * modify [vaddr, vaddr + size ) memory type or page access right.
 * prot_clr - memory type or page access right want to be clear
 * prot_set - memory type or page access right want to be set
 * @pre: the prot_set and prot_clr should set before call this function.
 * If you just want to modify access rights, you can just set the prot_clr
 * to what you want to set, prot_clr to what you want to clear. But if you
 * want to modify the MT, you should set the prot_set to what MT you want
 * to set, prot_clr to the MT mask.
 * type: MR_DEL
 * delete [vaddr_base, vaddr_base + size ) memory region page table mapping.
 */
void mmu_modify_or_del(uint64_t *vpn3_page, uint64_t vaddr_base, uint64_t size,
		uint64_t prot_set, uint64_t prot_clr, const struct memory_ops *mem_ops, uint32_t type)
{
	uint64_t vaddr = round_page_up(vaddr_base);
	uint64_t vaddr_next, vaddr_end;
	uint64_t *vpn3;

	vaddr_end = vaddr + round_page_down(size);
	pr_dbg("%s, vaddr: 0x%lx, size: 0x%lx",
		__func__, vaddr, size);

	while (vaddr < vaddr_end) {
		vaddr_next = (vaddr & VPN3_MASK) + VPN3_SIZE;
		vpn3 = vpn3_offset(vpn3_page, vaddr);
		if ((mem_ops->pgentry_present(*vpn3) == 0UL) && (type == MR_MODIFY)) {
			ASSERT(false);
		} else {
			modify_or_del_vpn2(vpn3, vaddr, vaddr_end, prot_set, prot_clr, mem_ops, type);
			vaddr = vaddr_next;
		}
	}
}

/*
 * In PT level,
 * add [vaddr_start, vaddr_end) to [paddr_base, ...) MT PT mapping
 */
static void add_pte(const uint64_t *vpn1, uint64_t paddr_start, uint64_t vaddr_start, uint64_t vaddr_end,
		uint64_t prot, const struct memory_ops *mem_ops)
{
	uint64_t *pt_page = vpn_to_vaddr(vpn1);
	uint64_t vaddr = vaddr_start;
	uint64_t paddr = paddr_start;
	uint64_t index = pte_index(vaddr);

	pr_dbg("%s, paddr: 0x%lx, vaddr: [0x%lx - 0x%lx]",
		__func__, paddr, vaddr_start, vaddr_end);
	for (; index < PTRS_PER_PTE; index++) {
		uint64_t *pte = pt_page + index;

		if (mem_ops->pgentry_present(*pte) != 0UL) {
			pr_dbg("%s, pte 0x%lx is already present!", __func__, vaddr);
		} else {
			construct_pte(pte, paddr, prot, mem_ops);
		}
		paddr += PTE_SIZE;
		vaddr += PTE_SIZE;

		if (vaddr >= vaddr_end) {
			break;	/* done */
		}
	}
}

/*
 * In PD level,
 * add [vaddr_start, vaddr_end) to [paddr_base, ...) MT PT mapping
 */
static void add_vpn1(const uint64_t *vpn2, uint64_t paddr_start, uint64_t vaddr_start, uint64_t vaddr_end,
		uint64_t prot, const struct memory_ops *mem_ops)
{
	uint64_t *pd_page = vpn_to_vaddr(vpn2);
	uint64_t vaddr = vaddr_start;
	uint64_t paddr = paddr_start;
	uint64_t index = vpn1_index(vaddr);

	pr_dbg("%s, paddr: 0x%lx, vaddr: [0x%lx - 0x%lx]",
		__func__, paddr, vaddr, vaddr_end);
	for (; index < PTRS_PER_VPN1; index++) {
		uint64_t *vpn1 = pd_page + index;
		uint64_t vaddr_next = (vaddr & VPN1_MASK) + VPN1_SIZE;

		if (vpn_large(*vpn1) != 0UL) {
			pr_dbg("%s, vpn1 0x%lx is already present!", __func__, vaddr);
		} else {
			if (mem_ops->pgentry_present(*vpn1) == 0UL) {
				if (mem_ops->large_page_support(VPN1) &&
					mem_aligned_check(paddr, VPN1_SIZE) &&
					mem_aligned_check(vaddr, VPN1_SIZE) &&
					(vaddr_next <= vaddr_end)) {
					mem_ops->tweak_exe_right(&prot);
					construct_pte(vpn1, paddr, prot, mem_ops);
					if (vaddr_next < vaddr_end) {
						paddr += (vaddr_next - vaddr);
						vaddr = vaddr_next;
						continue;
					}
					break;	/* done */
				} else {
					void *pt_page = mem_ops->get_pt_page(mem_ops->info, vaddr);
					construct_pgentry(vpn1, (void *)pt_page, mem_ops->get_default_access_right(), mem_ops);
				}
			}
			add_pte(vpn1, paddr, vaddr, vaddr_end, prot, mem_ops);
		}
		if (vaddr_next >= vaddr_end) {
			break;	/* done */
		}
		paddr += (vaddr_next - vaddr);
		vaddr = vaddr_next;
	}
}

/*
 * In PDPT level,
 * add [vaddr_start, vaddr_end) to [paddr_base, ...) MT PT mapping
 */
static void add_vpn2(const uint64_t *vpn3, uint64_t paddr_start, uint64_t vaddr_start, uint64_t vaddr_end,
		uint64_t prot, const struct memory_ops *mem_ops)
{
	uint64_t *vpn2_page = vpn_to_vaddr(vpn3);
	uint64_t vaddr = vaddr_start;
	uint64_t paddr = paddr_start;
	uint64_t index = vpn2_index(vaddr);

	pr_dbg("%s, paddr: 0x%lx, vaddr: [0x%lx - 0x%lx]", __func__, paddr, vaddr, vaddr_end);
	for (; index < PTRS_PER_VPN2; index++) {
		uint64_t *vpn2 = vpn2_page + index;
		uint64_t vaddr_next = (vaddr & VPN2_MASK) + VPN2_SIZE;

		if (vpn_large(*vpn2) != 0UL) {
			pr_dbg("%s, vpn2 0x%lx is already present!", __func__, vaddr);
		} else {
			if (mem_ops->pgentry_present(*vpn2) == 0UL) {
				if (mem_ops->large_page_support(VPN2) &&
					mem_aligned_check(paddr, VPN2_SIZE) &&
					mem_aligned_check(vaddr, VPN2_SIZE) &&
					(vaddr_next <= vaddr_end)) {
					mem_ops->tweak_exe_right(&prot);
					construct_pte(vpn2, paddr, prot, mem_ops);
					if (vaddr_next < vaddr_end) {
						paddr += (vaddr_next - vaddr);
						vaddr = vaddr_next;
						continue;
					}
					break;	/* done */
				} else {
					void *pd_page = mem_ops->get_pd_page(mem_ops->info, vaddr);
					construct_pgentry(vpn2, pd_page, mem_ops->get_default_access_right(), mem_ops);
				}
			}
			add_vpn1(vpn2, paddr, vaddr, vaddr_end, prot, mem_ops);
		}
		if (vaddr_next >= vaddr_end) {
			break;	/* done */
		}
		paddr += (vaddr_next - vaddr);
		vaddr = vaddr_next;
	}
}

static void tlb_all_sync(void)
{
	if (!smp_ops)
		return;

	return smp_ops->rfence(cpu_online_map, 0, 0);
}

void mmu_add(uint64_t *vpn3_page, uint64_t paddr_base, uint64_t vaddr_base, uint64_t size, uint64_t prot,
		const struct memory_ops *mem_ops)
{
	uint64_t vaddr, vaddr_next, vaddr_end;
	uint64_t paddr;
	uint64_t *vpn3;

	pr_dbg("%s, paddr 0x%lx, vaddr 0x%lx, size 0x%lx", __func__, paddr_base, vaddr_base, size);

	/* align address to page size*/
	vaddr = round_page_up(vaddr_base);
	paddr = round_page_up(paddr_base);
	vaddr_end = vaddr + round_page_down(size);

	while (vaddr < vaddr_end) {
		vaddr_next = (vaddr & VPN3_MASK) + VPN3_SIZE;
		vpn3 = vpn3_offset(vpn3_page, vaddr);
		if (mem_ops->pgentry_present(*vpn3) == 0UL) {
			void *vpn2_page = mem_ops->get_pdpt_page(mem_ops->info, vaddr);
			construct_pgentry(vpn3, vpn2_page, mem_ops->get_default_access_right(), mem_ops);
		}
		add_vpn2(vpn3, paddr, vaddr, vaddr_end, prot, mem_ops);

		paddr += (vaddr_next - vaddr);
		vaddr = vaddr_next;
	}

	tlb_all_sync();
}

const uint64_t *lookup_address(uint64_t *vpn3_page, uint64_t addr, uint64_t *pg_size, const struct memory_ops *mem_ops)
{
	const uint64_t *pret = NULL;
	bool present = true;
	uint64_t *vpn3, *vpn2, *vpn1, *pte;

	vpn3 = vpn3_offset(vpn3_page, addr);

#ifdef CONFIG_MACRN
	if ((uint64_t)vpn3 >= 0x180000000)
		return NULL;
#endif
	present = (mem_ops->pgentry_present(*vpn3) != 0UL);

	if (present) {
		vpn2 = vpn2_offset(vpn3, addr);
		present = (mem_ops->pgentry_present(*vpn2) != 0UL);
		if (present) {
			if (vpn_large(*vpn2) != 0UL) {
				*pg_size = VPN2_SIZE;
				pret = vpn2;
			} else {
				vpn1 = vpn1_offset(vpn2, addr);
				present = (mem_ops->pgentry_present(*vpn1) != 0UL);
				if (present) {
					if (vpn_large(*vpn1) != 0UL) {
						*pg_size = VPN1_SIZE;
						pret = vpn1;
					} else {
						pte = pte_offset(vpn1, addr);
						present = (mem_ops->pgentry_present(*pte) != 0UL);
						if (present) {
							*pg_size = PTE_SIZE;
							pret = pte;
						}
					}
				}
			}
		}
	}

	return pret;
}

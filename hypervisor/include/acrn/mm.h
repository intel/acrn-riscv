/******************************************************************************
 * include/acrn/mm.h
 *
 * Definitions for memory pages, frame numbers, addresses, allocations, etc.
 *
 * Copyright (c) 2002-2006, K A Fraser <keir@xensource.com>
 *
 *                         +---------------------+
 *                          Xen Memory Management
 *                         +---------------------+
 *
 * Xen has to handle many different address spaces.  It is important not to
 * get these spaces mixed up.  The following is a consistent terminology which
 * should be adhered to.
 *
 * mfn: Machine Frame Number
 *   The values Xen puts into its own pagetables.  This is the host physical
 *   memory address space with RAM, MMIO etc.
 *
 * gfn: Guest Frame Number
 *   The values a guest puts in its own pagetables.  For an auto-translated
 *   guest (hardware assisted with 2nd stage translation, or shadowed), gfn !=
 *   mfn.  For a non-translated guest which is aware of Xen, gfn == mfn.
 *
 * pfn: Pseudophysical Frame Number
 *   A linear idea of a guest physical address space. For an auto-translated
 *   guest, pfn == gfn while for a non-translated guest, pfn != gfn.
 *
 * dfn: Device DMA Frame Number (definitions in include/acrn/iommu.h)
 *   The linear frame numbers of device DMA address space. All initiators for
 *   (i.e. all devices assigned to) a guest share a single DMA address space
 *   and, by default, Xen will ensure dfn == pfn.
 *
 * WARNING: Some of these terms have changed over time while others have been
 * used inconsistently, meaning that a lot of existing code does not match the
 * definitions above.  New code should use these terms as described here, and
 * over time older code should be corrected to be consistent.
 *
 * An incomplete list of larger work area:
 * - Phase out the use of 'pfn' from the x86 pagetable code.  Callers should
 *   know explicitly whether they are talking about mfns or gfns.
 * - Phase out the use of 'pfn' from the ARM mm code.  A cursory glance
 *   suggests that 'mfn' and 'pfn' are currently used interchangeably, where
 *   'mfn' is the appropriate term to use.
 * - Phase out the use of gpfn/gmfn where pfn/mfn are meant.  This excludes
 *   the x86 shadow code, which uses gmfn/smfn pairs with different,
 *   documented, meanings.
 */

#ifndef __ACRN_MM_H__
#define __ACRN_MM_H__


#include <acrn/typesafe.h>

TYPE_SAFE(unsigned long, mfn);

#ifndef mfn_t
#define mfn_t /* Grep fodder: mfn_t, _mfn() and mfn_x() are defined above */
#define _mfn
#define mfn_x
#undef mfn_t
#undef _mfn
#undef mfn_x
#endif


#define pfn_to_paddr(pfn) ((paddr_t)(pfn) << PAGE_SHIFT)
#define paddr_to_pfn(pa)  ((unsigned long)((pa) >> PAGE_SHIFT))
#define mfn_to_maddr(mfn)   pfn_to_paddr(mfn_x(mfn))
#define maddr_to_mfn(ma)    _mfn(paddr_to_pfn(ma))
#define gfn_to_gaddr(gfn)   pfn_to_paddr(gfn_x(gfn))

static inline unsigned int get_order_from_bytes(paddr_t size)
{
    unsigned int order;

    size = (size - 1) >> PAGE_SHIFT;
    for ( order = 0; size; order++ )
        size >>= 1;

    return order;
}

TYPE_SAFE(unsigned long, gfn);
#define PRI_gfn          "05lx"
#define INVALID_GFN      _gfn(~0UL)
/*
 * To be used for global variable initialization. This workaround a bug
 * in GCC < 5.0 https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64856
 */
#define INVALID_GFN_INITIALIZER { ~0UL }

#ifndef gfn_t
#define gfn_t /* Grep fodder: gfn_t, _gfn() and gfn_x() are defined above */
#define _gfn
#define gfn_x
#undef gfn_t
#undef _gfn
#undef gfn_x
#endif

#define gaddr_to_gfn(ga)    _gfn(paddr_to_pfn(ga))

#endif /* __ACRN_MM_H__ */

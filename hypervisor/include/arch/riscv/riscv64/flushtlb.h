#ifndef __RISCV_RISCV64_FLUSHTLB_H__
#define __RISCV_RISCV64_FLUSHTLB_H__

#define GTLB_HELPER(name) \
static inline void name(void)   \
{                               \
    asm volatile(               \
        "hfence.gvma;"           \
        : : : "memory");        \
}

/* Flush local TLBs, current VMID only. */
GTLB_HELPER(flush_guest_tlb_local);

/* Flush innershareable TLBs, current VMID only */
GTLB_HELPER(flush_guest_tlb);

/* Flush local TLBs, all VMIDs, non-hypervisor mode */
GTLB_HELPER(flush_all_guests_tlb_local);

/* Flush innershareable TLBs, all VMIDs, non-hypervisor mode */
GTLB_HELPER(flush_all_guests_tlb);

#define TLB_HELPER(name) \
static inline void name(void)   \
{                               \
    asm volatile(               \
        "sfence.vma;"           \
        : : : "memory");        \
}

/* Flush all hypervisor mappings from the TLB of the local processor. */
TLB_HELPER(flush_acrn_tlb_local);

/* Flush TLB of local processor for address va. */
static inline void  __flush_acrn_tlb_one_local(vaddr_t va)
{
    asm volatile("sfence.vma;" : : "r" (va>>PAGE_SHIFT) : "memory");
}

/* Flush TLB of all processors in the inner-shareable domain for address va. */
static inline void __flush_acrn_tlb_one(vaddr_t va)
{
    asm volatile("sfence.vma;" : : "r" (va>>PAGE_SHIFT) : "memory");
}

#endif /* __RISCV_RISCV64_FLUSHTLB_H__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */

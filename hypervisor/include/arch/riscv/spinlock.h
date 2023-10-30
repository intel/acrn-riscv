#ifndef __RISCV_SPINLOCK_H__
#define __RISCV_SPINLOCK_H__

#define arch_lock_acquire_barrier() smp_mb()
#define arch_lock_release_barrier() smp_mb()

#define arch_lock_relax() wfe()
#define arch_lock_signal() do { \
    dsb(ishst);                 \
    sev();                      \
} while(0)

#define arch_lock_signal_wmb()  arch_lock_signal()

#endif /* __RISCV_SPINLOCK_H__ */

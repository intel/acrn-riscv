#include <acrn/lib.h>
#include <public/irq.h>
#include <acrn/smp.h>
#include <acrn/spinlock.h>
//#include <public/sysctl.h>
#include <asm/atomic.h>


static always_inline spinlock_tickets_t observe_lock(spinlock_tickets_t *t)
{
    spinlock_tickets_t v;

    smp_rmb();
    v.head_tail = read_atomic(&t->head_tail);
    return v;
}

static always_inline u16 observe_head(spinlock_tickets_t *t)
{
    smp_rmb();
    return read_atomic(&t->head);
}

void _spin_lock(spinlock_t *lock)
{
    spinlock_tickets_t tickets = SPINLOCK_TICKET_INC;

    //preempt_disable();
    tickets.head_tail = arch_fetch_and_add(&lock->tickets.head_tail,
                                           tickets.head_tail);
    while ( tickets.tail != observe_head(&lock->tickets) )
    {
        arch_lock_relax();
    }
    arch_lock_acquire_barrier();
}

void _spin_lock_irq(spinlock_t *lock)
{
    ASSERT(local_irq_is_enabled());
    local_irq_disable();
    _spin_lock(lock);
}

unsigned long _spin_lock_irqsave(spinlock_t *lock)
{
    unsigned long flags;

    local_irq_save(flags);
    _spin_lock(lock);
    return flags;
}

void _spin_unlock(spinlock_t *lock)
{
    arch_lock_release_barrier();
    add_sized(&lock->tickets.head, 1);
    arch_lock_signal();
    //preempt_enable();
}

void _spin_unlock_irq(spinlock_t *lock)
{
    _spin_unlock(lock);
    local_irq_enable();
}

void _spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
    _spin_unlock(lock);
    local_irq_restore(flags);
}

#ifdef CONFIG_RISCV
#define cmpxchg(ptr, o, n) 0
#endif

int _spin_trylock(spinlock_t *lock)
{
    spinlock_tickets_t old, new;

    old = observe_lock(&lock->tickets);
    if ( old.head != old.tail )
        return 0;
    new = old;
    new.tail++;
    //preempt_disable();
    if ( cmpxchg(&lock->tickets.head_tail,
                 old.head_tail, new.head_tail) != old.head_tail )
    {
        //preempt_enable();
        return 0;
    }
    /*
     * cmpxchg() is a full barrier so no need for an
     * arch_lock_acquire_barrier().
     */
    return 1;
}

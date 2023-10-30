#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <asm/system.h>
#include <asm/spinlock.h>
#include <asm/types.h>


typedef union {
    u32 head_tail;
    struct {
        u16 head;
        u16 tail;
    };
} spinlock_tickets_t;

#define SPINLOCK_TICKET_INC { .head_tail = 0x10000, }

#define SPIN_LOCK_UNLOCKED { { 0 } }
#define DEFINE_SPINLOCK(l) spinlock_t l = SPIN_LOCK_UNLOCKED

typedef struct spinlock {
    spinlock_tickets_t tickets;
} spinlock_t;


#define spin_lock_init(l) (*(l) = (spinlock_t)SPIN_LOCK_UNLOCKED)

void _spin_lock(spinlock_t *lock);
void _spin_lock_irq(spinlock_t *lock);
unsigned long _spin_lock_irqsave(spinlock_t *lock);

void _spin_unlock(spinlock_t *lock);
void _spin_unlock_irq(spinlock_t *lock);
void _spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);

int _spin_is_locked(spinlock_t *lock);
int _spin_trylock(spinlock_t *lock);


#define spin_lock(l)                  _spin_lock(l)
#define spin_lock_irq(l)              _spin_lock_irq(l)
#define spin_lock_irqsave(l, f)                                 \
    ({                                                          \
        BUILD_BUG_ON(sizeof(f) != sizeof(unsigned long));       \
        ((f) = _spin_lock_irqsave(l));                          \
    })

#define spin_unlock(l)                _spin_unlock(l)
#define spin_unlock_irq(l)            _spin_unlock_irq(l)
#define spin_unlock_irqrestore(l, f)  _spin_unlock_irqrestore(l, f)

#define spin_is_locked(l)             _spin_is_locked(l)
#define spin_trylock(l)               _spin_trylock(l)

#define spin_trylock_irqsave(lock, flags)       \
({                                              \
    local_irq_save(flags);                      \
    spin_trylock(lock) ?                        \
    1 : ({ local_irq_restore(flags); 0; });     \
})

#define spinlock_obtain(l) _spin_lock(l)
#define spinlock_release(l) _spin_unlock(l)

#endif /* __SPINLOCK_H__ */

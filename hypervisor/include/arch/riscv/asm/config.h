/******************************************************************************
 * config.h
 *
 * A Linux-style configuration list.
 */

#ifndef __RISCV_CONFIG_H__
#define __RISCV_CONFIG_H__

# define LONG_BYTEORDER 3
# define ELFSIZE 64

#define BYTES_PER_LONG (1 << LONG_BYTEORDER)
#define BITS_PER_LONG (BYTES_PER_LONG << 3)
#define POINTER_ALIGN BYTES_PER_LONG

#define BITS_PER_LLONG 64

/* acrn_ulong_t is always 64 bits */
#define BITS_PER_ACRN_ULONG 64

#define CONFIG_PAGING_LEVELS 3

#define CONFIG_RISCV_L1_CACHE_SHIFT 7 /* XXX */
#define CONFIG_IRQ_HAS_MULTIPLE_ACTION 1

#define CONFIG_PAGEALLOC_MAX_ORDER 18
#define CONFIG_DOMU_MAX_ORDER      9
#define CONFIG_HWDOM_MAX_ORDER     10

#define OPT_CONSOLE_STR "dtuart"

#define MAX_VIRT_CPUS 128u

#define INVALID_VCPU_ID MAX_VIRT_CPUS

#define __LINUX_RISCV_ARCH__ 7
#define CONFIG_AEABI

#define IS_ALIGNED(val, align) (((val) & ((align) - 1)) == 0)

#define ALIGN2 .align 2
#define ENTRY(name)                             \
  .globl name;                                  \
  ALIGN2;                                        \
  name:
#define GLOBAL(name)                            \
  .globl name;                                  \
  name:
#define END(name) \
  .size name, .-name
#define ENDPROC(name) \
  .type name, %function; \
  END(name)

#ifdef __ASSEMBLY__
#define _AC(X,Y)	X
#define _AT(T,X)	X
#else
#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define _AT(T,X)	((T)(X))
#endif

/*
 * Common RISCV64 layout:
 *   0  -   2M   Unmapped
 *   2M -   (2M + CONFIG_TEXT_SIZE)   acrn text, data, bss
 *   (2M + CONFIG_TEXT_SIZE) -  (2M + CONFIG_TEXT_SIZE + 2M)   Fixmap: special-purpose 4K mapping slots
 *
 * RISCV64 layout:
 *   0  -  CONFIG_TEXT_SIZE + 4M   <COMMON>
 *
 * The rest part could map as identical mapping
 *
 */

#define ACRN_VIRT_START         _AT(uint64_t, 0x80000000)
#define ACRN_MSTACK_START       _AT(uint64_t, ACRN_VIRT_START + 0x80000)
#define ACRN_MSTACK_SIZE        CONFIG_MSTACK_SIZE
#define ACRN_STACK_START        _AT(uint64_t, ACRN_VIRT_START + 0x70000)
#define ACRN_STACK_SIZE         CONFIG_STACK_SIZE
#define ACRN_VSTACK_START       _AT(uint64_t, ACRN_VIRT_START + 0x60000)
#define ACRN_VSTACK_SIZE        CONFIG_STACK_SIZE
#define FIXMAP_ADDR(n)          (_AT(uint64_t, ACRN_VIRT_START+ CONFIG_TEXT_SIZE) + (n) * PAGE_SIZE)

#ifdef CONFIG_LIVEPATCH
#define LIVEPATCH_VMAP_START   _AT(uint64_t, ACRN_VIRT_START + 0xa00000)
#define LIVEPATCH_VMAP_END     (LIVEPATCH_VMAP_START + MB(2))
#endif

#define HYPERVISOR_VIRT_START  ACRN_VIRT_START

#define SLOT0_ENTRY_BITS  39
#define SLOT0(slot) (_AT(uint64_t,slot) << SLOT0_ENTRY_BITS)
#define SLOT0_ENTRY_SIZE  SLOT0(1)

#define VMAP_VIRT_START  GB(1)
#define VMAP_VIRT_END    (VMAP_VIRT_START + GB(1))

#define FRAMETABLE_VIRT_START  GB(32)
#define FRAMETABLE_SIZE        GB(32)
#define FRAMETABLE_NR          (FRAMETABLE_SIZE / sizeof(*frame_table))
#define FRAMETABLE_VIRT_END    (FRAMETABLE_VIRT_START + FRAMETABLE_SIZE - 1)

#define DIRECTMAP_VIRT_START   SLOT0(256)
#define DIRECTMAP_SIZE         (SLOT0_ENTRY_SIZE * (265-256))
#define DIRECTMAP_VIRT_END     (DIRECTMAP_VIRT_START + DIRECTMAP_SIZE - 1)

#define ACRNHEAP_VIRT_START     acrnheap_virt_start

#define HYPERVISOR_VIRT_END    DIRECTMAP_VIRT_END

/* Fixmap slots */
#define FIXMAP_CONSOLE  0  /* The primary UART */
#define FIXMAP_MISC     1  /* Ephemeral mappings of hardware */
#define FIXMAP_ACPI_BEGIN  2  /* Start mappings of ACPI tables */
#define FIXMAP_ACPI_END    (FIXMAP_ACPI_BEGIN + NUM_FIXMAP_ACPI_PAGES - 1)  /* End mappings of ACPI tables */

#define PAGE_SHIFT          12
#define PAGE_SIZE           (1 << PAGE_SHIFT)
#define PAGE_MASK           (~(PAGE_SIZE-1))
#define PAGE_FLAG_MASK      (~0)

#define NR_hypercalls 64

#define STACK_ORDER 3
#define STACK_SIZE  (PAGE_SIZE << STACK_ORDER)

#ifndef __ASSEMBLY__
extern unsigned long acrn_phys_start;
extern unsigned long acrnheap_phys_end;
extern unsigned long frametable_virt_end;
#endif

#define watchdog_disable() ((void)0)
#define watchdog_enable()  ((void)0)

#endif /* __RISCV_CONFIG_H__ */

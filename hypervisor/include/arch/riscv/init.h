#ifndef __RISCV_INIT_H__
#define __RISCV_INIT_H__

#define SP_BOTTOM_MAGIC    0x696e746cUL
struct init_info
{
    /* Pointer to the stack, used by head.S when entering in C */
    unsigned char *stack;
    /* Logical CPU ID, used by start_secondary */
    unsigned int cpuid;
};

#endif /* __RISCV_INIT_H__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */

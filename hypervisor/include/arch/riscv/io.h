#ifndef __RISCV_IO_H__
#define __RISCV_IO_H__

#if defined(CONFIG_RISCV_32)
# include <asm/riscv32/io.h>
#elif defined(CONFIG_RISCV_64)
# include <asm/riscv64/io.h>
#else
# error "unknown RISCV variant"
#endif

#endif
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */

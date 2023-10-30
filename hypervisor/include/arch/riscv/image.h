#ifndef __RISCV_IMAGE_H__
#define __RISCV_IMAGE_H__

#include <asm/guest/vm.h>

void get_kernel_info(struct kernel_info *info);
void get_dtb_info(struct dtb_info *info);


#endif

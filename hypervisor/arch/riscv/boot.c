#include <asm/image.h>
#include <asm/config.h>

#ifndef CONFIG_EFI_BOOT
#define KERNEL_IMAGE_START 0
#define KERNEL_IMAGE_SIZE 0
#define DTB_IMAGE_START 0
#define	DTB_IMAGE_SIZE 0

void get_kernel_info(struct kernel_info *info)
{
	info->kernel_addr = KERNEL_IMAGE_START;
	info->kernel_len = KERNEL_IMAGE_SIZE;
}

void get_dtb_info(struct dtb_info *info)
{
	info->dtb_addr = DTB_IMAGE_START;
	info->dtb_len = DTB_IMAGE_SIZE;
}

/* get the actual Hypervisor load address (HVA) */
uint64_t get_hv_image_base(void)
{
	return 0;
}

#endif

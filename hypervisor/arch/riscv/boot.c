/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <asm/image.h>
#include <asm/config.h>
#include <asm/boot.h>

#ifndef CONFIG_EFI_BOOT

#define KERNEL_IMAGE_SIZE	0x10000000
#define DTB_IMAGE_SIZE		CONFIG_SOS_DTB_SIZE

#ifdef CONFIG_MACRN
struct fw_dynamic_info *fw_dinfo = NULL;
paddr_t fw_dtb = 0UL;

void get_kernel_info(struct kernel_info *info)
{
	info->kernel_addr = fw_dinfo->addr;
	info->kernel_len = KERNEL_IMAGE_SIZE;
}

void get_dtb_info(struct dtb_info *info)
{
	info->dtb_addr = fw_dtb;
	info->dtb_len = DTB_IMAGE_SIZE;
}
#else
#define KERNEL_IMAGE_START	CONFIG_SOS_MEM_START
#define DTB_IMAGE_START		CONFIG_SOS_DTB_BASE

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
#endif

/* get the actual Hypervisor load address (HVA) */
uint64_t get_hv_image_base(void)
{
	return 0;
}

#endif

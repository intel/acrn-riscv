/*-
 * Copyright (c) 2018 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NETAPP, INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NETAPP, INC OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <asm/byteorder.h>

#include "dm.h"
#include "vmmapi.h"
#include "sw_load.h"
#include "log.h"

#define SETUP_SIG 0x5a5aaa55

/* PSR bits (CPSR, SPSR) */

#define PSR_THUMB       (1<<5)        /* Thumb Mode enable */
#define PSR_FIQ_MASK    (1<<6)        /* Fast Interrupt mask */
#define PSR_IRQ_MASK    (1<<7)        /* Interrupt mask */
#define PSR_ABT_MASK    (1<<8)        /* Asynchronous Abort mask */
#define PSR_BIG_ENDIAN  (1<<9)        /* arm32: Big Endian Mode */
#define PSR_DBG_MASK    (1<<9)        /* arm64: Debug Exception mask */
#define PSR_IT_MASK     (0x0600fc00)  /* Thumb If-Then Mask */
#define PSR_JAZELLE     (1<<24)       /* Jazelle Mode */

#define PSR_MODE_EL1h 0x05

#define PSR_GUEST64_INIT (PSR_ABT_MASK|PSR_FIQ_MASK|PSR_IRQ_MASK|PSR_MODE_EL1h)

/* If we load kernel/ramdisk/bootargs directly, the UOS
 * memory layout will be like:
 *
 * ctx->baseaddr
 *
 * | ...                                                 |
 * +-----------------------------------------------------+
 * | offset: 60MB (dtb)                         |
 * +-----------------------------------------------------+
 * | offset: 64MB (kernel image)                         |
 * +-----------------------------------------------------+
 * | offset: 128MB (initrd image)                         |
 * +-----------------------------------------------------+
 * | offset: lowmem - RAMDISK_LOAD_SIZE (ramdisk image)  |
 * +-----------------------------------------------------+
 * | offset: lowmem - 8K (bootargs)                      |
 * +-----------------------------------------------------+
 * | offset: lowmem - 6K (kernel entry address)          |
 * +-----------------------------------------------------+
 * | offset: lowmem - 4K (zero_page include e820 table)  |
 * +-----------------------------------------------------+
 */

/* Check default e820 table in sw_load_common.c for info about ctx->lowmem */
/* use ramdisk size for ramdisk load offset, leave 8KB room for bootargs */
#define BOOTARGS_LOAD_OFF(ctx)	(ctx->lowmem - 8*KB)
#define KERNEL_ENTRY_OFF(ctx)	(ctx->lowmem - 6*KB)
#define ZEROPAGE_LOAD_OFF(ctx)	(ctx->lowmem - 4*KB)
#define KERNEL_LOAD_OFF(ctx)	(ctx->lowmem_start + 64*MB)
#define DTB_LOAD_OFF(ctx)	(ctx->lowmem_start + 60*MB)
#define RAMDISK_LOAD_OFF(ctx)	(ctx->lowmem_start + 128*MB)

/* See Documentation/arm64/booting.txt in the Linux kernel */
struct image_header {
	uint32_t	code0;		/* Executable code */
	uint32_t	code1;		/* Executable code */
	uint64_t	text_offset;	/* Image load offset, LE */
	uint64_t	image_size;	/* Effective Image size, LE */
	uint64_t	flags;		/* Kernel flags, LE */
	uint64_t	res2;		/* reserved */
	uint64_t	res3;		/* reserved */
	uint64_t	res4;		/* reserved */
	uint32_t	magic;		/* Magic number */
	uint32_t	res5;
} *image_hdr;

static char dtb_path[STR_LEN];
static char ramdisk_path[STR_LEN];
static char kernel_path[STR_LEN];
static int with_ramdisk;
static int with_kernel;
static int with_dtb;
static size_t ramdisk_size;
static size_t dtb_size;
static size_t kernel_size;

int
acrn_parse_kernel(char *arg)
{
	size_t len = strnlen(arg, STR_LEN);

	if (len < STR_LEN) {
		strncpy(kernel_path, arg, len + 1);
		if (check_image(kernel_path, 0, &kernel_size) != 0){
			fprintf(stderr, "SW_LOAD: check_image failed for '%s'\n",
				kernel_path);
			exit(10); /* Non-zero */
		}
		kernel_file_name = kernel_path;

		with_kernel = 1;
		printf("SW_LOAD: get kernel path %s\n", kernel_path);
		return 0;
	} else
		return -1;
}

int
acrn_parse_ramdisk(char *arg)
{
	size_t len = strnlen(arg, STR_LEN);

	if (len < STR_LEN) {
		strncpy(ramdisk_path, arg, len + 1);
		if (check_image(ramdisk_path, 0, &ramdisk_size) != 0){
			fprintf(stderr, "SW_LOAD: check_image failed for '%s'\n",
				ramdisk_path);
			exit(11); /* Non-zero */
		}

		with_ramdisk = 1;
		printf("SW_LOAD: get ramdisk path %s\n", ramdisk_path);
		return 0;
	} else
		return -1;
}

int
acrn_parse_dtb(char *arg)
{
	size_t len = strnlen(arg, STR_LEN);

	if (len < STR_LEN) {
		strncpy(dtb_path, arg, len + 1);
		if (check_image(dtb_path, 0, &dtb_size) != 0){
			fprintf(stderr, "SW_LOAD: check_image failed for '%s'\n",
				dtb_path);
			exit(11); /* Non-zero */
		}

		with_dtb = 1;
		printf("SW_LOAD: get dtb path %s\n", dtb_path);
		return 0;
	} else
		return -1;
}

static int
acrn_prepare_dtb(struct vmctx *ctx)
{
	FILE *fp;
	long len;
	size_t read;

	fp = fopen(dtb_path, "r");
	if (fp == NULL) {
		printf("SW_LOAD ERR: could not open dtb file %s\n",
				dtb_path);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);

	if (len != dtb_size) {
		fprintf(stderr, "SW_LOAD ERR: dtb file changed\n");
		fclose(fp);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	read = fread(ctx->baseaddr + DTB_LOAD_OFF(ctx),	sizeof(char), len, fp);
	if (read < len) {
		pr_err("SW_LOAD ERR: could not read the whole dtb file,"
				" file len=%ld, read %lu\n", len, read);
		fclose(fp);
		return -1;
	}
	fclose(fp);
	pr_info("SW_LOAD: dtb %s size %lu copied to guest 0x%lx\n",
			dtb_path, dtb_size, DTB_LOAD_OFF(ctx));

	return 0;
}

static int
acrn_prepare_ramdisk(struct vmctx *ctx)
{
	FILE *fp;
	long len;
	size_t read;

	fp = fopen(ramdisk_path, "r");
	if (fp == NULL) {
		printf("SW_LOAD ERR: could not open ramdisk file %s\n",
				ramdisk_path);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);

	if (len != ramdisk_size) {
		fprintf(stderr,
			"SW_LOAD ERR: ramdisk file changed\n");
		fclose(fp);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	read = fread(ctx->baseaddr + RAMDISK_LOAD_OFF(ctx),
			sizeof(char), len, fp);
	if (read < len) {
		pr_err("SW_LOAD ERR: could not read the whole ramdisk file,"
				" file len=%ld, read %lu\n", len, read);
		fclose(fp);
		return -1;
	}
	fclose(fp);
	pr_info("SW_LOAD: ramdisk %s size %lu copied to guest 0x%lx\n",
			ramdisk_path, ramdisk_size, RAMDISK_LOAD_OFF(ctx));

	return 0;
}

static int
acrn_prepare_kernel(struct vmctx *ctx)
{
	FILE *fp;
	long len;
	size_t read;

	fp = fopen(kernel_path, "r");
	if (fp == NULL) {
		printf("SW_LOAD ERR: could not open kernel file %s\n",
				kernel_path);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);

	if (len != kernel_size) {
		fprintf(stderr,
			"SW_LOAD ERR: kernel file changed\n");
		fclose(fp);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	read = fread(ctx->baseaddr + KERNEL_LOAD_OFF(ctx),
			sizeof(char), len, fp);
	if (read < len) {
		printf("SW_LOAD ERR: could not read the whole kernel file,"
				" file len=%ld, read %lu\n", len, read);
		fclose(fp);
		return -1;
	}
	fclose(fp);
	printf("SW_LOAD: kernel %s size %lu copied to guest 0x%lx\n",
			kernel_path, kernel_size, KERNEL_LOAD_OFF(ctx));
	image_hdr = (void *)ctx->baseaddr + KERNEL_LOAD_OFF(ctx);

	return 0;
}

int
acrn_sw_load_image(struct vmctx *ctx)
{
	int ret;
	uint64_t load_offset, text_size;

	memset(&ctx->bsp_regs, 0, sizeof(struct acrn_vcpu_regs));
	ctx->bsp_regs.vcpu_id = 0;

	if (with_dtb) {
		ret = acrn_prepare_dtb(ctx);
		if (ret)
			return ret;
		ctx->bsp_regs.arm_regs.regs[0] = DTB_LOAD_OFF(ctx);
	}

	if (with_ramdisk) {
		ret = acrn_prepare_ramdisk(ctx);
		if (ret)
			return ret;
	}

	if (with_kernel) {
		ret = acrn_prepare_kernel(ctx);
		if (ret)
			return ret;

		load_offset = __le64_to_cpu(image_hdr->text_offset);
		text_size = __le64_to_cpu(image_hdr->image_size);

		ctx->bsp_regs.arm_regs.pc = ctx->lowmem_start + load_offset;

		memcpy(ctx->baseaddr + ctx->lowmem_start + load_offset, ctx->baseaddr + KERNEL_LOAD_OFF(ctx), text_size);
#if 0
		*((uint32_t *)(ctx->baseaddr + load_offset)) = 0x58000046;
		*((uint32_t *)(ctx->baseaddr + load_offset + 4)) = 0xf90000c0;
		*((uint32_t *)(ctx->baseaddr + load_offset + 8)) = 0x88889999;
#endif

		pr_info("SW_LOAD: kernel_entry_addr=0x%lx gpa[%lx] text_size[%lx]\n",
				ctx->baseaddr + ctx->lowmem_start + load_offset, ctx->lowmem_start + load_offset, text_size);
	}

	ctx->bsp_regs.arm_regs.pstate = PSR_GUEST64_INIT;
	ctx->bsp_regs.arm_regs.regs[1] = 0;
	ctx->bsp_regs.arm_regs.regs[2] = 0;
	ctx->bsp_regs.arm_regs.regs[3] = 0;
	return 0;
}


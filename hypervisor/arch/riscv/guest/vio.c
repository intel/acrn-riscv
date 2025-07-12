/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <errno.h>
#include <asm/lib/atomic.h>
#include <asm/apicreg.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/vm.h>
#include <asm/guest/vpci.h>
#include <asm/guest/instr_emul.h>
#include <asm/guest/vmexit.h>
#include <asm/vmx.h>
#include <asm/guest/s2vm.h>
#include <asm/pgtable.h>
#include <io_req.h>
#include <trace.h>
#include <logmsg.h>

void arch_fire_hsm_interrupt(void)
{
	struct acrn_vm *sos_vm;
	struct acrn_vcpu *vcpu;

	sos_vm = get_sos_vm();
	vcpu = vcpu_from_vid(sos_vm, BSP_CPU_ID);

	vplic_accept_intr(vcpu, get_hsm_notification_vector(), true);
}

/**
 * Dummy handler, riscv doesn't support pio.
 */
void
emulate_pio_complete(struct acrn_vcpu *vcpu, const struct io_request *io_req)
{
	pr_fatal("Wrong state, should not reach here!\n");
}

#ifdef CONFIG_MACRN
uint32_t get_instruction(uint64_t status, uint64_t gva, uint32_t *xlen)
{
	uint64_t m = 0xa0000;
	register uint32_t ins;
	uint64_t st;

	ASSERT((status & 0x1800) == 0x800);
	status |= m;
	local_irq_disable();
	st = cpu_csr_read(mstatus);
	asm volatile (
		"csrw mstatus, %[status] \n\t"
		"lw %[ins], (%[gva]) \n\t"
		"csrw mstatus, %[old] \n\t"
		: [ins] "=r"(ins)
		: [gva] "r"(gva), [status] "r"(status), [old] "r"(st)
		: "memory"
	);
	local_irq_enable();

	if ((ins & 0x3) != 0x3) {
		*xlen = 16;
		ins &= 0xffff;
	} else {
		*xlen = 32;
	}

	return ins;
}

static uint64_t get_gpa(uint64_t *vpn3, uint64_t gva)
{
	uint64_t gpa = INVALID_HPA;
	const uint64_t *pret = NULL;
	uint64_t pg_size;

	pret = lookup_address(vpn3, gva, &pg_size, &ppt_mem_ops);
	if (pret != NULL)
		gpa = (((*pret << 2) & (~PPT_PFN_HIGH_MASK)) & (~(pg_size - 1UL)))
			| (gva & (pg_size - 1UL));

	return gpa;
}
#else

static uint64_t get_gpa(uint64_t *vpn3, uint64_t gva)
{
	return gva;
}

uint32_t get_instruction(uint64_t status, uint64_t gva, uint32_t *xlen)
{
	return 0;
}
#endif

static bool need_pagetable_walk(uint64_t satp)
{
	return (satp & 0xF000000000000000UL) != 0;
}

int32_t mmio_access_vmexit_handler(struct acrn_vcpu *vcpu)
{
	int ret;
	int32_t status = -1;
	uint64_t exit_qual;
	uint64_t gva, gpa;
	uint32_t ins, xlen = 0;
	struct io_request *io_req = &vcpu->req;
	struct acrn_mmio_request *mmio_req = &io_req->reqs.mmio_request;
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	/* Handle page fault from guest */
	exit_qual = vcpu->arch.exit_qualification;
	ins = get_instruction(ctx->cpu_gp_regs.regs.status,
			      ctx->cpu_gp_regs.regs.ip, &xlen);
	gva = ctx->cpu_gp_regs.regs.tval;
	if (need_pagetable_walk(ctx->satp))
		gpa = get_gpa(satp_to_vpn3_page(ctx->satp), gva);
	else
		gpa = gva;

	io_req->io_type = ACRN_IOREQ_TYPE_MMIO;

	/* Specify if read or write operation */
	switch (exit_qual) {
	case HX_EXIT_PF_GUEST_STORE:
	case HX_EXIT_STORE_ACCESS:
		/* Write operation */
		mmio_req->direction = ACRN_IOREQ_DIR_WRITE;
		mmio_req->value = 0UL;
		break;
	case HX_EXIT_PF_GUEST_LOAD:
	case HX_EXIT_LOAD_ACCESS:
		/* Read operation */
		mmio_req->direction = ACRN_IOREQ_DIR_READ;
		break;
	default:
		status = -1;
		pr_acrnlog("unsupported access to address: 0x%016lx", gpa);
		return status;
		break;
	}

	mmio_req->address = gpa;
	ret = decode_instruction(vcpu, ins, xlen);
	if (ret > 0) {
		mmio_req->size = (uint64_t)ret;
		if (gpa == INVALID_HPA) {
			mmio_req->value = 0UL;
			emulate_instruction(vcpu);
			return 0;
		}

		/*
		 * For MMIO write, ask DM to run MMIO emulation after
		 * instruction emulation. For MMIO read, ask DM to run MMIO
		 * emulation at first.
		 */

		/* Determine value being written. */
		if (mmio_req->direction == ACRN_IOREQ_DIR_WRITE) {
			status = emulate_instruction(vcpu);
			if (status != 0) {
				ret = -EFAULT;
			}
		}

		if (ret > 0) {
			status = emulate_io(vcpu, io_req);
		}
	} else {
		if (ret == -EFAULT) {
			pr_info("page fault happen during decode_instruction");
			status = 0;
		}
	}
	if (ret <= 0) {
		//pr_acrnlog("Guest Linear Address: 0x%016lx", exec_vmread(VMX_GUEST_LINEAR_ADDR));
		pr_acrnlog("Guest Physical Address address: 0x%016lx", gpa);
	}

	return status;
}

/**
 * Dummy handler, riscv doesn't support pio.
 */
void allow_guest_pio_access(struct acrn_vm *vm, uint16_t port_address,
		uint32_t nbytes)
{
	pr_fatal("Wrong state, should not reach here!\n");
}

void deny_guest_pio_access(struct acrn_vm *vm, uint16_t port_address,
		uint32_t nbytes)
{
	pr_fatal("Wrong state, should not reach here!\n");
}

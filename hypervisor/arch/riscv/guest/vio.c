/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#include <types.h>
#include <errno.h>
#include <asm/atomic.h>
#include <io_req.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/vm.h>
#include <asm/guest/instr_emul.h>
#include <asm/guest/vmexit.h>
#include <asm/vmx.h>
#include <asm/guest/s2vm.h>
#include <asm/pgtable.h>
#include <trace.h>
#include <logmsg.h>

void arch_fire_vhm_interrupt(void)
{
	/*
	 * use vLAPIC to inject vector to SOS vcpu 0 if vclint is enabled
	 * otherwise, send IPI hardcoded to BSP_CPU_ID
	 */
	struct acrn_vm *sos_vm;
	struct acrn_vcpu *vcpu;

	sos_vm = get_sos_vm();
	vcpu = vcpu_from_vid(sos_vm, BSP_CPU_ID);

	vclint_set_intr(vcpu, get_vhm_notification_vector());
}

/**
 * Dummy handler, riscv doesn't support pio.
 */
void
emulate_pio_complete(struct acrn_vcpu *vcpu, const struct io_request *io_req)
{
	pr_fatal("Wrong state, should not reach here!\n");
}

int32_t s2pt_violation_vmexit_handler(struct acrn_vcpu *vcpu)
{
	int ret;
	int32_t status;
	uint64_t exit_qual;
	uint64_t gpa;
	struct io_request *io_req = &vcpu->req;
	struct mmio_request *mmio_req = &io_req->reqs.mmio;

	/* Handle page fault from guest */
	exit_qual = vcpu->arch.exit_qualification;

	if ((exit_qual & 0x4UL) != 0UL) {
	} else {

		io_req->io_type = REQ_MMIO;

		/* Specify if read or write operation */
		if ((exit_qual & 0x2UL) != 0UL) {
			/* Write operation */
			mmio_req->direction = REQUEST_WRITE;
			mmio_req->value = 0UL;

			/* XXX: write access while EPT perm RX -> WP */
			if ((exit_qual & 0x38UL) == 0x28UL) {
				io_req->io_type = REQ_WP;
			}
		} else {
			/* Read operation */
			mmio_req->direction = REQUEST_READ;

			/* TODO: Need to determine how sign extension is determined for
			 * reads
			 */
		}

		/* Adjust IPA appropriately and OR page offset to get full IPA of abort
		 */
		mmio_req->address = gpa;

		ret = decode_instruction(vcpu);
		if (ret > 0) {
			mmio_req->size = (uint64_t)ret;
			/*
			 * For MMIO write, ask DM to run MMIO emulation after
			 * instruction emulation. For MMIO read, ask DM to run MMIO
			 * emulation at first.
			 */

			/* Determine value being written. */
			if (mmio_req->direction == REQUEST_WRITE) {
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

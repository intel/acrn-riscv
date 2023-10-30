/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 *
 * this file contains vmcs operations which is vcpu related
 */

#include <types.h>
#include <asm/guest/vmcs.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/vm.h>
#include <asm/vmx.h>
#include <asm/csr.h>
#include <asm/current.h>
#include <asm/pgtable.h>
#include <acrn/percpu.h>
#include <acrn/kernel.h>
//#include <cpu_caps.h>
//#include <cpufeatures.h>
#include <asm/guest/vmexit.h>
#include <logmsg.h>

static void init_guest_state(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	vcpu_set_gpreg(vcpu, OFFSET_REG_A0, vcpu->vcpu_id);
	cpu_csr_write(vsstatus, ctx->run_ctx.sstatus);
	cpu_csr_write(vsepc, ctx->run_ctx.sepc);

	/* use magic # to check if vs-mode switching succeeds or not */
	//cpu_csr_write(vsepc, 0xaaaabbbb);
	cpu_csr_write(vsip, ctx->run_ctx.sip);
	cpu_csr_write(vsie, ctx->run_ctx.sie);
	cpu_csr_write(vstvec, ctx->run_ctx.stvec);
	cpu_csr_write(vsscratch, ctx->run_ctx.sscratch);
	cpu_csr_write(vstval, ctx->run_ctx.stval);
	cpu_csr_write(vscause, ctx->run_ctx.scause);
	cpu_csr_write(vsatp, ctx->run_ctx.satp);
}

static void init_host_state(void)
{
	uint64_t value64;

	pr_dbg("Initialize host state");
	pr_dbg("guest kernel entry: %lx\n", _vkernel);

	value64 = 0x200000080;
	cpu_csr_set(hstatus, value64);

	/* must set the SPP in order to enter into guest s-mode */
	value64 = 0x200000100;
	cpu_csr_set(sstatus, value64);
	value64 = (uint64_t)_vkernel;
	cpu_csr_write(sepc, value64);

	value64 = 0xb100;
	cpu_csr_write(hedeleg, value64);
}

DEFINE_PER_CPU(void *, vcpu_run);
/**
 * @pre vcpu != NULL
 */
void init_vmcs(struct acrn_vcpu *vcpu)
{
	void **vcpu_ptr = &get_cpu_var(vcpu_run);

	/* Log message */
	pr_dbg("Initializing VMCS");
	/* Initialize the Virtual Machine Control Structure (VMCS) */
	init_host_state();
	init_guest_state(vcpu);
	*vcpu_ptr = (void *)vcpu;
}

/**
 * @pre vcpu != NULL
 */
void load_vmcs(const struct acrn_vcpu *vcpu)
{
	void **vcpu_ptr = &get_cpu_var(vcpu_run);
	init_guest_state(vcpu);
	*vcpu_ptr = (void *)vcpu;
}

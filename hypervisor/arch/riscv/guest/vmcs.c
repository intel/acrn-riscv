/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <asm/guest/vmcs.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/vm.h>
#include <asm/vmx.h>
#include <asm/current.h>
#include <asm/pgtable.h>
#include <asm/per_cpu.h>
#include <asm/init.h>
//#include <cpu_caps.h>
//#include <cpufeatures.h>
#include <asm/guest/vcsr.h>
#include <asm/guest/vmexit.h>
#include <asm/guest/s2vm.h>
#include <logmsg.h>

#ifndef CONFIG_MACRN
static void init_guest_state(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	vcpu_set_gpreg(vcpu, OFFSET_REG_A0, vcpu->vcpu_id);
	cpu_csr_write(vsstatus, ctx->run_ctx.sstatus);
	cpu_csr_write(vsepc, ctx->run_ctx.sepc);
	cpu_csr_write(vsip, ctx->run_ctx.sip);
//	cpu_csr_write(hvip, (ctx->run_ctx.sip & 0x222) << 1);
	cpu_csr_write(vsie, ctx->run_ctx.sie);
//	cpu_csr_write(hie, (ctx->run_ctx.sie & 0x22) << 1);
	cpu_csr_write(vstvec, ctx->run_ctx.stvec);
	cpu_csr_write(vsscratch, ctx->run_ctx.sscratch);
	cpu_csr_write(vstval, ctx->run_ctx.stval);
	cpu_csr_write(vscause, ctx->run_ctx.scause);
	cpu_csr_write(vsatp, ctx->run_ctx.satp);
}

static void load_guest_state(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	cpu_csr_write(vsstatus, ctx->run_ctx.sstatus);
	cpu_csr_write(vsepc, ctx->run_ctx.sepc);
//	cpu_csr_write(vsip, ctx->run_ctx.sip);
	cpu_csr_write(hvip, (ctx->run_ctx.sip & 0x2) << 1);
	cpu_csr_write(vsie, ctx->run_ctx.sie);
//	cpu_csr_write(hie, (ctx->run_ctx.sie & 0x2) << 1);
	cpu_csr_write(vstvec, ctx->run_ctx.stvec);
	cpu_csr_write(vsscratch, ctx->run_ctx.sscratch);
	cpu_csr_write(vstval, ctx->run_ctx.stval);
	cpu_csr_write(vscause, ctx->run_ctx.scause);
	cpu_csr_write(vsatp, ctx->run_ctx.satp);
}

static void save_guest_state(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	ctx->run_ctx.sstatus = cpu_csr_read(vsstatus);
	ctx->run_ctx.sepc = cpu_csr_read(vsepc);
	ctx->run_ctx.sip = cpu_csr_read(vsip);
	ctx->run_ctx.sie = cpu_csr_read(vsie);
	ctx->run_ctx.stvec = cpu_csr_read(vstvec);
	ctx->run_ctx.sscratch = cpu_csr_read(vsscratch);
	ctx->run_ctx.stval = cpu_csr_read(vstval);
	ctx->run_ctx.scause = cpu_csr_read(vscause);
	ctx->run_ctx.satp = cpu_csr_read(vsatp);
}

static void load_host_state(struct acrn_vcpu *vcpu)
{
	uint64_t osatp;
	uint64_t s2pt_satp = vcpu->vm->arch_vm.s2pt_satp;

	osatp = cpu_csr_read(hgatp);
	if (osatp != s2pt_satp)
		s2pt_flush_guest(vcpu->vm);
}

static void init_host_state(struct acrn_vcpu *vcpu)
{
	uint64_t value64;
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	pr_dbg("Initialize host state");
	value64 = 0x200000180;
	cpu_csr_set(hstatus, value64);
	ctx->run_ctx.cpu_gp_regs.regs.hstatus = value64;

	/* must set the SPP in order to enter into guest s-mode */
	value64 = 0x2000C2100;
	cpu_csr_set(sstatus, value64);
	ctx->run_ctx.cpu_gp_regs.regs.status = value64;

	value64 = 0x444;
	cpu_csr_write(hideleg, value64);

	value64 = 0xf0bfff;
	cpu_csr_write(hedeleg, value64);

	value64 = 0xc000000000000000;
	cpu_csr_write(henvcfg, value64);

	value64 = 0x7;
	cpu_csr_write(hcounteren, value64);

	s2pt_flush_guest(vcpu->vm);
}

static inline void load_guest_pmp(struct acrn_vcpu *vcpu) {}
#else /* !CONFIG_MACRN */
static void init_guest_state(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	cpu_csr_write(sstatus, 0x2000C0000);
	cpu_csr_write(sepc, ctx->run_ctx.sepc);
	cpu_csr_write(sip, ctx->run_ctx.sip);
	cpu_csr_write(sie, ctx->run_ctx.sie);
	cpu_csr_write(stvec, ctx->run_ctx.stvec);
	cpu_csr_write(sscratch, ctx->run_ctx.sscratch);
	cpu_csr_write(stval, ctx->run_ctx.stval);
	cpu_csr_write(scause, ctx->run_ctx.scause);
	cpu_csr_write(satp, ctx->run_ctx.satp);
}

static void load_guest_state(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	cpu_csr_write(sstatus, ctx->run_ctx.sstatus);
	cpu_csr_write(sepc, ctx->run_ctx.sepc);
	if (is_service_vm(vcpu->vm))
		cpu_csr_set(mip, (ctx->run_ctx.sip & 0x22));
	else
		cpu_csr_set(mip, ctx->run_ctx.sip);
	cpu_csr_write(sie, ctx->run_ctx.sie);
	cpu_csr_write(stvec, ctx->run_ctx.stvec);
	cpu_csr_write(sscratch, ctx->run_ctx.sscratch);
	cpu_csr_write(stval, ctx->run_ctx.stval);
	cpu_csr_write(scause, ctx->run_ctx.scause);
	cpu_csr_write(satp, ctx->run_ctx.satp);
}

static void save_guest_state(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	ctx->run_ctx.sstatus = cpu_csr_read(sstatus);
	ctx->run_ctx.sepc = cpu_csr_read(sepc);
	ctx->run_ctx.sip = cpu_csr_read(sip);
	ctx->run_ctx.sie = cpu_csr_read(sie);
	ctx->run_ctx.stvec = cpu_csr_read(stvec);
	ctx->run_ctx.sscratch = cpu_csr_read(sscratch);
	ctx->run_ctx.stval = cpu_csr_read(stval);
	ctx->run_ctx.scause = cpu_csr_read(scause);
	ctx->run_ctx.satp = cpu_csr_read(satp);
}

#define load_host_state(vcpu) do {} while(0)

static void init_host_state(struct acrn_vcpu *vcpu)
{
	uint64_t value64;
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];

	pr_dbg("Initialize host state");

	/* must set the MPP in order to enter into guest s-mode */
	value64 = 0x200000800;
	cpu_csr_set(mstatus, value64);
	ctx->run_ctx.cpu_gp_regs.regs.status = value64;
	value64 = 0xf0b55f;
	cpu_csr_write(medeleg, value64);
}

static inline void sos_pmp_switch(void)
{
	int pmp_cfg = 0x0f08080f; // Disable PLIC/UART passthrough for SOS
	int pmp_addr0 = CONFIG_PLIC_BASE >> 2;
	int pmp_addr1 = CONFIG_UART_BASE >> 2;
	int pmp_addr2 = (CONFIG_UART_BASE + CONFIG_UART_SIZE) >> 2;
	int pmp_addr3 = 0xffffffff;

	asm volatile (
		"csrw pmpcfg0, %0 \n\t"
		"csrw pmpaddr0, %1 \n\t"
		"csrw pmpaddr1, %2 \n\t"
		"csrw pmpaddr2, %3 \n\t"
		"csrw pmpaddr3, %4 \n\t"
		::"r"(pmp_cfg), "r"(pmp_addr0), "r"(pmp_addr1), "r"(pmp_addr2), "r"(pmp_addr3)
	);
}

static inline void uos_pmp_switch(void)
{
	int pmp_cfg = 0x0f080f;

	/* pass-thru rtc to UOS since no vtrc yet */
	int pmp_addr0 = (CONFIG_RTC_BASE + CONFIG_RTC_SIZE) >> 2;
	int pmp_addr1 = 0x20000000;
	int pmp_addr2 = 0xffffffff;

	asm volatile (
		"csrw pmpcfg0, %0 \n\t"
		"csrw pmpaddr0, %1 \n\t"
		"csrw pmpaddr1, %2 \n\t"
		"csrw pmpaddr2, %3 \n\t"
		::"r"(pmp_cfg), "r"(pmp_addr0), "r"(pmp_addr1), "r"(pmp_addr2)
	);
}

void load_guest_pmp(struct acrn_vcpu *vcpu)
{
	if (is_service_vm(vcpu->vm))
		sos_pmp_switch();
	else
		uos_pmp_switch();
}
#endif /* CONFIG_MACRN */

/**
 * @pre vcpu != NULL
 */
void init_vmcs(struct acrn_vcpu *vcpu)
{
	void **vcpu_ptr = &get_cpu_var(vcpu_run);

	/* Log message */
	pr_dbg("Initializing VMCS");
	/* Initialize the Virtual Machine Control Structure (VMCS) */
	init_host_state(vcpu);
	init_guest_state(vcpu);
	*vcpu_ptr = (void *)vcpu;
}

/**
 * @pre vcpu != NULL
 */
void load_vmcs(struct acrn_vcpu *vcpu)
{
	void **vcpu_ptr = &get_cpu_var(vcpu_run);

	load_host_state(vcpu);
	load_guest_state(vcpu);
	load_guest_pmp(vcpu);
	*vcpu_ptr = (void *)vcpu;
}

void save_vmcs(struct acrn_vcpu *vcpu)
{
	save_guest_state(vcpu);
}

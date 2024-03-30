/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <errno.h>
#include <asm/lib/bits.h>
#include <asm/guest/vcpu.h>
#include <asm/vmx.h>
#include <logmsg.h>
#include <asm/per_cpu.h>
#include <asm/init.h>
#include <asm/guest/vm.h>
#include <asm/guest/virq.h>
#include <asm/guest/vmcs.h>
//#include <mmu.h>
//#include <schedule.h>
#include <sprintf.h>
#include <asm/irq.h>
#include <asm/current.h>

/* stack_frame is linked with the sequence of stack operation in arch_switch_to() */
struct stack_frame {
	uint64_t ip;
	uint64_t ra;
	uint64_t sp;
	uint64_t gp;
	uint64_t tp;
	uint64_t t0;
	uint64_t t1;
	uint64_t t2;
	uint64_t s0;
	uint64_t s1;
	uint64_t a0;
	uint64_t a1;
	uint64_t a2;
	uint64_t a3;
	uint64_t a4;
	uint64_t a5;
	uint64_t a6;
	uint64_t a7;
	uint64_t s2;
	uint64_t s3;
	uint64_t s4;
	uint64_t s5;
	uint64_t s6;
	uint64_t s7;
	uint64_t s8;
	uint64_t s9;
	uint64_t s10;
	uint64_t s11;
	uint64_t t3;
	uint64_t t4;
	uint64_t t5;
	uint64_t t6;
	uint64_t status;
	uint64_t tval;
	uint64_t cause;
	uint64_t hstatus;
	uint64_t orig_a0;
//	uint64_t magic;
};

uint64_t vcpu_get_gpreg(const struct acrn_vcpu *vcpu, uint32_t reg)
{
	const struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	return ctx->cpu_gp_regs.longs[reg];
}

void vcpu_set_gpreg(struct acrn_vcpu *vcpu, uint32_t reg, uint64_t val)
{
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	ctx->cpu_gp_regs.longs[reg] = val;
}

uint64_t vcpu_get_rip(struct acrn_vcpu *vcpu)
{
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	return ctx->cpu_gp_regs.regs.ip;
}

void vcpu_set_rip(struct acrn_vcpu *vcpu, uint64_t val)
{
	vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx.cpu_gp_regs.regs.ip = val;
}

uint64_t vcpu_get_sp(const struct acrn_vcpu *vcpu)
{
	const struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	return ctx->cpu_gp_regs.regs.sp;
}

void vcpu_set_sp(struct acrn_vcpu *vcpu, uint64_t val)
{
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	ctx->cpu_gp_regs.regs.sp = val;
	bitmap_set_lock(CPU_REG_SP, &vcpu->reg_updated);
}

uint64_t vcpu_get_status(struct acrn_vcpu *vcpu)
{
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	if (!bitmap_test(CPU_REG_STATUS, &vcpu->reg_updated) &&
		!bitmap_test_and_set_lock(CPU_REG_STATUS,
			&vcpu->reg_cached) && vcpu->launched) {
		ctx->sstatus = cpu_csr_read(vsstatus);
	}

	return ctx->sstatus;
}

void vcpu_set_status(struct acrn_vcpu *vcpu, uint64_t val)
{
	vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx.sstatus = val;
	bitmap_set_lock(CPU_REG_STATUS, &vcpu->reg_updated);
}

uint64_t vcpu_get_guest_csr(const struct acrn_vcpu *vcpu, uint32_t csr)
{
	uint32_t index = vcsr_get_guest_csr_index(csr);
	uint64_t val = 0UL;

	if (index < NUM_GUEST_CSRS) {
		val = vcpu->arch.guest_csrs[index];
	}

	return val;
}

void vcpu_set_guest_csr(struct acrn_vcpu *vcpu, uint32_t csr, uint64_t val)
{
	uint32_t index = vcsr_get_guest_csr_index(csr);

	if (index < NUM_GUEST_CSRS) {
		vcpu->arch.guest_csrs[index] = val;
	}
}

/*
 * Write the eoi_exit_bitmaps to VMCS fields
 */
void vcpu_set_vmcs_eoi_exit(const struct acrn_vcpu *vcpu)
{
	pr_dbg("%s", __func__);
}

/*
 * Set the eoi_exit_bitmap bit for specific vector
 * @pre vcpu != NULL && vector <= 255U
 */
void vcpu_set_eoi_exit_bitmap(struct acrn_vcpu *vcpu, uint32_t vector)
{
	pr_dbg("%s", __func__);

	if (!bitmap_test_and_set_lock((uint16_t)(vector & 0x3fU),
			&(vcpu->arch.eoi_exit_bitmap[(vector & 0xffU) >> 6U]))) {
		vcpu_make_request(vcpu, ACRN_REQUEST_EOI_EXIT_BITMAP_UPDATE);
	}
}

void vcpu_clear_eoi_exit_bitmap(struct acrn_vcpu *vcpu, uint32_t vector)
{
	pr_dbg("%s", __func__);

	if (bitmap_test_and_clear_lock((uint16_t)(vector & 0x3fU),
			&(vcpu->arch.eoi_exit_bitmap[(vector & 0xffU) >> 6U]))) {
		vcpu_make_request(vcpu, ACRN_REQUEST_EOI_EXIT_BITMAP_UPDATE);
	}
}

/*
 * Reset all eoi_exit_bitmaps
 */
void vcpu_reset_eoi_exit_bitmaps(struct acrn_vcpu *vcpu)
{
	pr_dbg("%s", __func__);

	(void)memset((void *)(vcpu->arch.eoi_exit_bitmap), 0U, sizeof(vcpu->arch.eoi_exit_bitmap));
	vcpu_make_request(vcpu, ACRN_REQUEST_EOI_EXIT_BITMAP_UPDATE);
}

static void reset_vcpu_gp_regs(struct acrn_vcpu *vcpu)
{
}

/* As a vcpu reset internal API, DO NOT touch any vcpu state transition in this function. */
static void vcpu_reset_internal(struct acrn_vcpu *vcpu, enum reset_mode mode)
{
	int32_t i;
	struct acrn_vclint *vclint;

	vcpu->launched = false;
	vcpu->arch.nr_sipi = 0U;

	vcpu->arch.exception_info.exception = VECTOR_INVALID;
	vcpu->arch.cur_context = NORMAL_WORLD;

	for (i = 0; i < NR_WORLD; i++) {
		(void)memset((void *)(&vcpu->arch.contexts[i]), 0U,
			sizeof(struct run_context));
	}

	vclint = vcpu_vclint(vcpu);
	vclint_reset(vclint, vclint_ops, mode);

	reset_vcpu_gp_regs(vcpu);

	for (i = 0; i < VCPU_EVENT_NUM; i++) {
		reset_event(&vcpu->events[i]);
	}
}

struct acrn_vcpu *get_running_vcpu(uint16_t pcpu_id)
{
	struct thread_object *curr = sched_get_current(pcpu_id);
	struct acrn_vcpu *vcpu = NULL;

	if ((curr != NULL) && (!is_idle_thread(curr))) {
		vcpu = container_of(curr, struct acrn_vcpu, thread_obj);
	}

	return vcpu;
}

struct acrn_vcpu *get_ever_run_vcpu(uint16_t pcpu_id)
{
	return (struct acrn_vcpu *)per_cpu(ever_run_vcpu, pcpu_id);
}

void set_vcpu_regs(struct acrn_vcpu *vcpu, struct cpu_regs *vcpu_gp_regs)
{
	struct ext_context *ectx;
	struct run_context *ctx;

	ectx = &(vcpu->arch.contexts[vcpu->arch.cur_context].ext_ctx);
	ctx = &(vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx);

	(void)memcpy_s((void *)&(ctx->cpu_gp_regs), sizeof(struct cpu_regs),
			(void *)&(vcpu_gp_regs), sizeof(struct cpu_regs));

	vcpu_set_rip(vcpu, vcpu_gp_regs->ip);
	vcpu_set_sp(vcpu, vcpu_gp_regs->sp);
	if (vcpu_gp_regs->status == 0UL) {
		vcpu_set_status(vcpu, 0x02UL);
	} else {
		vcpu_set_status(vcpu, vcpu_gp_regs->status & ~(0x8d5UL));
	}
}

void init_vcpu_protect_mode_regs(struct acrn_vcpu *vcpu, uint64_t vgdt_base_gpa)
{
}

void set_vcpu_startup_entry(struct acrn_vcpu *vcpu, uint64_t entry)
{
	struct ext_context *ectx;

	ectx = &(vcpu->arch.contexts[vcpu->arch.cur_context].ext_ctx);
	vcpu_set_rip(vcpu, 0UL);
}

struct acrn_vcpu *vclint_vcpu(struct acrn_vclint *vclint, uint32_t idx)
{
	return &(vclint->vm->hw.vcpu[idx]);
}

struct acrn_vclint *vcpu_vclint(struct acrn_vcpu *vcpu)
{
	return &(vcpu->vm->vclint);
}

struct acrn_vplic *vcpu_vplic(struct acrn_vcpu *vcpu)
{
	return &(vcpu->vm->vplic);
}
/*
 * @pre (&vcpu->stack[CONFIG_STACK_SIZE] & (CPU_STACK_ALIGN - 1UL)) == 0
 */
static uint64_t build_stack_frame(struct acrn_vcpu *vcpu)
{
	uint64_t stacktop = (uint64_t)&vcpu->stack[STACK_SIZE];
	struct stack_frame *frame;
	uint64_t *ret;

	frame = (struct stack_frame *)stacktop;
	frame -= 1;

//	frame->magic = SP_BOTTOM_MAGIC;
	frame->ip = (uint64_t)vcpu->thread_obj.thread_entry; /*return address*/
	frame->ra = (uint64_t)vcpu->thread_obj.thread_entry;
	//frame->ra = 0UL;
	frame->sp = (uint64_t)frame;
	frame->gp = 0UL;
	frame->tp = (uint64_t)&vcpu->thread_obj;
	frame->t0 = 0UL;
	frame->t1 = 0UL;
	frame->t2 = 0UL;
	frame->s0 = 0UL;
	frame->s1 = 0UL;
	frame->a0 = (uint64_t)&vcpu->thread_obj;
	frame->a1 = 0UL;
	frame->a2 = 0UL;
	frame->a3 = 0UL;
	frame->a4 = 0UL;
	frame->a5 = 0UL;
	frame->a6 = 0UL;
	frame->a7 = 0UL;
	frame->s2 = 0UL;
	frame->s3 = 0UL;
	frame->s4 = 0UL;
	frame->s5 = 0UL;
	frame->s6 = 0UL;
	frame->s7 = 0UL;
	frame->s8 = 0UL;
	frame->s9 = 0UL;
	frame->s10 = 0UL;
	frame->s11 = 0UL;
	frame->t3 = 0UL;
	frame->t4 = 0UL;
	frame->t5 = 0UL;
	frame->t6 = 0UL;

	ret = &frame->ip;

	return (uint64_t)ret;
}

uint64_t vcpu_get_efer(struct acrn_vcpu *vcpu)
{
	return 0;
}

uint64_t vcpu_get_cr0(struct acrn_vcpu *vcpu)
{
	return 0;
}

/*
 *  @pre vcpu != NULL
 */
int32_t run_vcpu(struct acrn_vcpu *vcpu)
{
	int32_t status = 0;
	const struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	/* If this VCPU is not already launched, launch it */
	if (!vcpu->launched) {
		pr_info("VM %d Starting VCPU %hu",
				vcpu->vm->vm_id, vcpu->vcpu_id);

		/* Set vcpu launched */
		vcpu->launched = true;

#ifdef CONFIG_L1D_FLUSH_VMENTRY_ENABLED
		cpu_l1d_flush();
#endif
		load_vmcs(vcpu);
		/* Launch the VM */
		status = vmx_vmrun(vcpu);
		save_vmcs(vcpu);

		/* See if VM launched successfully */
		if (status == 0) {
			if (is_vcpu_bsp(vcpu)) {
				pr_info("VM %d VCPU %hu successfully launched",
					vcpu->vm->vm_id, vcpu->vcpu_id);
			}
		}
	} else {
	#ifdef CONFIG_L1D_FLUSH_VMENTRY_ENABLED
		cpu_l1d_flush();
#endif

		load_vmcs(vcpu);
		/* Resume the VM */
		status = vmx_vmrun(vcpu);
		save_vmcs(vcpu);
	}

	vcpu->reg_cached = 0UL;

	/* Obtain current VCPU instruction length */
	vcpu->arch.inst_len = 64;

	/* Obtain VM exit reason */
	vcpu->arch.exit_reason = ctx->cpu_gp_regs.regs.cause;

	if (status != 0) {
		/* refer to 64-ia32 spec section 24.9.1 volume#3 */
		if ((vcpu->arch.exit_reason & HX_VMENTRY_FAIL) != 0U) {
			pr_fatal("vmentry fail reason=%lx", vcpu->arch.exit_reason);
		} else {
			pr_fatal("vmexit fail err_inst=%x", cpu_csr_read(sepc));
		}
	}

	return status;
}

/*
 *  @pre vcpu != NULL
 *  @pre vcpu->state == VCPU_ZOMBIE
 */
void offline_vcpu(struct acrn_vcpu *vcpu)
{
	vclint_free(vcpu);
	per_cpu(ever_run_vcpu, pcpuid_from_vcpu(vcpu)) = NULL;

	/* This operation must be atomic to avoid contention with posted interrupt handler */
	per_cpu(vcpu_array, pcpuid_from_vcpu(vcpu))[vcpu->vm->vm_id] = NULL;

	vcpu_set_state(vcpu, VCPU_OFFLINE);
}

void kick_vcpu(struct acrn_vcpu *vcpu)
{
	uint16_t pcpu_id = pcpuid_from_vcpu(vcpu);

	if ((get_pcpu_id() != pcpu_id) &&
		(per_cpu(vcpu_run, pcpu_id) == vcpu))
	{
		send_single_swi(pcpu_id, NOTIFY_VCPU_SWI);
	}
}

/* NOTE:
 * vcpu should be paused before call this function.
 * @pre vcpu != NULL
 * @pre vcpu->state == VCPU_ZOMBIE
 */
void reset_vcpu(struct acrn_vcpu *vcpu, enum reset_mode mode)
{
	pr_dbg("vcpu%hu reset", vcpu->vcpu_id);

	vcpu_reset_internal(vcpu, mode);
	vcpu_set_state(vcpu, VCPU_INIT);
}

void zombie_vcpu(struct acrn_vcpu *vcpu, enum vcpu_state new_state)
{
	enum vcpu_state prev_state;
	uint16_t pcpu_id = pcpuid_from_vcpu(vcpu);

	pr_dbg("vcpu%hu paused, new state: %d",	vcpu->vcpu_id, new_state);

	if (((vcpu->state == VCPU_RUNNING) || (vcpu->state == VCPU_INIT)) && (new_state == VCPU_ZOMBIE)) {
		prev_state = vcpu->state;
		vcpu_set_state(vcpu, new_state);

		if (prev_state == VCPU_RUNNING) {
			if (pcpu_id == get_pcpu_id()) {
				sleep_thread(&vcpu->thread_obj);
			} else {
				sleep_thread_sync(&vcpu->thread_obj);
			}
		}
	}
}

/* TODO:
 * Now we have switch_out and switch_in callbacks for each thread_object, and schedule
 * will call them every thread switch. We can implement lazy context swtich , which
 * only do context swtich when really need.
 */
static void context_switch_out(struct thread_object *prev)
{
	struct acrn_vcpu *vcpu = container_of(prev, struct acrn_vcpu, thread_obj);
	save_vmcs(vcpu);
}

static void context_switch_in(struct thread_object *next)
{
	struct acrn_vcpu *vcpu = container_of(next, struct acrn_vcpu, thread_obj);
	load_vmcs(vcpu);
}

/**
 * @pre vcpu != NULL
 * @pre vcpu->state == VCPU_INIT
 */
void launch_vcpu(struct acrn_vcpu *vcpu)
{
	uint16_t pcpu_id = vcpu->pcpu_id;

	pr_info("vcpu%hu scheduled on pcpu%hu", vcpu->vcpu_id, pcpu_id);
	vcpu_set_state(vcpu, VCPU_RUNNING);
	vcpu_set_gpreg(vcpu, CPU_REG_A0, pcpu_id);
	wake_thread(&vcpu->thread_obj);
}

/*
 * @pre vm != NULL && rtn_vcpu_handle != NULL
 */
int create_vcpu(struct acrn_vm *vm, uint16_t vcpu_id)
{
	struct acrn_vcpu *vcpu;
	int32_t i, ret;
	uint16_t pcpu_id;
	char thread_name[16];

	pcpu_id = vcpu_id + vm->vm_id * CONFIG_MAX_VCPU;

	/*
	 * vcpu->vcpu_id = vm->hw.created_vcpus;
	 * vm->hw.created_vcpus++;
	 */
	vcpu_id = vm->hw.created_vcpus;
	if (vcpu_id < MAX_VCPUS_PER_VM) {
		/* Allocate memory for VCPU */
		vcpu = &(vm->hw.vcpu[vcpu_id]);
		(void)memset((void *)vcpu, 0U, sizeof(struct acrn_vcpu));

		/* Initialize CPU ID for this VCPU */
		vcpu->vcpu_id = vcpu_id;
		vcpu->pcpu_id = pcpu_id;
		per_cpu(ever_run_vcpu, pcpu_id) = vcpu;

		/* Initialize the parent VM reference */
		vcpu->vm = vm;

		/* Initialize the virtual ID for this VCPU */
		/* FIXME:
		 * We have assumption that we always destroys vcpus in one
		 * shot (like when vm is destroyed). If we need to support
		 * specific vcpu destroy on fly, this vcpu_id assignment
		 * needs revise.
		 */

		pr_info("Create VM%d-VCPU%d, Role: %s",
				vcpu->vm->vm_id, vcpu->vcpu_id,
				is_vcpu_bsp(vcpu) ? "PRIMARY" : "SECONDARY");

		/*
		 * If the logical processor is in HX non-root operation and
		 * the "enable VPID" VM-execution control is 1, the current VPID
		 * is the value of the VPID VM-execution control field in the VMCS.
		 *
		 * This assignment guarantees a unique non-zero per vcpu vpid at runtime.
		 */
		vcpu->arch.vpid = 1U + (vm->vm_id * MAX_VCPUS_PER_VM) + vcpu->vcpu_id;

		/*
		 * ACRN uses the following approach to manage VT-d PI notification vectors:
		 * Allocate unique Activation Notification Vectors (ANV) for each vCPU that
		 * belongs to the same pCPU, the ANVs need only be unique within each pCPU,
		 * not across all vCPUs. The max numbers of vCPUs may be running on top of
		 * a pCPU is CONFIG_MAX_VM_NUM, since ACRN does not support 2 vCPUs of same
		 * VM running on top of same pCPU. This reduces # of pre-allocated ANVs for
		 * posted interrupts to CONFIG_MAX_VM_NUM, and enables ACRN to avoid switching
		 * between active and wake-up vector values in the posted interrupt desciptor
		 * on vCPU scheduling state changes.
		 *
		 * We maintain a per-pCPU array of vCPUs, and use vm_id as the index to the
		 * vCPU array
		 */
		per_cpu(vcpu_array, pcpu_id)[vm->vm_id] = vcpu;

		/* Populate the return handle */
		vcpu_set_state(vcpu, VCPU_INIT);
#ifdef CONFIG_KTEST
		vcpu_set_rip(vcpu, (uint64_t)_vboot);
#else
		vcpu_set_rip(vcpu, vm->sw.kernel_info.entry);
#endif
		(void)memset((void *)&vcpu->req, 0U, sizeof(struct io_request));
		vm->hw.created_vcpus++;

		snprintf(thread_name, 16U, "vm%hu:vcpu%hu", vm->vm_id, vcpu->vcpu_id);
		(void)strncpy_s(vcpu->thread_obj.name, 16U, thread_name, 16U);
		vcpu->thread_obj.sched_ctl = &per_cpu(sched_ctl, pcpu_id);
		vcpu->thread_obj.thread_entry = vcpu_thread;
		vcpu->thread_obj.pcpu_id = pcpu_id;
		/* vcpu->thread_obj.notify_mode is initialized in vcpu_reset_internal() when create vcpu */
		vcpu->thread_obj.host_sp = build_stack_frame(vcpu);
		vcpu->thread_obj.switch_out = context_switch_out;
		vcpu->thread_obj.switch_in = context_switch_in;
		init_thread_data(&vcpu->thread_obj, &get_vm_config(vm->vm_id)->sched_params);
		for (i = 0; i < VCPU_EVENT_NUM; i++) {
			init_event(&vcpu->events[i]);
		}

		vcpu_make_request(vcpu, ACRN_REQUEST_INIT_VMCS);
		ret = 0;
	} else {
		pr_err("%s, vcpu id is invalid!\n", __func__);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @pre vcpu != NULL
 */
uint16_t pcpuid_from_vcpu(const struct acrn_vcpu *vcpu)
{
	return sched_get_pcpuid(&vcpu->thread_obj);
}

uint64_t vcpumask2pcpumask(struct acrn_vm *vm, uint64_t vdmask)
{
	uint16_t vcpu_id;
	uint64_t dmask = 0UL;
	struct acrn_vcpu *vcpu;

	for (vcpu_id = 0U; vcpu_id < vm->hw.created_vcpus; vcpu_id++) {
		if ((vdmask & (1UL << vcpu_id)) != 0UL) {
			vcpu = vcpu_from_vid(vm, vcpu_id);
			bitmap_set_nolock(pcpuid_from_vcpu(vcpu), &dmask);
		}
	}

	return dmask;
}

/*
 * @brief Update the state of vCPU and state of vclint
 *
 * The vclint state of VM shall be updated for some vCPU
 * state update cases, such as from VCPU_INIT to VCPU_RUNNING.

 * @pre (vcpu != NULL)
 */
void vcpu_set_state(struct acrn_vcpu *vcpu, enum vcpu_state new_state)
{
	vcpu->state = new_state;
	update_vm_vclint_state(vcpu->vm);
}

bool is_lapic_pt_enabled(struct acrn_vcpu *vcpu)
{
	return false;
}

/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <errno.h>
#include <asm/lib/spinlock.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/vm.h>
#include <asm/guest/vmexit.h>
#include <asm/guest/virq.h>
#include <acrn_hv_defs.h>
#include <hypercall.h>
#include <trace.h>
#include <logmsg.h>
#include "sbi.h"

int32_t hcall_set_irqline(__unused struct acrn_vcpu *vcpu, struct acrn_vm *target_vm,
	__unused uint64_t param1, uint64_t param2)
{
	int32_t ret = -1;
	struct acrn_irqline_ops *ops = (struct acrn_irqline_ops *)&param2;

	if (!is_poweroff_vm(target_vm)) {
		if (ops->gsi < PLIC_NUM_SOURCES) {
			vplic_accept_intr(vcpu, ops->gsi, ops->op == GSI_SET_LOW);
			ret = 0;
		}
	}

	return ret;
}

static int32_t dispatch_sos_hypercall(struct acrn_vcpu *vcpu, uint64_t hypcall_id)
{
	struct acrn_vm *sos_vm = vcpu->vm;
	struct acrn_vm *target_vm;
	/* hypercall param1 from guest*/
	uint64_t param1 = vcpu_get_gpreg(vcpu, CPU_REG_A0);
	/* hypercall param2 from guest*/
	uint64_t param2 = vcpu_get_gpreg(vcpu, CPU_REG_A1);
	/* hypercall param1 is a relative vm id from SOS view */
	uint16_t relative_vm_id = (uint16_t)param1;
	uint16_t vm_id = rel_vmid_2_vmid(sos_vm->vm_id, relative_vm_id);
	int32_t ret = -1;

	if (vm_id < CONFIG_MAX_VM_NUM) {
		target_vm = get_vm_from_vmid(vm_id);
		if (hypcall_id == HC_CREATE_VM) {
			target_vm->vm_id = vm_id;
		}
	}

	switch (hypcall_id) {
	case HC_GET_API_VERSION:
		ret = hcall_get_api_version(vcpu, sos_vm, param1, param2);
		break;

	case HC_SET_CALLBACK_VECTOR:
		ret = hcall_set_callback_vector(vcpu, sos_vm, param1, param2);
		break;

	case HC_CREATE_VM:
		ret = hcall_create_vm(vcpu, sos_vm, param1, param2);
		break;

	case HC_DESTROY_VM:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_destroy_vm(vcpu, target_vm, param1, param2);
		}
		break;

	case HC_START_VM:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_start_vm(vcpu, target_vm, param1, param2);
		}
		break;

	case HC_RESET_VM:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_reset_vm(vcpu, target_vm, param1, param2);
		}
		break;

	case HC_PAUSE_VM:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_pause_vm(vcpu, target_vm, param1, param2);
		}
		break;

	case HC_CREATE_VCPU:
		ret = 0;
		break;

	case HC_SET_VCPU_REGS:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_set_vcpu_regs(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_SET_IRQLINE:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_set_irqline(vcpu, sos_vm, vm_id,
					(uint64_t)(struct acrn_irqline_ops *)&param2);
		}
		break;

	case HC_INJECT_MSI:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_inject_msi(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_SET_IOREQ_BUFFER:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_set_ioreq_buffer(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_NOTIFY_REQUEST_FINISH:
		/* param1: relative vmid to sos, vm_id: absolute vmid
		 * param2: vcpu_id */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_notify_ioreq_finish(vcpu, sos_vm, param1,
				(uint16_t)param2);
		}
		break;

	case HC_VM_SET_MEMORY_REGIONS:
		ret = hcall_set_vm_memory_regions(vcpu, sos_vm, param1, param2);
		break;

	case HC_VM_WRITE_PROTECT_PAGE:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_write_protect_page(vcpu, sos_vm, vm_id, param2);
		}
		break;

	/*
	 * Don't do MSI remapping and make the pmsi_data equal to vmsi_data
	 * This is a temporary solution before this hypercall is removed from SOS
	 */
	case HC_VM_PCI_MSIX_REMAP:
		ret = 0;
		break;

	case HC_VM_GPA2HPA:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if ((vm_id < CONFIG_MAX_VM_NUM) && !is_prelaunched_vm(get_vm_from_vmid(vm_id))) {
			ret = hcall_gpa_to_hpa(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_ASSIGN_PCIDEV:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_assign_pcidev(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_DEASSIGN_PCIDEV:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_deassign_pcidev(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_ASSIGN_MMIODEV:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_assign_mmiodev(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_DEASSIGN_MMIODEV:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
//			ret = hcall_deassign_mmiodev(sos_vm, vm_id, param2);
		}
		break;

	case HC_SET_PTDEV_INTR_INFO:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_set_ptdev_intr_info(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_RESET_PTDEV_INTR_INFO:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
			ret = hcall_reset_ptdev_intr_info(vcpu, sos_vm, vm_id, param2);
		}
		break;

	case HC_PM_GET_CPU_STATE:
//		ret = hcall_get_cpu_pm_state(vcpu, sos_vm, param1, param2);
		break;

	case HC_VM_INTR_MONITOR:
		/* param1: relative vmid to sos, vm_id: absolute vmid */
		if (is_valid_postlaunched_vmid(vm_id)) {
//			ret = hcall_vm_intr_monitor(sos_vm, vm_id, param2);
		}
		break;

	default:
	//	ret = hcall_debug(sos_vm, param1, param2, hypcall_id);
		break;
	}

	return ret;
}

#define VMCALL_TYPE_HYPERCALL 0x0A000000
/*
 * Pass return value to SOS by register rax.
 * This function should always return 0 since we shouldn't
 * deal with hypercall error in hypervisor.
 */
int32_t vmcall_vmexit_handler(struct acrn_vcpu *vcpu)
{
	int32_t ret;
	struct acrn_vm *vm = vcpu->vm;
	/* hypercall ID from guest*/
	uint64_t vmcall_type = vcpu_get_gpreg(vcpu, CPU_REG_A7);
	uint64_t hypcall_id;
	uint64_t pc = vcpu_get_gpreg(vcpu, CPU_REG_IP);

	vcpu_set_gpreg(vcpu, CPU_REG_IP, pc + 4);
	if (vmcall_type != VMCALL_TYPE_HYPERCALL)
		return sbi_ecall_handler(vcpu);

	hypcall_id = vcpu_get_gpreg(vcpu, CPU_REG_A6);
	
	if (hypcall_id == HC_WORLD_SWITCH ||
	    hypcall_id == HC_INITIALIZE_TRUSTY ||
	    hypcall_id == HC_SAVE_RESTORE_SWORLD_CTX) {
		pr_err ("hypcall 0x%lx not yet implemented\n", hypcall_id);
		 vcpu_inject_ud(vcpu);
		 ret = -ENODEV;
	} else if (is_service_vm(vm)) {
		/* Dispatch the hypercall handler */
		ret = dispatch_sos_hypercall(vcpu, hypcall_id);
	} else {
		pr_err("hypercall 0x%lx is only allowed from SOS_VM!\n", hypcall_id);
		vcpu_inject_ud(vcpu);
		ret = -ENODEV;
	}

	if ((ret != -EACCES) && (ret != -ENODEV)) {
		vcpu_set_gpreg(vcpu, CPU_REG_A0, (uint64_t)ret);
	}
//	TRACE_2L(TRACE_VMEXIT_VMCALL, vm->vm_id, hypcall_id);

	return 0;
}

/*-
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 * Copyright (c) 2023 Intel Corporation
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
 * $FreeBSD$
 */

#ifndef __RISCV_VCLINT_H__
#define __RISCV_VCLINT_H__

#include <asm/page.h>
#include <asm/timer.h>
#include <asm/apicreg.h>

/**
 * @file vclint.h
 *
 * @brief public APIs for virtual LAPIC
 */


#define VCLINT_MAXLVT_INDEX CLINT_LVT_MAX

struct vclint_timer {
	struct hv_timer timer;
	uint32_t mode;
	uint32_t tmicr;
	uint32_t divisor_shift;
};

struct acrn_vclint {
	/*
	 * Please keep 'apic_page' as the first field in
	 * current structure, as below alignment restrictions are mandatory
	 * to support APICv features:
	 * - 'apic_page' MUST be 4KB aligned.
	 * IRR, TMR and PIR could be accessed by other vCPUs when deliver
	 * an interrupt to vLAPIC.
	 */
	struct clint_regs	clint_page;

	struct vclint_timer	vtimer;

	uint64_t	clint_base;

	const struct acrn_vclint_ops *ops;

	/*
	 * Copies of some registers in the virtual APIC page. We do this for
	 * a couple of different reasons:
	 * - to be able to detect what changed (e.g. svr_last)
	 * - to maintain a coherent snapshot of the register (e.g. lvt_last)
	 */
	uint32_t	svr_last;
	uint32_t	lvt_last[VCLINT_MAXLVT_INDEX + 1];
} __aligned(PAGE_SIZE);


struct acrn_vcpu;
struct acrn_vclint_ops {
	void (*accept_intr)(struct acrn_vclint*vclint, uint32_t vector, bool level);
	void (*inject_intr)(struct acrn_vclint*vclint, bool guest_irq_enabled, bool injected);
	bool (*has_pending_delivery_intr)(struct acrn_vcpu *vcpu);
	bool (*has_pending_intr)(struct acrn_vcpu *vcpu);
	bool (*clint_read_access_may_valid)(uint32_t offset);
	bool (*clint_write_access_may_valid)(uint32_t offset);
};

enum reset_mode;
extern const struct acrn_apicv_ops *vclint_ops;
void vclint_set_vclint_ops(void);

/**
 * @brief virtual LAPIC
 *
 * @addtogroup acrn_vclintACRN vLAPIC
 * @{
 */

uint64_t vclint_get_tsc_deadline_csr(const struct acrn_vclint*vclint);
void vclint_set_tsc_deadline_csr(struct acrn_vclint*vclint, uint64_t val_arg);
uint64_t vclint_get_clintbase(const struct acrn_vclint*vclint);
int32_t vclint_set_clintbase(struct acrn_vclint*vclint, uint64_t new);

/*
 * Signals to the LAPIC that an interrupt at 'vector' needs to be generated
 * to the 'cpu', the state is recorded in IRR.
 *  @pre vcpu != NULL
 *  @pre vector <= 255U
 */
void vclint_set_intr(struct acrn_vcpu *vcpu, uint32_t vector);

/**
 * @brief Triggers LAPIC local interrupt(LVT).
 *
 * @param[in] vm           Pointer to VM data structure
 * @param[in] vcpu_id_arg  ID of vCPU, BROADCAST_CPU_ID means triggering
 *			   interrupt to all vCPUs.
 * @param[in] lvt_index    The index which LVT would to be fired.
 *
 * @retval 0 on success.
 * @retval -EINVAL on error that vcpu_id_arg or vector of the LVT is invalid.
 *
 * @pre vm != NULL
 */
int32_t vclint_set_local_intr(struct acrn_vm *vm, uint16_t vcpu_id_arg, uint32_t lvt_index);

/**
 * @brief Inject MSI to target VM.
 *
 * @param[in] vm   Pointer to VM data structure
 * @param[in] addr MSI address.
 * @param[in] msg  MSI data.
 *
 * @retval 0 on success.
 * @retval -1 on error that addr is invalid.
 *
 * @pre vm != NULL
 */
void vclint_receive_intr(struct acrn_vm *vm, bool level, uint32_t dest,
		bool phys, uint32_t delmode, uint32_t vec, bool rh);

void vclint_init(struct acrn_vm *vm);
/*
 *  @pre vcpu != NULL
 */
void vclint_free(struct acrn_vcpu *vcpu);

void vclint_reset(struct acrn_vclint*vclint, const struct acrn_apicv_ops *ops, enum reset_mode mode);
uint64_t vclint_get_clint_access_addr(void);
uint64_t vclint_get_clint_page_addr(struct acrn_vclint*vclint);
int32_t clint_access_vmexit_handler(struct acrn_vcpu *vcpu);
int32_t clint_write_vmexit_handler(struct acrn_vcpu *vcpu);
int32_t veoi_vmexit_handler(struct acrn_vcpu *vcpu);
void vclint_calc_dest(struct acrn_vm *vm, uint64_t *dmask, bool is_broadcast,
		uint32_t dest, bool phys, bool lowprio);

bool vclint_has_pending_intr(struct acrn_vcpu *vcpu);

/**
 * @}
 */
/* End of acrn_vclint*/
#endif /* __RISCV_VCLINT_H__ */

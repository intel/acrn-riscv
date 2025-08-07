/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_INSTR_EMUL_H__
#define __RISCV_INSTR_EMUL_H__

#include <types.h>
#include <asm/cpu.h>
#include <asm/guest/guest_memory.h>

#define TYPE_INST_READ		(0UL << 12U)
#define TYPE_INST_WRITE		(1UL << 12U)

struct acrn_vcpu;
struct instr_emul_vie_op {
	uint8_t		op_type;	/* type of operation (e.g. MOV) */
	uint16_t	op_flags;
};

#define VIE_PREFIX_SIZE	4U
#define VIE_INST_SIZE	15U
struct instr_emul_vie {
	uint8_t		inst[VIE_INST_SIZE];	/* instruction bytes */
	uint8_t		num_valid;		/* size of the instruction */
	uint8_t		num_processed;

	uint8_t		addrsize:4, opsize:4;	/* address and operand sizes */
	uint8_t		rex_w:1,		/* REX prefix */
			rex_r:1,
			rex_x:1,
			rex_b:1,
			rex_present:1,
			repz_present:1,		/* REP/REPE/REPZ prefix */
			repnz_present:1,	/* REPNE/REPNZ prefix */
			opsize_override:1,	/* Operand size override */
			addrsize_override:1,	/* Address size override */
			seg_override:1;	/* Segment override */

	uint8_t		mod:2,			/* ModRM byte */
			reg:4,
			rm:4;

	uint8_t		ss:2,			/* SIB byte */
			index:4,
			base:4;

	uint8_t		disp_bytes;
	uint8_t		imm_bytes;

	uint8_t		scale;
	enum cpu_reg_name base_register;		/* CPU_REG_xyz */
	enum cpu_reg_name index_register;	/* CPU_REG_xyz */
	enum cpu_reg_name segment_register;	/* CPU_REG_xyz */

	int64_t		displacement;		/* optional addr displacement */
	int64_t		immediate;		/* optional immediate operand */

	uint8_t		decoded;	/* set to 1 if successfully decoded */

	uint8_t		opcode;
	struct instr_emul_vie_op	op;			/* opcode description */

	uint64_t	dst_gpa;	/* saved dst operand gpa. Only for movs */
};

struct instr_emul_ctxt {
	struct instr_emul_vie vie;
};

extern uint32_t get_instruction(struct run_context *, uint32_t *xlen);
extern int32_t emulate_instruction(struct acrn_vcpu *vcpu);
extern int32_t decode_instruction(struct acrn_vcpu *vcpu, uint32_t ins,
				   uint32_t xlen);

#endif /* __RISCV_INSTR_EMUL_H__ */

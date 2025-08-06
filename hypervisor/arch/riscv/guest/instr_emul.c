/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <asm/guest/instr_emul.h>
#include <asm/guest/vcpu.h>

#define INS16_OPCODE_MASK	0xC000
#define INS16_OPRD_MASK		0x1C
#define INS16_OPRS2_MASK	0x1C

#define INS16_OPCODE_LQ		0x1
#define INS16_OPCODE_LW		0x2
#define INS16_OPCODE_LD		0x3
#define INS16_OPCODE_SQ		0x5
#define INS16_OPCODE_SW		0x6
#define INS16_OPCODE_SD		0x7

#define INS32_OPCODE_MASK	0x7F
#define INS32_OPRD_MASK		0xF80
#define INS32_OPRS1_MASK	0xF8000
#define INS32_OPRS2_MASK	0x1F00000
#define INS32_OPSIZE_MASK	0x7000

#define INS32_OPCODE_LD		0x3
#define INS32_OPCODE_ST		0x23
#define INS32_OPSIZE_BYTE	0x0
#define INS32_OPSIZE_HALF	0x1
#define INS32_OPSIZE_WORD	0x2
#define INS32_OPSIZE_DWORD	0x3

#define BIT64_MASK		0xffffffffffffffff
#define BIT32_MASK		0xffffffff
#define BIT16_MASK		0xffff
#define BIT8_MASK		0xff

static inline uint64_t get_mask(uint32_t size)
{
	uint64_t mask;

	switch (size) {
	case 1:
		mask = BIT8_MASK;
		break;
	case 2:
		mask = BIT16_MASK;
		break;
	case 4:
		mask = BIT32_MASK;
		break;
	case 8:
	default:
		mask = BIT64_MASK;
		break;
	}

	return mask;
}

#ifdef CONFIG_MACRN
static int32_t emulate_ins16(struct acrn_vcpu *vcpu, uint32_t ins, uint32_t size)
{
	struct acrn_mmio_request *mmio_req = &vcpu->req.reqs.mmio_request;
	uint64_t pc;
	int32_t reg, rc = 0;
	uint64_t mask = get_mask(size);

	pc = vcpu_get_gpreg(vcpu, CPU_REG_IP);
	vcpu_set_gpreg(vcpu, CPU_REG_IP, pc + 2);

	if ((INS16_OPCODE_MASK & ins) <= 0x8000) {
		reg = (ins & INS16_OPRD_MASK) >> 2;
		vcpu_set_gpreg(vcpu, reg + 0x8, (mmio_req->value) & mask);
	} else {
		reg = (ins & INS16_OPRS2_MASK) >> 2;
		mmio_req->value = vcpu_get_gpreg(vcpu, reg + 0x8) & mask;
	}

	return rc;
}

int32_t emulate_ins32(struct acrn_vcpu *vcpu, uint32_t ins, uint32_t size)
{
	struct acrn_mmio_request *mmio_req = &vcpu->req.reqs.mmio_request;
	uint64_t pc;
	int32_t reg, rc = 0;
	int64_t mask = get_mask(size);

	pc = vcpu_get_gpreg(vcpu, CPU_REG_IP);
	vcpu_set_gpreg(vcpu, CPU_REG_IP, pc + 4);

	if ((INS32_OPCODE_MASK & ins) == 0x3) {
		reg = (ins & INS32_OPRD_MASK) >> 7;
		vcpu_set_gpreg(vcpu, reg, (mmio_req->value) & mask);
	} else if ((INS32_OPCODE_MASK & ins) == 0x23) {
		reg = (ins & INS32_OPRS2_MASK) >> 20;
		mmio_req->value = vcpu_get_gpreg(vcpu, reg) & mask;
	} else {
		rc = -EFAULT;
	}

	return rc;
}

int32_t emulate_instruction(struct acrn_vcpu *vcpu)
{
	uint32_t ins, xlen, size;
	struct run_context *ctx;

	ctx = &vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;
	ins = get_instruction(ctx, &xlen);
	size = vcpu->req.reqs.mmio_request.size;

	if (xlen == 32)
		return emulate_ins32(vcpu, ins, size);
	else
		return emulate_ins16(vcpu, ins, size);
}

static int32_t decode_ins16(struct acrn_vcpu *vcpu, uint32_t ins)
{
	int size = (ins & INS16_OPCODE_MASK) >> 13;

	switch (size) {
	case INS16_OPCODE_LQ:
	case INS16_OPCODE_SQ:
		size = 16;
		break;
	case INS16_OPCODE_LW:
	case INS16_OPCODE_SW:
		size = 4;
		break;
	case INS16_OPCODE_LD:
	case INS16_OPCODE_SD:
	default:
		size = 8;
		break;
	}

	return size;
}

static int32_t decode_ins32(struct acrn_vcpu *vcpu, uint32_t ins)
{
	int size = (ins & INS32_OPSIZE_MASK) >> 12;

	switch (size) {
	case INS32_OPSIZE_BYTE:
		size = 1;
		break;
	case INS32_OPSIZE_HALF:
		size = 2;
		break;
	case INS32_OPSIZE_WORD:
		size = 4;
		break;
	case INS32_OPSIZE_DWORD:
	default:
		size = 8;
		break;
	}

	return size;
}

int32_t decode_instruction(struct acrn_vcpu *vcpu, uint32_t ins, uint32_t xlen)
{
	if (xlen == 32)
		return decode_ins32(vcpu, ins);
	else
		return decode_ins16(vcpu, ins);
}

#else /* !CONFIG_MACRN */

int32_t emulate_pseudo(struct acrn_vcpu *vcpu, uint32_t ins, uint32_t size)
{
	int32_t rc = 0;

	return rc;
}

int32_t emulate_ins32(struct acrn_vcpu *vcpu, uint32_t ins, uint32_t size)
{
	struct acrn_mmio_request *mmio_req = &vcpu->req.reqs.mmio_request;
	uint64_t pc;
	int32_t reg, rc = 0;
	int64_t mask = get_mask(size);
	int32_t xlen = (ins & 0x3) == 0x1 ? 2: 4;

	pc = vcpu_get_gpreg(vcpu, CPU_REG_IP);
	vcpu_set_gpreg(vcpu, CPU_REG_IP, pc + xlen);

	if ((INS32_OPCODE_MASK & ins) == 0x3) {
		reg = (ins & INS32_OPRD_MASK) >> 7;
		vcpu_set_gpreg(vcpu, reg, (mmio_req->value) & mask);
	} else if ((INS32_OPCODE_MASK & ins) == 0x23) {
		reg = (ins & INS32_OPRS2_MASK) >> 20;
		mmio_req->value = vcpu_get_gpreg(vcpu, reg) & mask;
	} else {
		rc = -EFAULT;
	}

	return rc;
}

int32_t emulate_instruction(struct acrn_vcpu *vcpu)
{
	uint32_t ins, xlen, size, ret = 0;
	struct run_context *ctx;

	ctx = &vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;
	ins = get_instruction(ctx, &xlen);
	size = vcpu->req.reqs.mmio_request.size;

	xlen = ins & 0x3;
	switch (xlen) {
		case 0x0:
			ret = emulate_pseudo(vcpu, ins, size);
			break;
		case 0x1:
		case 0x3:
			ret = emulate_ins32(vcpu, ins, size);
			break;
		default:
			break;
	}

	return ret;
}

static int32_t decode_ins32(struct acrn_vcpu *vcpu, uint32_t ins)
{
	int size = (ins & INS32_OPSIZE_MASK) >> 12;

	switch (size) {
	case INS32_OPSIZE_BYTE:
		size = 1;
		break;
	case INS32_OPSIZE_HALF:
		size = 2;
		break;
	case INS32_OPSIZE_WORD:
		size = 4;
		break;
	case INS32_OPSIZE_DWORD:
	default:
		size = 8;
		break;
	}

	return size;
}

static int32_t decode_pseudo(struct acrn_vcpu *vcpu, uint32_t ins)
{
	int size = (ins & INS32_OPSIZE_MASK) >> 12;

	switch (size) {
	case INS32_OPSIZE_WORD:
		size = 4;
		break;
	case INS32_OPSIZE_DWORD:
	default:
		size = 8;
		break;
	}

	return size;
}


int32_t decode_instruction(struct acrn_vcpu *vcpu, uint32_t ins, uint32_t xlen)
{
	int32_t ret = -1;

	xlen = ins & 0x3;
	switch (xlen) {
		case 0x0:
			ret = decode_pseudo(vcpu, ins);
			break;
		case 0x1:
		case 0x3:
			ret = decode_ins32(vcpu, ins);
			break;
		default:
			break;
	}

	return ret;
}
#endif

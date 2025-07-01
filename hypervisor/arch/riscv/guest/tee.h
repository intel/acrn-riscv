#ifndef __RISCV_TEE_H__
#define __RISCV_TEE_H__

int32_t tee_switch(struct acrn_vcpu *vcpu);
int32_t tee_answer_ree(struct acrn_vcpu *vcpu);

#endif

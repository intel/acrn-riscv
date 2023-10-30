#ifndef __RISCV_CPU_H__
#define __RISCV_CPU_H__

#ifndef __ASSEMBLY__
#include <acrn/types.h>
#include <vhm_ioctl_defs.h>
#include <asm/offset.h>

#define BSP_CPU_ID             0U
#define INVALID_CPU_ID 0xffffU
#define BROADCAST_CPU_ID 0xfffeU

#define	NEED_OFFLINE		(1U)
#define	NEED_SHUTDOWN_VM	(2U)

#define MAX_PSTATE	20U	/* max num of supported Px count */
#define MAX_CSTATE	8U	/* max num of supported Cx count */

enum cpu_reg_name {
	/* General purpose register layout should align with
	 * struct acrn_gp_regs
	 */
	CPU_REG_IP,
	CPU_REG_RA,
	CPU_REG_SP,
	CPU_REG_GP,
	CPU_REG_TP,
	CPU_REG_T0,
	CPU_REG_T1,
	CPU_REG_T2,
	CPU_REG_S0,
	CPU_REG_S1,
	CPU_REG_A0,
	CPU_REG_A1,
	CPU_REG_A2,
	CPU_REG_A3,
	CPU_REG_A4,
	CPU_REG_A5,
	CPU_REG_A6,
	CPU_REG_A7,
	CPU_REG_S2,
	CPU_REG_S3,
	CPU_REG_S4,
	CPU_REG_S5,
	CPU_REG_S6,
	CPU_REG_S7,
	CPU_REG_S8,
	CPU_REG_S9,
	CPU_REG_S10,
	CPU_REG_S11,
	CPU_REG_T3,
	CPU_REG_T4,
	CPU_REG_T5,
	CPU_REG_T6,
	CPU_REG_STATUS,
	CPU_REG_SCAUSE,
	CPU_REG_STVAL,
	CPU_REG_ORIG_A0,
	/*CPU_REG_LAST*/
};

struct cpu_regs {
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
	/* Supervisor/Machine CSRs */
	uint64_t status;
	uint64_t badaddr;
	uint64_t cause;
	/* a0 value before the syscall */
	uint64_t orig_a0;
};

#define OFFSET_REG_IP	offsetof(struct cpu_regs, ip)
#define OFFSET_REG_RA	offsetof(struct cpu_regs, ra)
#define OFFSET_REG_SP	offsetof(struct cpu_regs, sp)
#define OFFSET_REG_GP	offsetof(struct cpu_regs, gp)
#define OFFSET_REG_TP	offsetof(struct cpu_regs, tp)
#define OFFSET_REG_T0	offsetof(struct cpu_regs, t0)
#define OFFSET_REG_T1	offsetof(struct cpu_regs, t1)
#define OFFSET_REG_T2	offsetof(struct cpu_regs, t2)
#define OFFSET_REG_S0	offsetof(struct cpu_regs, s0)
#define OFFSET_REG_S1	offsetof(struct cpu_regs, s1)
#define OFFSET_REG_A0	offsetof(struct cpu_regs, a0)
#define OFFSET_REG_A1	offsetof(struct cpu_regs, a1)
#define OFFSET_REG_A2	offsetof(struct cpu_regs, a2)
#define OFFSET_REG_A3	offsetof(struct cpu_regs, a3)
#define OFFSET_REG_A4	offsetof(struct cpu_regs, a4)
#define OFFSET_REG_A5	offsetof(struct cpu_regs, a5)
#define OFFSET_REG_A6	offsetof(struct cpu_regs, a6)
#define OFFSET_REG_A7	offsetof(struct cpu_regs, a7)
#define OFFSET_REG_S2	offsetof(struct cpu_regs, s2)
#define OFFSET_REG_S3	offsetof(struct cpu_regs, s3)
#define OFFSET_REG_S4	offsetof(struct cpu_regs, s4)
#define OFFSET_REG_S5	offsetof(struct cpu_regs, s5)
#define OFFSET_REG_S6	offsetof(struct cpu_regs, s6)
#define OFFSET_REG_S7	offsetof(struct cpu_regs, s7)
#define OFFSET_REG_S8	offsetof(struct cpu_regs, s8)
#define OFFSET_REG_S9	offsetof(struct cpu_regs, s9)
#define OFFSET_REG_S10	offsetof(struct cpu_regs, s10)
#define OFFSET_REG_S11	offsetof(struct cpu_regs, s11)
#define OFFSET_REG_T3	offsetof(struct cpu_regs, t3)
#define OFFSET_REG_T4	offsetof(struct cpu_regs, t4)
#define OFFSET_REG_T5	offsetof(struct cpu_regs, t5)
#define OFFSET_REG_T6	offsetof(struct cpu_regs, t6)
	/*OFFSET_REG_LAST*/

/* Per-VCPU state that lives at the top of the stack */
struct cpu_info {
    struct cpu_regs guest_cpu_ctx_regs;
    unsigned long elr;
    uint32_t flags;
};

void cpu_dead(void);
void cpu_do_idle(void);

#define cpu_relax() barrier() /* Could yield? */

#define ASM_STR(x) #x

/* Read CSR */
#define cpu_csr_read(reg)                                           \
({                                                                  \
	uint64_t v;                                                 \
	asm volatile (" csrr %0, " ASM_STR(reg) "\n\t"              \
			:"=r" (v):);                                \
	v;                                                          \
})

/* Write CSR */
#define cpu_csr_write(reg, csr_val)                                 \
({                                                                  \
	uint64_t val = (uint64_t)csr_val;			    \
	asm volatile (" csrw " ASM_STR(reg) ", %0 \n\t"             \
			:: "r"(val));         	                    \
})

/* Set CSR */
#define cpu_csr_set(reg, csr_val)                                 \
({                                                                  \
	uint64_t val = (uint64_t)csr_val;			    \
	asm volatile (" csrs " ASM_STR(reg) ", %0 \n\t"             \
			:: "r"(val));         	                    \
})


static inline void asm_pause(void)
{
	asm volatile ("fence; nop");
}

static inline void asm_hlt(void)
{
}

#define cpu_enable_irq()                         		\
{                                                           	\
	asm volatile ("li t0, 0x2 \n"                       	\
		      "csrs sstatus, t0 \n" : : : "cc");    	\
}

#define CPU_IRQ_ENABLE cpu_enable_irq

#define cpu_disable_irq()                         		\
{                                                           	\
	asm volatile ("li t0, 0x2 \n"                       	\
		      "csrc sstatus, t0 \n" : : : "cc");    	\
}

#define CPU_IRQ_DISABLE cpu_disable_irq

/*
 * extended context does not save/restore during vm exit/entry, it's mainly
 * used in trusty world switch
 */
struct ext_context {
	uint64_t tsc_offset;
};

#define NUM_GPRS                            16U
/**
 * @brief registers info saved for vcpu running context
 */
struct run_context {
/* Contains the guest register set.
 * NOTE: This must be the first element in the structure, so that the offsets
 * in vmx_asm.S match
 */
	union cpu_regs_t {
		struct cpu_regs regs;
		uint64_t longs[NUM_GPRS];
	} cpu_gp_regs;

	uint64_t sstatus;
	uint64_t sepc;
	uint64_t sip;
	uint64_t sie;
	uint64_t stvec;
	uint64_t sscratch;
	uint64_t stval;
	uint64_t scause;
	uint64_t satp;
};


struct cpu_context {
	struct run_context run_ctx;
	struct ext_context ext_ctx;
};

/*
 * Entries in the Interrupt Descriptor Table (IDT)
 */
#define IDT_DE      0U   /* #DE: Divide Error */
#define IDT_DB      1U   /* #DB: Debug */
#define IDT_NMI     2U   /* Nonmaskable External Interrupt */
#define IDT_BP      3U   /* #BP: Breakpoint */
#define IDT_OF      4U   /* #OF: Overflow */
#define IDT_BR      5U   /* #BR: Bound Range Exceeded */
#define IDT_UD      6U   /* #UD: Undefined/Invalid Opcode */
#define IDT_NM      7U   /* #NM: No Math Coprocessor */
#define IDT_DF      8U   /* #DF: Double Fault */
#define IDT_FPUGP   9U   /* Coprocessor Segment Overrun */
#define IDT_TS      10U  /* #TS: Invalid TSS */
#define IDT_NP      11U  /* #NP: Segment Not Present */
#define IDT_SS      12U  /* #SS: Stack Segment Fault */
#define IDT_GP      13U  /* #GP: General Protection Fault */
#define IDT_PF      14U  /* #PF: Page Fault */
#define IDT_MF      16U  /* #MF: FPU Floating-Point Error */
#define IDT_AC      17U  /* #AC: Alignment Check */
#define IDT_MC      18U  /* #MC: Machine Check */
#define IDT_XF      19U  /* #XF: SIMD Floating-Point Exception */
#define IDT_VE      20U  /* #VE: Virtualization Exception */

int get_pcpu_nums(void);

/*
 * stac/clac pair is used to access guest's memory protected by SUM,
 * following below flow:
 *
 *	stac();
 *	#access guest's memory.
 *	clac();
 *
 * Notes:Avoid inserting another stac/clac pair between stac and clac,
 *	As once clac after multiple stac will invalidate SUM protection
 *	and hence Page Fault crash.
 *	Logging message to memory buffer will induce this case,
 *	please disable SMAP temporlly or don't log messages to shared
 *	memory buffer, if it is evitable for you for debug purpose.
 */
static inline void stac(void)
{
	asm volatile ("nop" : : : "memory");
}

static inline void clac(void)
{
	asm volatile ("nop" : : : "memory");
}

#else /* __ASSEMBLY__ */

#include <asm/offset.h>
	.macro cpu_mctx_save
	sd sp, -0x108(sp)
	sd t0, -0xF0(sp)
	#li t0, 0x118
	addi sp, sp, -0x118
	#sub sp, sp, t0
	csrr t0, mepc
	sd t0, REG_EPC(sp)
	sd ra, REG_RA(sp)
	sd gp, REG_GP(sp)
	sd tp, REG_TP(sp)
	sd t1, REG_T1(sp)
	sd t2, REG_T2(sp)
	sd s0, REG_S0(sp)
	sd s1, REG_S1(sp)
	sd a0, REG_A0(sp)
	sd a1, REG_A1(sp)
	sd a2, REG_A2(sp)
	sd a3, REG_A3(sp)
	sd a4, REG_A4(sp)
	sd a5, REG_A5(sp)
	sd a6, REG_A6(sp)
	sd a7, REG_A7(sp)
	sd s2, REG_S2(sp)
	sd s3, REG_S3(sp)
	sd s4, REG_S4(sp)
	sd s5, REG_S5(sp)
	sd s6, REG_S6(sp)
	sd s7, REG_S7(sp)
	sd s8, REG_S8(sp)
	sd s9, REG_S9(sp)
	sd s10, REG_S10(sp)
	sd s11, REG_S11(sp)
	sd t3, REG_T3(sp)
	sd t4, REG_T4(sp)
	sd t5, REG_T5(sp)
	sd t6, REG_T6(sp)

	/* M-mode CSRs */
/*
	csrr t0, mstatus
	sd t0, REG_STATUS(sp)
	csrr t0, mtval
	sd t0, REG_TVAL(sp)
	csrr t0, mcause
	sd t0, REG_CAUSE(sp)
*/

	/* a0 value before the syscall */
	/*uint64_t orig_a0;*/
	.endm

	.macro cpu_mctx_restore
	/* M-mode CSRs */
/*
	ld t0, REG_STATUS(sp)
	csrw mstatus, t0
	ld t0, REG_TVAL(sp)
	csrw mtval, t0
	ld t0, REG_CAUSE(sp)
	csrw mcause, t0
*/
	/* a0 value bfore the syscall */
	/*uint64_t orig_a0*/

	ld t0, REG_EPC(sp)
	csrw mepc, t0
	ld ra, REG_RA(sp)
	ld gp, REG_GP(sp)
	ld tp, REG_TP(sp)
	ld t0, REG_T0(sp)
	ld t1, REG_T1(sp)
	ld t2, REG_T2(sp)
	ld s0, REG_S0(sp)
	ld s1, REG_S1(sp)
	ld a0, REG_A0(sp)
	ld a1, REG_A1(sp)
	ld a2, REG_A2(sp)
	ld a3, REG_A3(sp)
	ld a4, REG_A4(sp)
	ld a5, REG_A5(sp)
	ld a6, REG_A6(sp)
	ld a7, REG_A7(sp)
	ld s2, REG_S2(sp)
	ld s3, REG_S3(sp)
	ld s4, REG_S4(sp)
	ld s5, REG_S5(sp)
	ld s6, REG_S6(sp)
	ld s7, REG_S7(sp)
	ld s8, REG_S8(sp)
	ld s9, REG_S9(sp)
	ld s10, REG_S10(sp)
	ld s11, REG_S11(sp)
	ld t3, REG_T3(sp)
	ld t4, REG_T4(sp)
	ld t5, REG_T5(sp)
	ld t6, REG_T6(sp)
	ld sp, REG_SP(sp)
	.endm


	.macro cpu_disable_irq
	csrci sstatus, 0x2
	.endm

	.macro cpu_enable_irq
	csrsi sstatus, 0x2
	.endm

	.macro cpu_ctx_save
	sd sp, -0x108(sp)
	sd t0, -0xF0(sp)
	#li t0, 0x118
	addi sp, sp, -0x118
	#sub sp, sp, t0
	csrr t0, sepc
	sd t0, REG_EPC(sp)
	sd ra, REG_RA(sp)
	sd gp, REG_GP(sp)
	sd tp, REG_TP(sp)
	sd t1, REG_T1(sp)
	sd t2, REG_T2(sp)
	sd s0, REG_S0(sp)
	sd s1, REG_S1(sp)
	sd a0, REG_A0(sp)
	sd a1, REG_A1(sp)
	sd a2, REG_A2(sp)
	sd a3, REG_A3(sp)
	sd a4, REG_A4(sp)
	sd a5, REG_A5(sp)
	sd a6, REG_A6(sp)
	sd a7, REG_A7(sp)
	sd s2, REG_S2(sp)
	sd s3, REG_S3(sp)
	sd s4, REG_S4(sp)
	sd s5, REG_S5(sp)
	sd s6, REG_S6(sp)
	sd s7, REG_S7(sp)
	sd s8, REG_S8(sp)
	sd s9, REG_S9(sp)
	sd s10, REG_S10(sp)
	sd s11, REG_S11(sp)
	sd t3, REG_T3(sp)
	sd t4, REG_T4(sp)
	sd t5, REG_T5(sp)
	sd t6, REG_T6(sp)
	/* Supervisor CSRs */
/*
	csrr t0, sstatus
	sd t0, REG_STATUS(sp)
	csrr t0, stval
	sd t0, REG_TVAL(sp)
	csrr t0, scause
	sd t0, REG_CAUSE(sp)
*/
	/* a0 value before the syscall */
	/*uint64_t orig_a0;*/
	.endm

	.macro cpu_ctx_restore
	/* Supervisor CSRs */
/*
	ld t0, REG_STATUS(sp)
	csrw sstatus, t0
	ld t0, REG_TVAL(sp)
	csrw stval, t0
	ld t0, REG_CAUSE(sp)
	csrw scause, t0
*/
	/* a0 value bfore the syscall */
	/*uint64_t orig_a0*/

	ld t0, REG_EPC(sp)
	csrw sepc, t0
	ld ra, REG_RA(sp)
	ld gp, REG_GP(sp)
	ld tp, REG_TP(sp)
	ld t0, REG_T0(sp)
	ld t1, REG_T1(sp)
	ld t2, REG_T2(sp)
	ld s0, REG_S0(sp)
	ld s1, REG_S1(sp)
	ld a0, REG_A0(sp)
	ld a1, REG_A1(sp)
	ld a2, REG_A2(sp)
	ld a3, REG_A3(sp)
	ld a4, REG_A4(sp)
	ld a5, REG_A5(sp)
	ld a6, REG_A6(sp)
	ld a7, REG_A7(sp)
	ld s2, REG_S2(sp)
	ld s3, REG_S3(sp)
	ld s4, REG_S4(sp)
	ld s5, REG_S5(sp)
	ld s6, REG_S6(sp)
	ld s7, REG_S7(sp)
	ld s8, REG_S8(sp)
	ld s9, REG_S9(sp)
	ld s10, REG_S10(sp)
	ld s11, REG_S11(sp)
	ld t3, REG_T3(sp)
	ld t4, REG_T4(sp)
	ld t5, REG_T5(sp)
	ld t6, REG_T6(sp)
	.endm
#endif /* __ASSEMBLY__ */

#endif

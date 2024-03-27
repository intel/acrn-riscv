/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm/guest/vm.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/s2vm.h>
#include <debug/logmsg.h>
//#include <asm/guest_memory.h>
#include <asm/page.h>
#include <lib/errno.h>
//#include <asm/s2mm.h>
#include <asm/board.h>
#include <acrn/config.h>
//#include <asm/sched.h>
#include <irq.h>
#include <asm/board.h>
#include <asm/current.h>
#include <asm/image.h>

static struct acrn_vm vm_array[CONFIG_MAX_VM_NUM] __aligned(PAGE_SIZE);
struct acrn_vm_config vm_configs;
//struct acrn_vm acrn_sos_vm;
//struct acrn_vm acrn_uos_vm;
//struct acrn_vm *sos_vm = &acrn_sos_vm;
//struct acrn_vm *uos_vm = &acrn_uos_vm;
struct acrn_vm *sos_vm = &vm_array[0];
struct acrn_vm *uos_vm = &vm_array[1];

#define ZIMAGE64_MAGIC_V0 0x14000008
#define ZIMAGE64_MAGIC_V1 0x644d5241 /* "ARM\x64" */

#define MASK_2M 0xFFFFFFFFFFE00000

/**
 * @pre vm != NULL
 */
bool is_poweroff_vm(const struct acrn_vm *vm)
{
	return (vm->state == VM_POWERED_OFF);
}

/**
 * @pre vm != NULL
 */
bool is_created_vm(const struct acrn_vm *vm)
{
	return (vm->state == VM_CREATED);
}

/**
 * @pre vm != NULL
 */
bool is_paused_vm(const struct acrn_vm *vm)
{
	return (vm->state == VM_PAUSED);
}

bool is_service_vm(const struct acrn_vm *vm)
{
	return (vm != NULL) && (vm == sos_vm);
}

/* TODO: */
void get_vm_lock(struct acrn_vm *vm) { }
void put_vm_lock(struct acrn_vm *vm) { }

static void kernel_load(struct kernel_info *info)
{
	paddr_t load_addr;
	paddr_t paddr = info->kernel_addr;
	paddr_t len = info->kernel_len;
	void *kernel_hva;
	int rc;

	//load_addr = info->mem_start_gpa + info->text_offset;
	load_addr = info->text_offset;
	info->entry = load_addr;

	pr_info("Loading kernel from %lx to %lx - %lx",
		   paddr, load_addr, load_addr + len);

	kernel_hva = hpa2hva(paddr);

	rc = copy_to_gpa(sos_vm, kernel_hva, load_addr, len);
	if ( rc != 0 )
		pr_err("Unable to copy the kernel in the memory\n");
}


static int kernel_header_parse(struct kernel_info *info)
{
#if 0
	/* linux/Documentation/arm64/booting.txt */
	struct {
		uint32_t magic0;
		uint32_t res0;
		uint64_t text_offset;  /* Image load offset */
		uint64_t res1;
		uint64_t res2;
		/* zImage V1 only from here */
		uint64_t res3;
		uint64_t res4;
		uint64_t res5;
		uint32_t magic1;
		uint32_t res6;
	} zimage;
	void *addr = hpa2hva(info->kernel_addr);

	memcpy_s(&zimage, sizeof(zimage), addr, sizeof(zimage));

	if ( zimage.magic0 != ZIMAGE64_MAGIC_V0 &&
		 zimage.magic1 != ZIMAGE64_MAGIC_V1 )
		return -EINVAL;

	/*
	 * Given the above this check is a bit pointless, but leave it
	 * here in case someone adds a length field in the future.
	 */

	info->text_offset = zimage.text_offset;
	pr_info("zimage addr %lx text_offset %x", addr, info->text_offset);
#endif
	info->text_offset = (paddr_t)_boot;
	return 0;
}

static void init_vm_sw_load(struct acrn_vm *vm)
{
	struct kernel_info *kinfo = &vm->sw.kernel_info;
	struct dtb_info *dinfo = &vm->sw.dtb_info;

	// get kernel image load address and size.
	get_kernel_info(kinfo);
	pr_info("kernel addr =%lx kernel size =%lx", kinfo->kernel_addr, kinfo->kernel_len);
	// get dtb image load address and size.
	get_dtb_info(dinfo);
	kernel_header_parse(kinfo);
}

void prepare_sos_vm(void)
{
	sos_vm->vm_id = 0;
	// init stage 2 memory operations.
	init_s2pt_mem_ops(&sos_vm->arch_vm.s2pt_mem_ops, sos_vm->vm_id);
	sos_vm->arch_vm.s2ptp = sos_vm->arch_vm.s2pt_mem_ops.get_pml4_page(sos_vm->arch_vm.s2pt_mem_ops.info);
	pr_info("%s stage 2 transation table location: 0x%lx ", __func__, (uint64_t *)(sos_vm->arch_vm.s2ptp));
	init_vm_sw_load(sos_vm);
}

/*
 * FIXME: Need to support more UOS in future, hard code single uos for
 * now.
 */
void prepare_uos_vm(void)
{
	uos_vm->vm_id = 1;
	// init stage 2 memory operations.
	init_s2pt_mem_ops(&uos_vm->arch_vm.s2pt_mem_ops, uos_vm->vm_id);
	uos_vm->arch_vm.s2ptp = uos_vm->arch_vm.s2pt_mem_ops.get_pml4_page(uos_vm->arch_vm.s2pt_mem_ops.info);
	pr_info("%s stage 2 transation table location: 0x%lx ", __func__, (uint64_t *)(uos_vm->arch_vm.s2ptp));
}

static void dtb_load(struct dtb_info *info)
{
	void *dtb_hva = hpa2hva(info->dtb_addr);
	int rc = 0;

	pr_info("Loading DTB to 0x%llx - 0x%llx\n",
			info->dtb_start_gpa, info->dtb_start_gpa + info->dtb_size_gpa);

	return;

	pr_info("DTB dump: %x %x %x ", *(uint64_t *)dtb_hva, *(uint64_t *)(dtb_hva +8), *(uint64_t *)(dtb_hva+16) );
	rc = copy_to_gpa(sos_vm, (void *)dtb_hva, info->dtb_start_gpa, info->dtb_size_gpa);
	if ( rc != 0 )
		pr_err("Unable to copy the kernel in the memory\n");
}

static void allocate_guest_memory(struct acrn_vm *vm, struct kernel_info *info)
{
	uint64_t gpa = info->mem_start_gpa;
	uint64_t hpa = gpa;
	s2pt_add_mr(vm, vm->arch_vm.s2ptp, hpa, gpa, info->mem_size_gpa, PAGE_V | PAGE_RW_RW | PAGE_X);
}

static int map_irq_to_vm(struct acrn_vm *vm, unsigned int irq)
{
//	int res;

	/*
		 * Checking the return of vgic_reserve_virq is not
		 * necessary. It should not fail except when we try to map
		 * the IRQ twice. This can legitimately happen if the IRQ is shared
		 */
/*
	vgic_reserve_virq(vm, irq);
	res = route_irq_to_guest(vm, irq, irq);
	if (res < 0)
	{
		pr_err("Unable to map IRQ%" PRId32 " to vm%d\n",
			   irq, vm->vm_id);
		return res;
	}
*/
	return 0;
}

static void passthru_devices_to_sos(void)
{
	// Map all the devices to guest 0x8000000 - 0xb000000
	s2pt_add_mr(sos_vm, sos_vm->arch_vm.s2ptp, SOS_DEVICE_MMIO_START, SOS_DEVICE_MMIO_START,
			SOS_DEVICE_MMIO_SIZE, PAGE_V);
	s2pt_del_mr(sos_vm, sos_vm->arch_vm.s2ptp, CONFIG_CLINT_BASE, CONFIG_CLINT_SIZE);

	for (int irq = 32; irq < 992; irq++) {
		map_irq_to_vm(sos_vm, irq);
	}
}


int create_vm(struct acrn_vm *vm)
{
	struct guest_cpu_context *ctx;
	struct acrn_vcpu *vcpu;
	struct kernel_info *kinfo= &vm->sw.kernel_info;
	struct dtb_info *dinfo= &vm->sw.dtb_info;
	int ret, i;

	/* TODO: only support one SOS and one UOS now */
	if (is_service_vm(vm)) {
		kinfo->mem_start_gpa = CONFIG_SOS_MEM_START;
		kinfo->mem_size_gpa = CONFIG_SOS_MEM_SIZE; //256M
		dinfo->dtb_start_gpa = CONFIG_SOS_DTB_BASE; //membase + 128M
		dinfo->dtb_size_gpa = CONFIG_SOS_DTB_SIZE; // 2M
	} else {
		kinfo->mem_start_gpa = CONFIG_UOS_MEM_START;
		kinfo->mem_size_gpa = CONFIG_UOS_MEM_SIZE; //256M
	}
	pr_info("init stage 2 translation table");
	s2pt_init(vm);

	pr_info("allocate memory for guest");
	spinlock_init(&vm->emul_mmio_lock);

	if (is_service_vm(vm)) {
		allocate_guest_memory(vm, kinfo);
		pr_info("load kernel and dtb");
		kernel_load(kinfo);
		dtb_load(dinfo);
 
		pr_info("passthru devices");
		passthru_devices_to_sos();
	}

	vclint_init(vm);
	for (i = 0 ; i < CONFIG_MAX_VCPU; /*vm->max_vcpu*/ i++) {
		ret = create_vcpu(vm, i);
		pr_info("create_vcpu\n");
	}

	if (is_service_vm(vm)) {
		vcpu = &vm->hw.vcpu[0];
		ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];
		cpu_csr_set(sepc, kinfo->entry);
//		ctx->run_ctx.sepc = kinfo->entry;
//		pr_info("%lx %lx ", *((uint64_t *)(regs->pc)), *((uint64_t *)(regs->pc + 8)));
//		pr_info("%lx %lx ", *((uint64_t *)(regs->x0)), *((uint64_t *)(regs->x0 + 8)));

		/* prepare upcall interrupt, irq is hardcode in kernel */
/*
		if (vgic_reserve_virq(vm, CONFIG_UPCALL_IRQ))
			pr_info("Reserved UPCALL interrupt irq %d successfully!", CONFIG_UPCALL_IRQ);
		else
			pr_info("Reserved UPCALL interrupt irq %d failed!!!!!!!!!!", CONFIG_UPCALL_IRQ);
*/
	} else {
		/* FIXME:
		 * currently, directly enable IO completion polling mode, as there would be some race when
		 * passthru physical uart irq to Guest OS. */
		vm->sw.is_polling_ioreq = true;
		/* prepare virtio block interrupt, irq is hardcode in kernel */
/*
		if (vgic_reserve_virq(vm, CONFIG_UOS_VIRTIO_BLK_IRQ))
			pr_info("Reserved virtio blk interrupt irq %d successfully!", CONFIG_UOS_VIRTIO_BLK_IRQ);
		else
			pr_info("Reserved virtio blk interrupt irq %d failed!!!!!!!!!!", CONFIG_UOS_VIRTIO_BLK_IRQ);

		if (vgic_reserve_virq(vm, CONFIG_UOS_VIRTIO_NET_IRQ))
			pr_info("Reserved virtio net interrupt irq %d successfully!", CONFIG_UOS_VIRTIO_NET_IRQ);
		else
			pr_info("Reserved virtio net interrupt irq %d failed!", CONFIG_UOS_VIRTIO_NET_IRQ);
*/
		//irq = vgic_get_irq(vm, &vm->hw.vcpu[0], CONFIG_UOS_VIRTIO_BLK_IRQ);
		//irq->config = 1; /* level trigger for virtio device */

		// Passthru physical uart irq to Post-launched VM.
//		release_guest_irq(sos_vm, CONFIG_PHY_UART_IRQ);
		map_irq_to_vm(vm, CONFIG_PHY_UART_IRQ);
	}
	vm->state = VM_CREATED;
 
	return 0;
}

int32_t shutdown_vm(struct acrn_vm *vm)
{
	/* Only allow shutdown paused vm */
	vm->state = VM_POWERED_OFF;

	/* TODO: only have one core */
	offline_vcpu(&vm->hw.vcpu[0]);

	/* Return status to caller */
	return 0;
}

void pause_vm(struct acrn_vm *vm)
{
	/* For RTVM, we can only pause its vCPUs when it is powering off by itself */
	zombie_vcpu(&vm->hw.vcpu[0], VCPU_ZOMBIE);
	vm->state = VM_PAUSED;
}

int32_t reset_vm(struct acrn_vm *vm)
{
	if (is_service_vm(vm)) {
		/* TODO: */
	}

	reset_vm_ioreqs(vm);
	vm->state = VM_CREATED;

	return 0;
}

void start_vm(struct acrn_vm *vm)
{
	vm->state = VM_RUNNING;

	for (int i = 0; i < vm->hw.created_vcpus; i++)
		launch_vcpu(&vm->hw.vcpu[i]);
}

static inline struct cpu_info *get_cpu_info_from_sp(uint16_t id)
{
	return &curr_vcpu->arch.cpu_info;
}

void start_sos_vm(void)
{
	start_vm(sos_vm);
}

void update_vm_vclint_state(struct acrn_vm *vm)
{
}

/**
 * @pre vm != NULL && vm_config != NULL && vm->vmid < CONFIG_MAX_VM_NUM
 */
bool vm_hide_mtrr(const struct acrn_vm *vm)
{
	struct acrn_vm_config *vm_config = get_vm_config(vm->vm_id);

	return ((vm_config->guest_flags & GUEST_FLAG_HIDE_MTRR) != 0U);
}

/**
 * @pre vm != NULL && vm_config != NULL && vm->vmid < CONFIG_MAX_VM_NUM
 */
uint64_t read_vmtrr(const struct acrn_vcpu *vcpu, uint32_t msr)
{
	return 0;
}

/* virtual MTRR MSR write API */
void write_vmtrr(struct acrn_vcpu *vcpu, uint32_t csr, uint64_t value)
{
}

/**
 * @pre vm != NULL
 * @pre vm->vmid < CONFIG_MAX_VM_NUM
 */
bool is_prelaunched_vm(const struct acrn_vm *vm)
{
	struct acrn_vm_config *vm_config;

	vm_config = get_vm_config(vm->vm_id);
	return (vm_config->load_order == PRE_LAUNCHED_VM);
}


/**
 * @pre vm != NULL
 * @pre vm->vmid < CONFIG_MAX_VM_NUM
 */
bool is_postlaunched_vm(const struct acrn_vm *vm)
{
	return (get_vm_config(vm->vm_id)->load_order == POST_LAUNCHED_VM);
}

bool is_valid_postlaunched_vmid(uint16_t vm_id)
{
	return ((vm_id < CONFIG_MAX_VM_NUM) && is_postlaunched_vm(get_vm_from_vmid(vm_id)));
}

/**
 * return a pointer to the virtual machine structure associated with
 * this VM ID
 *
 * @pre vm_id < CONFIG_MAX_VM_NUM
 */
struct acrn_vm *get_vm_from_vmid(uint16_t vm_id)
{
	return &vm_array[vm_id];
}

uint16_t get_vmid_by_uuid(const uint8_t *uuid)
{
	uint16_t vm_id = 0U;

	return vm_id;
}

#include <asm/guest/vm.h>
#include <asm/guest/vcpu.h>
#include <debug/logmsg.h>
#include <asm/page.h>
#include <lib/errno.h>
#include <asm/board.h>
#include <acrn/config.h>
#include <asm/irq.h>
#include <asm/board.h>
#include <asm/current.h>

static struct acrn_vm vm_array[CONFIG_MAX_VM_NUM] __aligned(PAGE_SIZE);
struct acrn_vm_config vm_configs;
struct acrn_vm *sos_vm = &vm_array[0];
struct acrn_vm *uos_vm = &vm_array[1];

#define ZIMAGE64_MAGIC_V0 0x14000008
#define ZIMAGE64_MAGIC_V1 0x644d5241

#define MASK_2M 0xFFFFFFFFFFE00000

void get_kernel_info(struct kernel_info *info);
void get_dtb_info(struct dtb_info *info);
void init_s2pt_mem_ops(struct memory_ops *mem_ops, uint16_t vm_id);
int create_vcpu(struct acrn_vm *vm, uint16_t vcpu_id);
void offline_vcpu(struct acrn_vcpu *vcpu);
void zombie_vcpu(struct acrn_vcpu *vcpu, enum vcpu_state new_state);
void reset_vm_ioreqs(struct acrn_vm *vm);

bool is_poweroff_vm(const struct acrn_vm *vm)
{
	return (vm->state == VM_POWERED_OFF);
}

bool is_created_vm(const struct acrn_vm *vm)
{
	return (vm->state == VM_CREATED);
}

bool is_paused_vm(const struct acrn_vm *vm)
{
	return (vm->state == VM_PAUSED);
}

bool is_sos_vm(const struct acrn_vm *vm)
{
	return (vm != NULL) && (vm == sos_vm);
}

void get_vm_lock(struct acrn_vm *vm) { }
void put_vm_lock(struct acrn_vm *vm) { }

static void kernel_load(struct kernel_info *info)
{
	paddr_t load_addr;
	paddr_t paddr = info->kernel_addr;
	paddr_t len = info->kernel_len;
	void *kernel_hva;
	int rc;

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
	info->text_offset = _boot;
	return 0;
}

static void init_vm_sw_load(struct acrn_vm *vm)
{
	struct kernel_info *kinfo = &vm->sw.kernel_info;
	struct dtb_info *dinfo = &vm->sw.dtb_info;

	get_kernel_info(kinfo);
	pr_info("kernel addr =%lx kernel size =%lx", kinfo->kernel_addr, kinfo->kernel_len);
	get_dtb_info(dinfo);
	kernel_header_parse(kinfo);
}

void prepare_sos_vm(void)
{
	sos_vm->vm_id = 0;
	init_s2pt_mem_ops(&sos_vm->arch_vm.s2pt_mem_ops, sos_vm->vm_id);
	sos_vm->arch_vm.s2ptp = sos_vm->arch_vm.s2pt_mem_ops.get_pml4_page(sos_vm->arch_vm.s2pt_mem_ops.info);
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
}

static void dtb_load(struct dtb_info *info)
{
	void *dtb_hva = hpa2hva(info->dtb_addr);
	int rc = 0;

	return;

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

int map_irq_to_vm(struct acrn_vm *vm, unsigned int irq)
{
	int res;

	return 0;
}

int create_vm(struct acrn_vm *vm)
{
	struct guest_cpu_context *ctx;
	struct acrn_vcpu *vcpu;
	struct kernel_info *kinfo= &vm->sw.kernel_info;
	struct dtb_info *dinfo= &vm->sw.dtb_info;
	int ret, i;

	/* TODO: only support one SOS and one UOS now */
	if (is_sos_vm(vm)) {
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
	spin_lock_init(&vm->emul_mmio_lock);

	if (is_sos_vm(vm)) {
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
	if (is_sos_vm(vm)) {
		vcpu = &vm->hw.vcpu[0];
		ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];
		cpu_csr_set(sepc, kinfo->entry);
	} else {
		vm->sw.is_polling_ioreq = true;
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
	if (is_sos_vm(vm)) {
		/* TODO: */
	}

	reset_vm_ioreqs(vm);
	vm->state = VM_CREATED;

	return 0;
}

void start_vm(struct acrn_vm *vm)
{
	struct acrn_vcpu *bsp = NULL;

	vm->state = VM_RUNNING;

	/* Only start BSP (vid = 0) and let BSP start other APs */
	bsp = &vm->hw.vcpu[0];
	launch_vcpu(bsp);
}

static inline struct cpu_info *get_cpu_info_from_sp(uint16_t id)
{
	return &get_current()->arch.cpu_info;
}

#define guest_cpu_user_regs() (get_cpu_info_from_sp(get_processor_id())->guest_cpu_ctx_regs)
#define switch_stack_and_jump(stack, fn) \
	asm volatile ("mov sp,%0; b " STR(fn) : : "r" (stack) : "memory" )
#define reset_stack_and_jump(fn) switch_stack_and_jump(get_cpu_info_from_sp(get_processor_id()), fn)

static void context_switch(struct acrn_vcpu *prev, struct acrn_vcpu *next)
{
	local_irq_disable();

	set_current(next);
	pr_info("from prev %s to next %s ", prev->thread_obj.name, next->thread_obj.name);
	arch_switch_to(&prev->thread_obj.host_sp, &next->thread_obj.host_sp);

	local_irq_enable();
}

static void ctxt_switch_to(struct acrn_vcpu *n)
{
}

void continue_new_vcpu(__unused struct acrn_vcpu *prev, struct acrn_vcpu *next)
{
	struct cpu_user_regs *regs ;
}

void start_sos_vm(void)
{
	struct acrn_vcpu *next = &sos_vm->hw.vcpu[0];
	pr_info("start vm");

	context_switch(&idle_vcpu[0], next);
}

void update_vm_vclint_state(struct acrn_vm *vm)
{
}

bool vm_hide_mtrr(const struct acrn_vm *vm)
{
	struct acrn_vm_config *vm_config = get_vm_config(vm->vm_id);

	return ((vm_config->guest_flags & GUEST_FLAG_HIDE_MTRR) != 0U);
}

bool read_vmtrr(const struct acrn_vm *vm)
{
	struct acrn_vm_config *vm_config = get_vm_config(vm->vm_id);

	return ((vm_config->guest_flags & GUEST_FLAG_HIDE_MTRR) != 0U);
}

void write_vmtrr(struct acrn_vcpu *vcpu, uint32_t csr, uint64_t value)
{
}

bool is_prelaunched_vm(const struct acrn_vm *vm)
{
	struct acrn_vm_config *vm_config;

	vm_config = get_vm_config(vm->vm_id);
	return (vm_config->load_order == PRE_LAUNCHED_VM);
}


bool is_postlaunched_vm(const struct acrn_vm *vm)
{
	return (get_vm_config(vm->vm_id)->load_order == POST_LAUNCHED_VM);
}

bool is_valid_postlaunched_vmid(uint16_t vm_id)
{
	return ((vm_id < CONFIG_MAX_VM_NUM) && is_postlaunched_vm(get_vm_from_vmid(vm_id)));
}

struct acrn_vm *get_vm_from_vmid(uint16_t vm_id)
{
	return &vm_array[vm_id];
}

uint16_t get_vmid_by_uuid(const uint8_t *uuid)
{
	uint16_t vm_id = 0U;

	return vm_id;
}

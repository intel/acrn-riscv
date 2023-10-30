#enable stack overflow check
BASEDIR := $(shell pwd)
HV_OBJDIR ?= $(CURDIR)/build
HV_MODDIR ?= $(HV_OBJDIR)/modules
HV_FILE := acrn


BOOT_MOD = $(HV_MODDIR)/boot_mod.a


# initialize the flags we used
CFLAGS := -D__riscv64__
ASFLAGS := -D__riscv64__
LDFLAGS :=
ARFLAGS :=
ARCH_CFLAGS := -march=rv64iadh1p0_zifencei -mabi=lp64d -mcmodel=medany
ARCH_ASFLAGS :=
ARCH_ARFLAGS :=
ARCH_LDFLAGS := -mcmodel=medany

.PHONY: default
default: all


SCENARIO := test
BOARD := test


include ../paths.make

LD_IN_TOOL = scripts/genld.sh
BASH = $(shell which bash)

ARFLAGS += crs

# ACRN depends on zero length array. Silence the gcc if Warrary-bounds is default option
CFLAGS += -Wno-array-bounds
ifeq (y, $(CONFIG_RELOC))
CFLAGS += -fpie
else
CFLAGS += -static
endif


ifdef STACK_PROTECTOR
ifeq (true, $(shell [ $(GCC_MAJOR) -gt 4 ] && echo true))
CFLAGS += -fstack-protector-strong
else
ifeq (true, $(shell [ $(GCC_MAJOR) -eq 4 ] && [ $(GCC_MINOR) -ge 9 ] && echo true))
CFLAGS += -fstack-protector-strong
else
CFLAGS += -fstack-protector
endif
endif
CFLAGS += -DSTACK_PROTECTOR
endif

CFLAGS += -DCONFIG_RISCV_64

#ASFLAGS += -nostdlib
ASFLAGS += -nostdinc

ifeq (y, $(CONFIG_RELOC))
ASFLAGS += -DCONFIG_RELOC
endif

#LDFLAGS += -Wl,--gc-sections -nostartfiles -nostdlib 
LDFLAGS += -Wl,--gc-sections -nostartfiles  
LDFLAGS += -Wl,-n,-z,max-page-size=0x1000
LDFLAGS += -Wl,--no-dynamic-linker

LDFLAGS += -static


ARCH_CFLAGS += -DBUILD_ID -fno-strict-aliasing -Wall -Wstrict-prototypes -Wdeclaration-after-statement -Wno-unused-but-set-variable -Wno-unused-local-typedefs -O1 -fno-omit-frame-pointer -fno-builtin -fno-common -Wredundant-decls -Wno-pointer-arith -Wvla -pipe -Wa,--strip-local-absolute -g -mcmodel=medany -fno-stack-protector -fno-exceptions -fno-asynchronous-unwind-tables -Wnested-externs  

ARCH_CFLAGS += -D__ACRN__
CFLAGS += -std=gnu99

ARCH_ASFLAGS += -D__ASSEMBLY__ -DBUILD_ID -fno-strict-aliasing -Wall -Wstrict-prototypes -Wdeclaration-after-statement -Wno-unused-but-set-variable -Wno-unused-local-typedefs -O1 -fno-omit-frame-pointer  -fno-builtin -fno-common -Wredundant-decls -Wno-pointer-arith -Wvla -pipe  -Wa,--strip-local-absolute -g -mcmodel=medany -fno-stack-protector -fno-exceptions -fno-asynchronous-unwind-tables -Wnested-externs 
ARCH_ASFLAGS += -DCONFIG_RISCV_64
ARCH_ASFLAGS += -D__ACRN__
ARCH_ASFLAGS += -xassembler-with-cpp
ARCH_ARFLAGS +=
ARCH_LDFLAGS +=


ARCH_LDSCRIPT = ram_link.ld
ARCH_LDSCRIPT_IN = arch/riscv/ram_link.lds.S

INCLUDE_PATH += boot/include/
INCLUDE_PATH += include/lib/
INCLUDE_PATH += include/
INCLUDE_PATH += include/hw
INCLUDE_PATH += include/common/
INCLUDE_PATH += include/arch/
INCLUDE_PATH += include/debug/
INCLUDE_PATH += include/public/
INCLUDE_PATH += include/dm/
INCLUDE_PATH += /usr/include
#INCLUDE_PATH += /home/haicheng/.riscv_gnu_toolchain/include

#CC := riscv64-unknown-elf-gcc
#AS := riscv64-unknown-elf-as
#AR := riscv64-unknown-elf-ar
#LD := riscv64-unknown-elf-ld
#OBJCOPY ?= riscv64-unknown-elf-objcopy
CC := riscv64-unknown-linux-gnu-gcc
AS := riscv64-unknown-linux-gnu-as
AR := riscv64-unknown-linux-gnu-ar
LD := riscv64-unknown-linux-gnu-ld
OBJCOPY ?= riscv64-unknown-linux-gnu-objcopy
#LD := riscv-none-elf-ld


CFLAGS += -DCONFIG_RETPOLINE

# platform boot component
BOOT_S_SRCS += arch/riscv/start.s
BOOT_S_SRCS += arch/riscv/intr.s
BOOT_S_SRCS += arch/riscv/mmu.s
BOOT_S_SRCS += arch/riscv/guest/virt.s
BOOT_S_SRCS += arch/riscv/app.s
BOOT_S_SRCS += arch/riscv/sched.s

BOOT_C_SRCS += arch/riscv/mtrap.c
BOOT_C_SRCS += arch/riscv/trap.c
BOOT_C_SRCS += arch/riscv/smp.c
BOOT_C_SRCS += arch/riscv/uart.c
BOOT_C_SRCS += arch/riscv/kernel.c
BOOT_C_SRCS += arch/riscv/guest.c
BOOT_C_SRCS += arch/riscv/user.c
BOOT_C_SRCS += arch/riscv/mem.c
BOOT_C_SRCS += arch/riscv/page.c
BOOT_C_SRCS += arch/riscv/pagetable.c
BOOT_C_SRCS += arch/riscv/setup.c
BOOT_C_SRCS += arch/riscv/percpu.c
BOOT_C_SRCS += arch/riscv/smpboot.c
BOOT_C_SRCS += arch/riscv/timer.c
BOOT_C_SRCS += arch/riscv/irq.c
BOOT_C_SRCS += arch/riscv/clint.c
BOOT_C_SRCS += arch/riscv/plic.c
BOOT_C_SRCS += arch/riscv/bitops.c
BOOT_C_SRCS += arch/riscv/notify.c
BOOT_C_SRCS += arch/riscv/boot.c
BOOT_C_SRCS += arch/riscv/lib/memory.c
BOOT_C_SRCS += arch/riscv/lib/find_next_bit.c

BOOT_C_SRCS += arch/riscv/hx.c
BOOT_C_SRCS += arch/riscv/guest/vmcs.c
BOOT_C_SRCS += arch/riscv/guest/vm.c
BOOT_C_SRCS += arch/riscv/guest/vio.c
BOOT_C_SRCS += arch/riscv/guest/s2vm.c
BOOT_C_SRCS += arch/riscv/guest/vcpu.c
BOOT_C_SRCS += arch/riscv/guest/vcsr.c
BOOT_C_SRCS += arch/riscv/guest/virq.c
BOOT_C_SRCS += arch/riscv/guest/vclint.c
BOOT_C_SRCS += arch/riscv/guest/vmexit.c
BOOT_C_SRCS += arch/riscv/guest/vmcall.c
BOOT_C_SRCS += arch/riscv/guest/guest_memory.c
BOOT_C_SRCS += arch/riscv/guest/instr_emul.c

BOOT_C_SRCS += debug/printf.c
BOOT_C_SRCS += release/profiling.c
#BOOT_C_SRCS += debug/trace.c
BOOT_C_SRCS += lib/sprintf.c
BOOT_C_SRCS += lib/string.c
BOOT_C_SRCS += common/bitmap.c
BOOT_C_SRCS += common/spinlock.c
BOOT_C_SRCS += common/logmsg.c
BOOT_C_SRCS += common/uart16550.c
BOOT_C_SRCS += common/console.c
BOOT_C_SRCS += common/shell.c
BOOT_C_SRCS += common/schedule.c
BOOT_C_SRCS += common/sched_noop.c
BOOT_C_SRCS += common/event.c
BOOT_C_SRCS += common/hv_main.c
BOOT_C_SRCS += common/bsearch.c
BOOT_C_SRCS += common/rwlock.c
BOOT_C_SRCS += common/hypercall.c
#BOOT_C_SRCS += dm/vpic.c
BOOT_C_SRCS += dm/io_req.c

BOOT_C_OBJS := $(patsubst %.c,$(HV_OBJDIR)/%.o,$(BOOT_C_SRCS))
BOOT_S_OBJS := $(patsubst %.s,$(HV_OBJDIR)/%.o,$(BOOT_S_SRCS))

MODULES += $(BOOT_MOD)

DISTCLEAN_OBJS := $(shell find $(BASEDIR) -name '*.o')


.PHONY: all
all: pre_setup $(HV_OBJDIR)/$(HV_FILE).elf 

pre_setup:
	out=$(shell cd include/arch/; ln -sf riscv asm; cd -)

.PHONY: boot-mod 

$(BOOT_MOD): $(BOOT_S_OBJS) $(BOOT_C_OBJS)
	$(AR) $(ARFLAGS) $(BOOT_MOD) $(BOOT_S_OBJS) $(BOOT_C_OBJS)

boot-mod: $(BOOT_MOD)

$(HV_OBJDIR)/$(HV_FILE).elf: $(MODULES) $(HV_OBJDIR)/$(ARCH_LDSCRIPT)
	$(CC) -Wl,-Map=$(HV_OBJDIR)/$(HV_FILE).map -o $@ $(LDFLAGS) $(ARCH_LDFLAGS) -T$(HV_OBJDIR)/$(ARCH_LDSCRIPT) \
		-Wl,--start-group $(MODULES) -Wl,--end-group

$(HV_OBJDIR)/$(ARCH_LDSCRIPT): $(ARCH_LDSCRIPT_IN)
	#cp $< $@
	$(CC) -E -P $(patsubst %, -I%, $(INCLUDE_PATH))  $(ARCH_ASFLAGS) -include include/acrn/config.h -MMD -MT $@ -o $@ $<


.PHONY: clean
clean:
	rm -rf $(HV_OBJDIR)
	rm -rf include/arch/asm

.PHONY: distclean
distclean:
	rm -f $(DISTCLEAN_OBJS)
	rm -rf $(HV_OBJDIR)
	rm -f tags TAGS cscope.files cscope.in.out cscope.out cscope.po.out GTAGS GPATH GRTAGS GSYMS

$(HV_OBJDIR)/%.o: %.c  $(HV_OBJDIR)/$(HV_CONFIG_H) $(TARGET_ACPI_INFO_HEADER)
	[ ! -e $@ ] && mkdir -p $(dir $@) && mkdir -p $(HV_MODDIR); \
	$(CC) $(patsubst %, -I%, $(INCLUDE_PATH)) $(CFLAGS) $(ARCH_CFLAGS) -include include/acrn/config.h -c  $< -o $@ -MMD -MT $@

$(HV_OBJDIR)/%.o: %.s
	[ ! -e $@ ] && mkdir -p $(dir $@) && mkdir -p $(HV_MODDIR); \
	$(CC) $(patsubst %, -I%, $(INCLUDE_PATH))  $(ASFLAGS) $(ARCH_ASFLAGS) -include include/acrn/config.h  -c $< -o $@ -MMD -MT $@

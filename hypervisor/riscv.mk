CROSS_COMPILE ?= riscv64-unknown-linux-gnu-

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
ifdef CONFIG_SIFIVE_UNMATCHED
ARCH_CFLAGS := -march=rv64imafdc_zifencei -mabi=lp64d -mcmodel=medany
ARCH_ASFLAGS :=  -march=rv64imafdc_zifencei
CFLAGS += -DCONFIG_SIFIVE_UNMATCHED
ASFLAGS += -DCONFIG_SIFIVE_UNMATCHED
CONFIG_SIFIVE_UART := 1
else
ARCH_CFLAGS := -march=rv64g_zifencei_zbb -mabi=lp64d -mcmodel=medany
ARCH_ASFLAGS := -march=rv64g_zifencei_zbb
endif
ARCH_ARFLAGS :=
ARCH_LDFLAGS := -mcmodel=medany


.PHONY: default
default: all

SCENARIO := test
BOARD := test

BOARD_INFO_DIR := $(HV_CONFIG_DIR)/boards
SCENARIO_CFG_DIR := $(HV_CONFIG_DIR)/scenarios/$(SCENARIO)
BOARD_CFG_DIR := $(SCENARIO_CFG_DIR)

#include scripts/makefile/config.mk

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

CFLAGS += -DCONFIG_RISCV64

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

ARCH_CFLAGS += -DBUILD_ID -fno-strict-aliasing -Werror -Wall -Wstrict-prototypes -Wdeclaration-after-statement -Wno-unused-but-set-variable -Wno-unused-local-typedefs -O1 -fno-omit-frame-pointer -fno-builtin -fno-common -Wredundant-decls -Wno-pointer-arith -Wvla -pipe -Wa,--strip-local-absolute -g -mcmodel=medany -fno-stack-protector -fno-exceptions -fno-asynchronous-unwind-tables -Wnested-externs

ARCH_CFLAGS += -D__ACRN__
CFLAGS += -std=gnu99

ARCH_ASFLAGS += -D__ASSEMBLY__ -DBUILD_ID -fno-strict-aliasing -Werror -Wall -Wstrict-prototypes -Wdeclaration-after-statement -Wno-unused-but-set-variable -Wno-unused-local-typedefs -O1 -fno-omit-frame-pointer  -fno-builtin -fno-common -Wredundant-decls -Wno-pointer-arith -Wvla -pipe  -Wa,--strip-local-absolute -g -mcmodel=medany -fno-stack-protector -fno-exceptions -fno-asynchronous-unwind-tables -Wnested-externs
ARCH_ASFLAGS += -DCONFIG_RISCV64
ARCH_ASFLAGS += -D__ACRN__
ARCH_ASFLAGS += -xassembler-with-cpp

ARCH_LDSCRIPT = ram_link.ld
ARCH_LDSCRIPT_IN = arch/riscv/ram_link.lds.S

INCLUDE_PATH += boot/include/
INCLUDE_PATH += include/lib/
INCLUDE_PATH += include/
INCLUDE_PATH += include/hw
INCLUDE_PATH += include/common/
INCLUDE_PATH += include/acrn/
INCLUDE_PATH += include/arch/
INCLUDE_PATH += include/debug/
INCLUDE_PATH += include/public/
INCLUDE_PATH += include/dm/
INCLUDE_PATH += include/arch/$(ARCH)
#INCLUDE_PATH += /usr/include

CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)ld
OBJCOPY ?= $(CROSS_COMPILE)objcopy

CFLAGS += -DCONFIG_RETPOLINE

# m-mode hypervisor
CONFIG_MACRN := 1

# unit testing framework with builtin fake kernel
#CONFIG_KTEST := 1

ifdef CONFIG_MACRN
CFLAGS += -DCONFIG_MACRN
ASFLAGS += -DCONFIG_MACRN
endif

ifdef CONFIG_KTEST
CFLAGS += -DCONFIG_KTEST
ASFLAGS += -DCONFIG_KTEST
endif

# platform boot component
BOOT_S_SRCS += arch/riscv/start.s
BOOT_S_SRCS += arch/riscv/intr.s
ifdef CONFIG_MACRN
BOOT_S_SRCS += arch/riscv/guest/pmp.s
else
BOOT_S_SRCS += arch/riscv/guest/virt.s
endif
BOOT_S_SRCS += arch/riscv/sched.s
ifdef CONFIG_KTEST
BOOT_S_SRCS += arch/riscv/ktest/mmu.s
BOOT_S_SRCS += arch/riscv/ktest/start.s
BOOT_S_SRCS += arch/riscv/ktest/intr.s
endif

BOOT_C_SRCS += arch/riscv/mtrap.c
BOOT_C_SRCS += arch/riscv/trap.c
BOOT_C_SRCS += arch/riscv/smp.c
ifdef CONFIG_SIFIVE_UART
BOOT_C_SRCS += arch/riscv/sifive_uart.c
else
BOOT_C_SRCS += arch/riscv/uart.c
endif
BOOT_C_SRCS += arch/riscv/setup.c
BOOT_C_SRCS += arch/riscv/percpu.c
BOOT_C_SRCS += arch/riscv/smpboot.c
BOOT_C_SRCS += arch/riscv/timer.c
BOOT_C_SRCS += arch/riscv/irq.c
BOOT_C_SRCS += arch/riscv/clint.c
BOOT_C_SRCS += arch/riscv/plic.c
BOOT_C_SRCS += arch/riscv/notify.c
BOOT_C_SRCS += arch/riscv/boot.c
BOOT_C_SRCS += arch/riscv/lib/bits.c
BOOT_C_SRCS += arch/riscv/lib/memory.c

BOOT_C_SRCS += arch/riscv/guest/vmcs.c
BOOT_C_SRCS += arch/riscv/guest/vm.c
BOOT_C_SRCS += arch/riscv/guest/vio.c
BOOT_C_SRCS += arch/riscv/guest/sbi.c
BOOT_C_SRCS += arch/riscv/guest/vuart.c
BOOT_C_SRCS += arch/riscv/guest/vpci/vuart.c
BOOT_C_SRCS += arch/riscv/guest/vpci/vdev.c
BOOT_C_SRCS += arch/riscv/guest/vpci/vhostbridge.c

BOOT_C_SRCS += arch/riscv/mem.c
BOOT_C_SRCS += arch/riscv/pgtable.c
BOOT_C_SRCS += arch/riscv/pager.c

ifndef CONFIG_MACRN
BOOT_C_SRCS += arch/riscv/guest/s2vm.c
endif

BOOT_C_SRCS += arch/riscv/guest/vcpu.c
BOOT_C_SRCS += arch/riscv/guest/vcsr.c
BOOT_C_SRCS += arch/riscv/guest/virq.c
BOOT_C_SRCS += arch/riscv/guest/vclint.c
BOOT_C_SRCS += arch/riscv/guest/vplic.c
BOOT_C_SRCS += arch/riscv/guest/vmexit.c
BOOT_C_SRCS += arch/riscv/guest/vmcall.c
BOOT_C_SRCS += arch/riscv/guest/guest_memory.c
BOOT_C_SRCS += arch/riscv/guest/instr_emul.c
BOOT_C_SRCS += arch/riscv/guest/tee.c

BOOT_C_SRCS += release/profiling.c
BOOT_C_SRCS += release/trace.c
BOOT_C_SRCS += lib/sprintf.c
BOOT_C_SRCS += lib/string.c
BOOT_C_SRCS += common/timer.c
BOOT_C_SRCS += common/irq.c
BOOT_C_SRCS += common/sbuf.c
BOOT_C_SRCS += common/schedule.c
BOOT_C_SRCS += common/sched_iorr.c
BOOT_C_SRCS += common/softirq.c
BOOT_C_SRCS += common/event.c
BOOT_C_SRCS += common/ticks.c
BOOT_C_SRCS += common/hv_main.c
#BOOT_C_SRCS += common/hypercall.c
BOOT_C_SRCS += debug/printf.c
BOOT_C_SRCS += release/sbuf.c
BOOT_C_SRCS += debug/shell.c
BOOT_C_SRCS += debug/string.c
BOOT_C_SRCS += debug/logmsg.c
BOOT_C_SRCS += debug/console.c
BOOT_C_SRCS += release/hypercall.c
#BOOT_C_SRCS += dm/vpic.c
#BOOT_C_SRCS += dm/vuart.c
BOOT_C_SRCS += dm/io_req.c
#BOOT_C_SRCS += dm/vpci/vdev.c

ifdef CONFIG_KTEST
BOOT_C_SRCS += arch/riscv/ktest/app.c
BOOT_C_SRCS += arch/riscv/ktest/smp.c
endif

BOOT_C_OBJS := $(patsubst %.c,$(HV_OBJDIR)/%.o,$(BOOT_C_SRCS))
BOOT_S_OBJS := $(patsubst %.s,$(HV_OBJDIR)/%.o,$(BOOT_S_SRCS))

MODULES += $(BOOT_MOD)

DISTCLEAN_OBJS := $(shell find $(BASEDIR) -name '*.o')


.PHONY: all
all: $(HV_OBJDIR)/$(HV_FILE).elf


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
	$(CC) $(patsubst %, -I%, $(INCLUDE_PATH)) $(ASFLAGS) $(ARCH_ASFLAGS) -include include/acrn/config.h -c $< -o $@ -MMD -MT $@

.DEFAULT_GOAL := all

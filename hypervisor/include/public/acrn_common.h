/*
 * common definition
 *
 * Copyright (C) 2017 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

/**
 * @file acrn_common.h
 *
 * @brief acrn common data structure for hypercall or ioctl
 */

#ifndef ACRN_COMMON_H
#define ACRN_COMMON_H

#include <types.h>

/*
 * Common structures for ACRN/VHM/DM
 */

/*
 * IO request
 */
#define VHM_REQUEST_MAX 16U

#define REQ_STATE_FREE          3U
#define REQ_STATE_PENDING	0U
#define REQ_STATE_COMPLETE	1U
#define REQ_STATE_PROCESSING	2U

#define REQ_PORTIO	0U
#define REQ_MMIO	1U
#define REQ_PCICFG	2U
#define REQ_WP		3U

#define REQUEST_READ	0U
#define REQUEST_WRITE	1U

/* IOAPIC device model info */
#define VIOAPIC_RTE_NUM	48U  /* vioapic pins */

#if VIOAPIC_RTE_NUM < 24U
#error "VIOAPIC_RTE_NUM must be larger than 23"
#endif

/* Generic VM flags from guest OS */
#define GUEST_FLAG_SECURE_WORLD_ENABLED		(1UL << 0U)	/* Whether secure world is enabled */
#define GUEST_FLAG_LAPIC_PASSTHROUGH		(1UL << 1U)  	/* Whether LAPIC is passed through */
#define GUEST_FLAG_IO_COMPLETION_POLLING	(1UL << 2U)  	/* Whether need hypervisor poll IO completion */
#define GUEST_FLAG_HIDE_MTRR			(1UL << 3U)  	/* Whether hide MTRR from VM */
#define GUEST_FLAG_RT				(1UL << 4U)     /* Whether the vm is RT-VM */

/* TODO: We may need to get this addr from guest ACPI instead of hardcode here */
#define VIRTUAL_PM1A_CNT_ADDR		0x404U
#define	VIRTUAL_PM1A_SCI_EN		0x0001
#define VIRTUAL_PM1A_SLP_TYP		0x1c00U
#define VIRTUAL_PM1A_SLP_EN		0x2000U
#define	VIRTUAL_PM1A_ALWAYS_ZERO	0xc003

/**
 * @brief Hypercall
 *
 * @addtogroup acrn_hypercall ACRN Hypercall
 * @{
 */

/**
 * @brief Representation of a MMIO request
 */
struct mmio_request {
	/**
	 * @brief Direction of the access
	 *
	 * Either \p REQUEST_READ or \p REQUEST_WRITE.
	 */
	uint32_t direction;

	/**
	 * @brief reserved
	 */
	uint32_t reserved;

	/**
	 * @brief Address of the I/O access
	 */
	uint64_t address;

	/**
	 * @brief Width of the I/O access in byte
	 */
	uint64_t size;

	/**
	 * @brief The value read for I/O reads or to be written for I/O writes
	 */
	uint64_t value;
} __aligned(8);

/**
 * @brief Representation of a port I/O request
 */
struct pio_request {
	/**
	 * @brief Direction of the access
	 *
	 * Either \p REQUEST_READ or \p REQUEST_WRITE.
	 */
	uint32_t direction;

	/**
	 * @brief reserved
	 */
	uint32_t reserved;

	/**
	 * @brief Port address of the I/O access
	 */
	uint64_t address;

	/**
	 * @brief Width of the I/O access in byte
	 */
	uint64_t size;

	/**
	 * @brief The value read for I/O reads or to be written for I/O writes
	 */
	uint32_t value;
} __aligned(8);

/**
 * @brief Representation of a PCI configuration space access
 */
struct pci_request {
	/**
	 * @brief Direction of the access
	 *
	 * Either \p REQUEST_READ or \p REQUEST_WRITE.
	 */
	uint32_t direction;

	/**
	 * @brief Reserved
	 */
	uint32_t reserved[3];/* need keep same header fields with pio_request */

	/**
	 * @brief Width of the I/O access in byte
	 */
	int64_t size;

	/**
	 * @brief The value read for I/O reads or to be written for I/O writes
	 */
	int32_t value;

	/**
	 * @brief The \p bus part of the BDF of the device
	 */
	int32_t bus;

	/**
	 * @brief The \p device part of the BDF of the device
	 */
	int32_t dev;

	/**
	 * @brief The \p function part of the BDF of the device
	 */
	int32_t func;

	/**
	 * @brief The register to be accessed in the configuration space
	 */
	int32_t reg;
} __aligned(8);

union vhm_io_request {
	struct pio_request pio;
	struct pci_request pci;
	struct mmio_request mmio;
	int64_t reserved1[8];
};

/**
 * @brief 256-byte VHM requests
 *
 * The state transitions of a VHM request are:
 *
 *    FREE -> PENDING -> PROCESSING -> COMPLETE -> FREE -> ...
 *
 * When a request is in COMPLETE or FREE state, the request is owned by the
 * hypervisor. SOS (VHM or DM) shall not read or write the internals of the
 * request except the state.
 *
 * When a request is in PENDING or PROCESSING state, the request is owned by
 * SOS. The hypervisor shall not read or write the request other than the state.
 *
 * Based on the rules above, a typical VHM request lifecycle should looks like
 * the following.
 *
 * @verbatim embed:rst:leading-asterisk
 *
 * +-----------------------+-------------------------+----------------------+
 * | SOS vCPU 0            | SOS vCPU x              | UOS vCPU y           |
 * +=======================+=========================+======================+
 * |                       |                         | Hypervisor:          |
 * |                       |                         |                      |
 * |                       |                         | - Fill in type,      |
 * |                       |                         |   addr, etc.         |
 * |                       |                         | - Pause UOS vCPU y   |
 * |                       |                         | - Set state to       |
 * |                       |                         |   PENDING (a)        |
 * |                       |                         | - Fire upcall to     |
 * |                       |                         |   SOS vCPU 0         |
 * |                       |                         |                      |
 * +-----------------------+-------------------------+----------------------+
 * | VHM:                  |                         |                      |
 * |                       |                         |                      |
 * | - Scan for pending    |                         |                      |
 * |   requests            |                         |                      |
 * | - Set state to        |                         |                      |
 * |   PROCESSING (b)      |                         |                      |
 * | - Assign requests to  |                         |                      |
 * |   clients (c)         |                         |                      |
 * |                       |                         |                      |
 * +-----------------------+-------------------------+----------------------+
 * |                       | Client:                 |                      |
 * |                       |                         |                      |
 * |                       | - Scan for assigned     |                      |
 * |                       |   requests              |                      |
 * |                       | - Handle the            |                      |
 * |                       |   requests (d)          |                      |
 * |                       | - Set state to COMPLETE |                      |
 * |                       | - Notify the hypervisor |                      |
 * |                       |                         |                      |
 * +-----------------------+-------------------------+----------------------+
 * |                       | Hypervisor:             |                      |
 * |                       |                         |                      |
 * |                       | - resume UOS vCPU y     |                      |
 * |                       |   (e)                   |                      |
 * |                       |                         |                      |
 * +-----------------------+-------------------------+----------------------+
 * |                       |                         | Hypervisor:          |
 * |                       |                         |                      |
 * |                       |                         | - Post-work (f)      |
 * |                       |                         | - set state to FREE  |
 * |                       |                         |                      |
 * +-----------------------+-------------------------+----------------------+
 *
 * @endverbatim
 *
 * Note that the following shall hold.
 *
 *   1. (a) happens before (b)
 *   2. (c) happens before (d)
 *   3. (e) happens before (f)
 *   4. One vCPU cannot trigger another I/O request before the previous one has
 *      completed (i.e. the state switched to FREE)
 *
 * Accesses to the state of a vhm_request shall be atomic and proper barriers
 * are needed to ensure that:
 *
 *   1. Setting state to PENDING is the last operation when issuing a request in
 *      the hypervisor, as the hypervisor shall not access the request any more.
 *
 *   2. Due to similar reasons, setting state to COMPLETE is the last operation
 *      of request handling in VHM or clients in SOS.
 */
struct vhm_request {
	/**
	 * @brief Type of this request.
	 *
	 * Byte offset: 0.
	 */
	uint32_t type;

	/**
	 * @brief Hypervisor will poll completion if set.
	 *
	 * Byte offset: 4.
	 */
	uint32_t completion_polling;

	/**
	 * @brief Reserved.
	 *
	 * Byte offset: 8.
	 */
	uint32_t reserved0[14];

	/**
	 * @brief Details about this request.
	 *
	 * For REQ_PORTIO, this has type
	 * pio_request. For REQ_MMIO and REQ_WP, this has type mmio_request. For
	 * REQ_PCICFG, this has type pci_request.
	 *
	 * Byte offset: 64.
	 */
	union vhm_io_request reqs;

	/**
	 * @brief Reserved.
	 *
	 * Byte offset: 128.
	 */
	uint32_t reserved1;

	/**
	 * @brief The client which is distributed to handle this request.
	 *
	 * Accessed by VHM only.
	 *
	 * Byte offset: 132.
	 */
	int32_t client;

	/**
	 * @brief The status of this request.
	 *
	 * Taking REQ_STATE_xxx as values.
	 *
	 * Byte offset: 136.
	 */
	uint32_t processed;
} __aligned(256);

union vhm_request_buffer {
	struct vhm_request req_queue[VHM_REQUEST_MAX];
	int8_t reserved[4096];
} __aligned(4096);

/**
 * @brief Info to create a VCPU (deprecated)
 *
 * the parameter for HC_CREATE_VCPU hypercall
 */
struct acrn_create_vcpu {
	/** the virtual CPU ID for the VCPU created */
	uint16_t vcpu_id;

	/** the physical CPU ID for the VCPU created */
	uint16_t pcpu_id;
} __aligned(8);

/**
 * @brief Info to set ioreq buffer for a created VM
 *
 * the parameter for HC_SET_IOREQ_BUFFER hypercall
 */
struct acrn_set_ioreq_buffer {
	/** guest physical address of VM request_buffer */
	uint64_t req_buf;
} __aligned(8);

/** Operation types for setting IRQ line */
#define GSI_SET_HIGH		0U
#define GSI_SET_LOW		1U
#define GSI_RAISING_PULSE	2U
#define GSI_FALLING_PULSE	3U

/**
 * @brief Info to Set/Clear/Pulse a virtual IRQ line for a VM
 *
 * the parameter for HC_SET_IRQLINE hypercall
 */
struct acrn_irqline_ops {
	uint32_t gsi;
	uint32_t op;
} __aligned(8);

/**
 * @brief Info to inject a NMI interrupt for a VM
 */
struct acrn_nmi_entry {
	/** virtual CPU ID to inject */
	uint16_t vcpu_id;

	/** Reserved */
	uint16_t reserved0;

	/** Reserved */
	uint32_t reserved1;
} __aligned(8);

#define PMCMD_VMID_MASK                0xff000000U
#define PMCMD_VCPUID_MASK      0x00ff0000U
#define PMCMD_STATE_NUM_MASK   0x0000ff00U

#define PMCMD_VMID_SHIFT       24U
#define PMCMD_VCPUID_SHIFT     16U
#define PMCMD_STATE_NUM_SHIFT  8U

/**
 * @brief Info to remap pass-through PCI MSI for a VM
 *
 * the parameter for HC_VM_PCI_MSIX_REMAP hypercall
 */
struct acrn_vm_pci_msix_remap {
	/** pass-through PCI device virtual BDF# */
	uint16_t virt_bdf;

	/** pass-through PCI device physical BDF# */
	uint16_t phys_bdf;

	/** pass-through PCI device MSI/MSI-X cap control data */
	uint16_t msi_ctl;

	/** reserved for alignment padding */
	uint16_t reserved;

	/** pass-through PCI device MSI address to remap, which will
	 * return the caller after remapping
	 */
	uint64_t msi_addr;		/* IN/OUT: msi address to fix */

	/** pass-through PCI device MSI data to remap, which will
	 * return the caller after remapping
	 */
	uint32_t msi_data;

	/** pass-through PCI device is MSI or MSI-X
	 *  0 - MSI, 1 - MSI-X
	 */
	int32_t msix;

	/** if the pass-through PCI device is MSI-X, this field contains
	 *  the MSI-X entry table index
	 */
	uint32_t msix_entry_index;

	/** if the pass-through PCI device is MSI-X, this field contains
	 *  Vector Control for MSI-X Entry, field defined in MSI-X spec
	 */
	uint32_t vector_ctl;
} __aligned(8);

/**
 * @brief Info The power state data of a VCPU.
 *
 */

#define SPACE_SYSTEM_MEMORY     0U
#define SPACE_SYSTEM_IO         1U
#define SPACE_PCI_CONFIG        2U
#define SPACE_Embedded_Control  3U
#define SPACE_SMBUS             4U
#define SPACE_PLATFORM_COMM     10U
#define SPACE_FFixedHW          0x7FU

struct acpi_generic_address {
	uint8_t 	space_id;
	uint8_t 	bit_width;
	uint8_t 	bit_offset;
	uint8_t 	access_size;
	uint64_t	address;
} __aligned(8);

struct cpu_cx_data {
	struct acpi_generic_address cx_reg;
	uint8_t 	type;
	uint32_t	latency;
	uint64_t	power;
} __aligned(8);

struct cpu_px_data {
	uint64_t core_frequency;	/* megahertz */
	uint64_t power;			/* milliWatts */
	uint64_t transition_latency;	/* microseconds */
	uint64_t bus_master_latency;	/* microseconds */
	uint64_t control;		/* control value */
	uint64_t status;		/* success indicator */
} __aligned(8);

struct acpi_sx_pkg {
	uint8_t		val_pm1a;
	uint8_t		val_pm1b;
	uint16_t	reserved;
} __aligned(8);

struct pm_s_state_data {
	struct acpi_generic_address pm1a_evt;
	struct acpi_generic_address pm1b_evt;
	struct acpi_generic_address pm1a_cnt;
	struct acpi_generic_address pm1b_cnt;
	struct acpi_sx_pkg s3_pkg;
	struct acpi_sx_pkg s5_pkg;
	uint32_t *wake_vector_32;
	uint64_t *wake_vector_64;
} __aligned(8);

/**
 * @brief Info to get a VM interrupt count data
 *
 * the parameter for HC_VM_INTR_MONITOR hypercall
 */
#define MAX_PTDEV_NUM 24U
struct acrn_intr_monitor {
	/** sub command for intr monitor */
	uint32_t cmd;
	/** the count of this buffer to save */
	uint32_t buf_cnt;

	/** the buffer which save each interrupt count */
	uint64_t buffer[MAX_PTDEV_NUM * 2];
} __aligned(8);

/** cmd for intr monitor **/
#define INTR_CMD_GET_DATA 0U
#define INTR_CMD_DELAY_INT 1U

/**
 * @}
 */
#endif /* ACRN_COMMON_H */

/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <acrn/spinlock.h>
#include <asm/io.h>
#include <common/uart16550.h>
#include <acrn/mm.h>
#include <acrn/kernel.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <printf.h>


#include <asm/early_printk.h>

extern DEFINE_PAGE_TABLE(acrn_pgtable);
#define MAX_BDF_LEN 8


struct console_uart {

	union {
		uint16_t port_address;
		void *mmio_base_vaddr;
	};

	spinlock_t rx_lock;
	spinlock_t tx_lock;

	uint32_t reg_width;
};

static struct console_uart uart = {
	.mmio_base_vaddr = NULL,
	.reg_width = 1,
};

typedef uint8_t uart_reg_t;

/**
 * @pre uart->enabled == true
 */
static char uart16550_read_reg(struct console_uart uart, uint16_t reg_idx)
{
	void __iomem *addr = uart.mmio_base_vaddr + (reg_idx * uart.reg_width);

	return readb(addr);
}

/**
 * @pre uart->enabled == true
 */
static void uart16550_write_reg(struct console_uart uart, char val, uint16_t reg_idx)
{
	void __iomem *addr = uart.mmio_base_vaddr + (reg_idx * uart.reg_width);

	writeb(val, addr);
}

static void uart16550_calc_baud_div(uint32_t ref_freq, uint32_t *baud_div_ptr, uint32_t baud_rate_arg)
{
	uint32_t baud_rate = baud_rate_arg;
	uint32_t baud_multiplier = baud_rate < BAUD_460800 ? 16U : 13U;

	if (baud_rate == 0U) {
		baud_rate = BAUD_115200;
	}
	*baud_div_ptr = ref_freq / (baud_multiplier * baud_rate);
}

/**
 * @pre uart->enabled == true
 */
static void uart16550_set_baud_rate(uint32_t baud_rate)
{
	uint32_t baud_div, duart_clock = UART_CLOCK_RATE;
	uart_reg_t temp_reg;

	/* Calculate baud divisor */
	uart16550_calc_baud_div(duart_clock, &baud_div, baud_rate);

	/* Enable DLL and DLM registers for setting the Divisor */
	temp_reg = uart16550_read_reg(uart, UART16550_LCR);
	temp_reg |= LCR_DLAB;
	uart16550_write_reg(uart, temp_reg, UART16550_LCR);

	/* Write the appropriate divisor value */
	uart16550_write_reg(uart, ((baud_div >> 8U) & 0xFFU), UART16550_DLM);
	uart16550_write_reg(uart, (baud_div & 0xFFU), UART16550_DLL);

	/* Disable DLL and DLM registers */
	temp_reg &= ~LCR_DLAB;
	uart16550_write_reg(uart, temp_reg, UART16550_LCR);
}

void uart16550_init(bool early_boot)
{
	/*
	* The FIXMAP in XEN has divided to three partitions for three usages:
	* 1, FIXMAP_CONSOLE for early print
	* 2, FIXMAP_MISC for temporary buffer in copy_from_paddr
	* 3, FIXMAP_ACPI_BEGIN/FIXMAP_ACPI_END for ACPI related.

	* In ACRN, we only use it for early print case. So remove entire fixmap
	* region as we use identical mapping for physical UART.
	*/
	clear_fixmap_pagetable();
	/* use the 1*1 mapping created in setup_pagetables. */
	uart.mmio_base_vaddr = hpa2hva(CONFIG_SERIAL_BASE);

	spin_lock_init(&uart.rx_lock);
	spin_lock_init(&uart.tx_lock);
	/* Enable TX and RX FIFOs */
	uart16550_write_reg(uart, FCR_FIFOE | FCR_RFR | FCR_TFR, UART16550_FCR);

	/* Set-up data bits / parity / stop bits. */
	uart16550_write_reg(uart, (LCR_WL8 | LCR_NB_STOP_BITS_1 | LCR_PARITY_NONE), UART16550_LCR);

	/* Disable interrupts (we use polling) */
	uart16550_write_reg(uart, UART_IER_DISABLE_ALL, UART16550_IER);

	/* Set baud rate */
	uart16550_set_baud_rate(BAUD_115200);

	/* Data terminal ready + Request to send */
	uart16550_write_reg(uart, MCR_RTS | MCR_DTR, UART16550_MCR);
}

char uart16550_getc(void)
{
	char ret = -1;
	uint64_t flags;

	spin_lock_irqsave(&uart.rx_lock, flags);
	/* If a character has been received, read it */
	if ((uart16550_read_reg(uart, UART16550_LSR) & LSR_DR) == LSR_DR) {
		/* Read a character */
		ret = uart16550_read_reg(uart, UART16550_RBR);

	}
	spin_unlock_irqrestore(&uart.rx_lock, flags);
	return ret;
}

/**
 * @pre uart->enabled == true
 */
void uart16550_putc(char c)
{
	uint32_t reg;

	/* Ensure there are no further Transmit buffer write requests */
	do {
		reg = uart16550_read_reg(uart, UART16550_LSR);
	} while ((reg & LSR_THRE) == 0U || (reg & LSR_TEMT) == 0U);
	/* Transmit the character. */
	uart16550_write_reg(uart, c, UART16550_THR);
}

size_t uart16550_puts(const char *buf, uint32_t len)
{
	uint32_t i;
	uint64_t rflags;

	spin_lock_irqsave(&uart.tx_lock, rflags);
	for (i = 0U; i < len; i++) {
		/* Transmit character */
		uart16550_putc(*buf);
		if (*buf == '\n') {
			/* Append '\r', no need change the len */
			uart16550_putc('\r');
		}
		buf++;
	}
	spin_unlock_irqrestore(&uart.tx_lock, rflags);
	return len;
}

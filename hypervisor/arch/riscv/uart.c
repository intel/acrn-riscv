#include <acrn/config.h>
#include <types.h>

#define UART_BASE (volatile uint8_t *)0x10000000

static void write8(volatile unsigned char *addr, char c)
{
	*addr = c;
}

static char read8(volatile unsigned char *addr)
{
	return *addr;
}

static void put_char(char c)
{
	unsigned char t = 0;
	write8(UART_BASE, c);
	while (!t)
		t = *(UART_BASE + 0x5) & 0x20;
}

static void get_char(char *c)
{
	char d;

	do {
		d = read8(UART_BASE + 5);
	} while ((d & 0x1) == 0);

	*c = read8(UART_BASE);
}

char uart16550_getc(void)
{
	char c;

	get_char(&c);
	return c;
}

size_t uart16550_puts(const char *buf, uint32_t len)
{
	int i = 0;
	if (buf == 0)
		return 0;

	for (i = 0U; i < len; i++) {
		if (buf[i] == '\n')
			put_char('\r');
		put_char(buf[i]);
	}

	return len;
}

void uart16550_init(bool early_boot)
{}

void early_putch(char c)
{
	put_char(c);
}

char early_getch(void)
{
	char c;

	get_char(&c);
	return c;
}

void early_printk(char *buf)
{
	int i = 0;
	if (buf == 0)
		return;

	while (buf[i] != '\0') {
		if (buf[i] == '\n')
			put_char('\r');
		put_char(buf[i++]);
	}
}

/* The initial log level*/
void npk_log_write(const char *buf, size_t buf_len)
{}

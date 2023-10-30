#define UART_BASE (volatile unsigned int *)0x10000000

static void write32(volatile unsigned int *addr, char c)
{
	*addr = c;
}

static void write8(volatile unsigned char *addr, char c)
{
	*addr = c;
}

static char read8(volatile unsigned char *addr)
{
	return *addr;
}

static void init_uart()
{
	write32(UART_BASE + 0xe, 0x10);
}

static void put_char(char c)
{
	unsigned char t = 0;
	write8((volatile unsigned char *)UART_BASE, c);
	while (!t)
		t = *((volatile unsigned char *)UART_BASE + 0x5) & 0x20;
}

static void get_char(char *c)
{
	char d;

	do {
		//d = read8((volatile unsigned char *)(UART_BASE + 5));
		d = read8((volatile unsigned char *)(0x10000005));
	} while ((d & 0x1) == 0);

	*c = read8((volatile unsigned char *)(UART_BASE));
}

void early_putch(char *c)
{
	put_char(c);
}

char early_getch(void)
{
	char c;

	get_char(&c);
	return c;
}

void printk(char *buf)
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

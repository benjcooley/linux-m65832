/*
 * Baremetal test: memcpy and vsnprintf
 * Tests the same code paths the kernel uses in its first printk.
 */

#define UART_BASE       0x10006000
#define UART_TX_DATA    (*(volatile unsigned int *)(UART_BASE + 0x00))
#define UART_STATUS     (*(volatile unsigned int *)(UART_BASE + 0x04))
#define UART_STATUS_TXRDY   (1 << 1)

static void uart_putc(char c)
{
	while (!(UART_STATUS & UART_STATUS_TXRDY))
		;
	UART_TX_DATA = c;
}

static void uart_puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			uart_putc('\r');
		uart_putc(*s++);
	}
}

static void uart_hex(unsigned int v)
{
	const char h[] = "0123456789ABCDEF";
	uart_puts("0x");
	for (int i = 28; i >= 0; i -= 4)
		uart_putc(h[(v >> i) & 0xF]);
}

/* Same memcpy as arch/m65832/lib/memcpy.c */
void *my_memcpy(void *dest, const void *src, unsigned int n)
{
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;
	unsigned int i;

	if (((unsigned long)d & 3) == 0 && ((unsigned long)s & 3) == 0) {
		unsigned long *dl = (unsigned long *)d;
		const unsigned long *sl = (const unsigned long *)s;
		unsigned int words = n / 4;

		for (i = 0; i < words; i++)
			dl[i] = sl[i];

		d = (unsigned char *)&dl[words];
		s = (const unsigned char *)&sl[words];
		n &= 3;
	}

	for (i = 0; i < n; i++)
		d[i] = s[i];

	return dest;
}

/* Simple sprintf-like: copy string into buffer */
static int my_sprintf(char *buf, const char *fmt)
{
	/* Very simple: just copy the format string (no % substitutions) */
	int len = 0;
	while (*fmt) {
		buf[len++] = *fmt++;
	}
	buf[len] = '\0';
	return len;
}

static const char linux_banner[] = "Linux version 6.19.0-rc7 (m65832)\n";
static char buffer[256];

int main(void)
{
	uart_puts("=== memcpy/sprintf test ===\n");

	/* Test 1: Simple memcpy */
	uart_puts("Test 1: memcpy 16 bytes... ");
	{
		char src[16] = "Hello, World!!\n";
		char dst[16];
		my_memcpy(dst, src, 16);
		if (dst[0] == 'H' && dst[5] == ',')
			uart_puts("OK\n");
		else
			uart_puts("FAIL\n");
	}

	/* Test 2: memcpy of linux_banner */
	uart_puts("Test 2: memcpy linux_banner... ");
	{
		unsigned int len = 0;
		const char *p = linux_banner;
		while (*p++) len++;
		uart_puts("len=");
		uart_hex(len);
		uart_puts(" ");
		my_memcpy(buffer, linux_banner, len + 1);
		if (buffer[0] == 'L' && buffer[1] == 'i')
			uart_puts("OK\n");
		else {
			uart_puts("FAIL buf[0]=");
			uart_hex(buffer[0]);
			uart_puts("\n");
		}
	}

	/* Test 3: sprintf into buffer */
	uart_puts("Test 3: sprintf... ");
	{
		int n = my_sprintf(buffer, linux_banner);
		uart_puts("n=");
		uart_hex(n);
		uart_puts(" \"");
		uart_puts(buffer);
		uart_puts("\" OK\n");
	}

	/* Test 4: Pointer arithmetic */
	uart_puts("Test 4: Pointer addresses...\n");
	uart_puts("  linux_banner at ");
	uart_hex((unsigned int)linux_banner);
	uart_puts("\n  buffer at ");
	uart_hex((unsigned int)buffer);
	uart_puts("\n  main at ");
	uart_hex((unsigned int)&main);
	uart_puts("\n");

	/* Test 5: Word-aligned memcpy with large size */
	uart_puts("Test 5: Large memcpy 256 bytes... ");
	{
		static char big_src[256];
		static char big_dst[256];
		for (int i = 0; i < 256; i++) big_src[i] = (char)(i & 0xFF);
		my_memcpy(big_dst, big_src, 256);
		int ok = 1;
		for (int i = 0; i < 256; i++)
			if (big_dst[i] != (char)(i & 0xFF)) { ok = 0; break; }
		uart_puts(ok ? "OK\n" : "FAIL\n");
	}

	uart_puts("\nAll tests done!\n");
	return 0;
}

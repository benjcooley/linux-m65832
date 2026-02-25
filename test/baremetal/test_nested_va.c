/*
 * Test nested va_list passing - matches printk's pattern:
 * printk(fmt, ...) -> vprintk(fmt, va) -> vsnprintf(buf, size, fmt, va)
 */
#include <stdarg.h>

#define UART_TX  (*(volatile unsigned int *)0x10006000)
#define UART_ST  (*(volatile unsigned int *)0x10006004)
static void putc(char c) { while (!(UART_ST & 2)); UART_TX = c; }
static void puts(const char *s) { while (*s) { if (*s=='\n') putc('\r'); putc(*s++); } }
static void puthex(unsigned int v) {
	const char h[]="0123456789ABCDEF";
	puts("0x");
	for (int i=28;i>=0;i-=4) putc(h[(v>>i)&0xF]);
}

/* Simulates vsnprintf's %s handling */
static int inner_format(char *buf, int size, const char *fmt, va_list ap)
{
	int pos = 0;
	while (*fmt && pos < size - 1) {
		if (*fmt == '%' && *(fmt+1) == 's') {
			const char *s = va_arg(ap, const char *);
			puts("  inner va_arg got: ");
			puthex((unsigned int)s);
			puts("\n");
			if (s) {
				while (*s && pos < size - 1)
					buf[pos++] = *s++;
			}
			fmt += 2;
		} else {
			buf[pos++] = *fmt++;
		}
	}
	buf[pos] = '\0';
	return pos;
}

/* Simulates vprintk_store: calls vsnprintf twice with va_copy */
static int middle_layer(const char *fmt, va_list args)
{
	char buf1[8];
	char buf2[256];
	va_list args2;
	int n;

	/* First call with va_copy (like vprintk_store line 2254) */
	va_copy(args2, args);
	puts("First vsnprintf (va_copy):\n");
	n = inner_format(buf1, sizeof(buf1), fmt, args2);
	va_end(args2);
	puts("  result: \"");
	puts(buf1);
	puts("\" len=");
	puthex(n);
	puts("\n");

	/* Second call with original args (like printk_sprint line 2312) */
	puts("Second vsnprintf (original args):\n");
	n = inner_format(buf2, sizeof(buf2), fmt, args);
	puts("  result: \"");
	puts(buf2);
	puts("\" len=");
	puthex(n);
	puts("\n");

	return n;
}

/* Simulates printk */
static int my_printk(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = middle_layer(fmt, args);
	va_end(args);
	return ret;
}

static const char *banner = "Linux version 6.19.0-rc7-m65832";

int main(void)
{
	puts("=== Nested va_list test ===\n");

	puts("Test 1: my_printk(\"%%s\", banner)\n");
	puts("  banner at ");
	puthex((unsigned int)banner);
	puts("\n");
	my_printk("%s", banner);

	puts("\nTest 2: my_printk(\"Hello %%s v%%d\", \"World\", 42)\n");
	/* This one we skip since we only handle %s */

	puts("\nAll nested va tests done!\n");
	return 0;
}

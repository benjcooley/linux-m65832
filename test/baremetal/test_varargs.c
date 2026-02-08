/*
 * Baremetal test: variadic functions (va_list)
 * Tests the same calling convention printk uses.
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

/* Simple variadic: sum N integers */
static int sum_ints(int count, ...)
{
	va_list ap;
	int total = 0;
	va_start(ap, count);
	for (int i = 0; i < count; i++)
		total += va_arg(ap, int);
	va_end(ap);
	return total;
}

/* Variadic string: print format with %s and %d */
static void my_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	while (*fmt) {
		if (*fmt == '%' && *(fmt+1) == 's') {
			const char *s = va_arg(ap, const char *);
			puts(s ? s : "(null)");
			fmt += 2;
		} else if (*fmt == '%' && *(fmt+1) == 'd') {
			int v = va_arg(ap, int);
			if (v < 0) { putc('-'); v = -v; }
			if (v == 0) putc('0');
			else {
				char buf[12]; int n = 0;
				while (v > 0) { buf[n++] = '0' + (v % 10); v /= 10; }
				while (n > 0) putc(buf[--n]);
			}
			fmt += 2;
		} else {
			if (*fmt == '\n') putc('\r');
			putc(*fmt++);
		}
	}
	va_end(ap);
}

static const char *banner = "Linux version M65832";

int main(void)
{
	puts("=== Variadic function test ===\n");

	/* Test 1: sum integers */
	puts("Test 1: sum(3, 10, 20, 30) = ");
	int s = sum_ints(3, 10, 20, 30);
	puthex(s);
	puts(s == 60 ? " OK\n" : " FAIL\n");

	/* Test 2: sum more integers */
	puts("Test 2: sum(5, 1,2,3,4,5) = ");
	s = sum_ints(5, 1, 2, 3, 4, 5);
	puthex(s);
	puts(s == 15 ? " OK\n" : " FAIL\n");

	/* Test 3: printf with %s */
	puts("Test 3: printf %%s: ");
	my_printf("Hello %s!\n", "World");

	/* Test 4: printf with %d */
	puts("Test 4: printf %%d: ");
	my_printf("Value = %d\n", 42);

	/* Test 5: printf with %s using banner pointer */
	puts("Test 5: printf banner: ");
	my_printf("%s\n", banner);

	/* Test 6: mixed */
	puts("Test 6: mixed: ");
	my_printf("%s version %d.%d\n", "M65832", 6, 19);

	puts("\nAll varargs tests done!\n");
	return 0;
}

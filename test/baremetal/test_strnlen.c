/*
 * Test strnlen at -O2 - the kernel's vsnprintf is stuck in strnlen
 */
#define UART_TX  (*(volatile unsigned int *)0x10006000)
#define UART_ST  (*(volatile unsigned int *)0x10006004)
static void putc(char c) { while (!(UART_ST & 2)); UART_TX = c; }
static void puts(const char *s) { while (*s) { if (*s=='\n') putc('\r'); putc(*s++); } }
static void puthex(unsigned int v) {
	const char h[]="0123456789ABCDEF";
	puts("0x");
	for (int i=28;i>=0;i-=4) putc(h[(v>>i)&0xF]);
}

/* Same as kernel lib/string.c strnlen */
unsigned int my_strnlen(const char *s, unsigned int count)
{
	const char *sc;
	for (sc = s; count-- && *sc != '\0'; ++sc)
		;
	return sc - s;
}

static const char banner[] = "Linux version 6.19.0-rc7-m65832-dirty (user@host) #1 Sat Feb 8 2026";
static const char short_str[] = "Hello";
static const char fmt[] = "%s";

int main(void)
{
	puts("=== strnlen test ===\n");

	puts("Test 1: strnlen(short, 100) = ");
	unsigned int n = my_strnlen(short_str, 100);
	puthex(n);
	puts(n == 5 ? " OK\n" : " FAIL\n");

	puts("Test 2: strnlen(banner, 1000) = ");
	n = my_strnlen(banner, 1000);
	puthex(n);
	puts(n == 67 ? " OK\n" : " FAIL\n");

	puts("Test 3: strnlen(fmt, 10) = ");
	n = my_strnlen(fmt, 10);
	puthex(n);
	puts(n == 2 ? " OK\n" : " FAIL\n");

	puts("Test 4: strnlen with count=0 = ");
	n = my_strnlen(banner, 0);
	puthex(n);
	puts(n == 0 ? " OK\n" : " FAIL\n");

	puts("Test 5: strnlen truncated = ");
	n = my_strnlen(banner, 5);
	puthex(n);
	puts(n == 5 ? " OK\n" : " FAIL\n");

	puts("\nAll strnlen tests done!\n");
	return 0;
}

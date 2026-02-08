/*
 * Baremetal UART test for M65832
 *
 * Tests UART output at DE25 platform address 0x10006000.
 * Run with: m65832emu --system uart_test.elf
 */

/* DE25 UART registers - emulator register layout */
#define UART_BASE       0x10006000
#define UART_STATUS     (*(volatile unsigned int *)(UART_BASE + 0x04))
#define UART_CTRL       (*(volatile unsigned int *)(UART_BASE + 0x08))
#define UART_TX_DATA    (*(volatile unsigned int *)(UART_BASE + 0x10))
#define UART_RX_DATA    (*(volatile unsigned int *)(UART_BASE + 0x14))

#define UART_STATUS_RXRDY   (1 << 0)
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

static void uart_put_hex(unsigned int val)
{
	const char hex[] = "0123456789ABCDEF";
	uart_puts("0x");
	for (int i = 28; i >= 0; i -= 4)
		uart_putc(hex[(val >> i) & 0xF]);
}

int main(void)
{
	/* Test 1: Simple string output */
	uart_puts("=== M65832 UART Baremetal Test ===\n");
	uart_puts("Test 1: String output... OK\n");

	/* Test 2: Hex output */
	uart_puts("Test 2: Hex output: ");
	uart_put_hex(0xDEADBEEF);
	uart_puts(" ... OK\n");

	/* Test 3: Status register check */
	uart_puts("Test 3: UART status: ");
	uart_put_hex(UART_STATUS);
	uart_puts("\n");

	/* Test 4: Character by character */
	uart_puts("Test 4: ABC -> ");
	uart_putc('A');
	uart_putc('B');
	uart_putc('C');
	uart_puts(" ... OK\n");

	uart_puts("\nAll UART tests passed!\n");

	return 0;
}

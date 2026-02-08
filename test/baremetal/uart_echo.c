/*
 * Baremetal UART echo test for M65832
 *
 * Echoes characters typed on the terminal back with a prompt.
 * Tests both TX and RX paths.
 *
 * Run with: m65832emu --system --raw uart_echo.elf
 * Type characters, see them echoed. Press 'q' to quit.
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

static int uart_rx_ready(void)
{
	return (UART_STATUS & UART_STATUS_RXRDY) != 0;
}

static char uart_getc(void)
{
	while (!uart_rx_ready())
		;
	return (char)(UART_RX_DATA & 0xFF);
}

int main(void)
{
	char c;

	uart_puts("\n=== M65832 UART Echo Test ===\n");
	uart_puts("Type characters to see them echoed.\n");
	uart_puts("Press 'q' to quit.\n\n");
	uart_puts("> ");

	while (1) {
		c = uart_getc();

		if (c == 'q' || c == 'Q') {
			uart_puts("\nGoodbye!\n");
			break;
		}

		/* Echo the character */
		uart_putc(c);

		/* Newline handling */
		if (c == '\r' || c == '\n') {
			uart_putc('\n');
			uart_puts("> ");
		}
	}

	return 0;
}

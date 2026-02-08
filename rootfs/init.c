/*
 * Minimal /sbin/init for M65832 Linux
 *
 * Prints a message to the console and loops forever.
 * This is the first userspace program the kernel runs.
 */

/* Direct UART output - no libc needed for initial test */
#define UART_BASE       0x10006000
#define UART_STATUS     (*(volatile unsigned int *)(UART_BASE + 0x04))
#define UART_TX_DATA    (*(volatile unsigned int *)(UART_BASE + 0x10))
#define UART_STATUS_TXRDY   (1 << 1)

static void putc(char c)
{
	while (!(UART_STATUS & UART_STATUS_TXRDY))
		;
	UART_TX_DATA = c;
}

static void puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			putc('\r');
		putc(*s++);
	}
}

void _start(void)
{
	puts("\n\n");
	puts("========================================\n");
	puts("  M65832 Linux - userspace init running!\n");
	puts("========================================\n");
	puts("\n");
	puts("Hello from PID 1!\n");
	puts("The kernel has booted successfully.\n");
	puts("\n");
	puts("Init halting (no shell available yet).\n");

	/* Loop forever - can't exit PID 1 */
	for (;;)
		;
}

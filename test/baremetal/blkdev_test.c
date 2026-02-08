/*
 * Baremetal block device test for M65832
 *
 * Tests block device DMA read/write at DE25 SD address 0x1000A000.
 * Run with: m65832emu --system --disk test.img blkdev_test.elf
 *
 * Create test disk: dd if=/dev/zero of=test.img bs=512 count=2048
 */

/* DE25 SD/Block device registers (offsets from base 0x1000A000) */
#define BLKDEV_BASE     0x1000A000
#define BLKDEV_CTRL     (*(volatile unsigned int *)(BLKDEV_BASE + 0x00))
#define BLKDEV_STATUS   (*(volatile unsigned int *)(BLKDEV_BASE + 0x04))
#define BLKDEV_ARG      (*(volatile unsigned int *)(BLKDEV_BASE + 0x0C))  /* sector_lo */
#define BLKDEV_RESP0    (*(volatile unsigned int *)(BLKDEV_BASE + 0x10))  /* sector_hi */
#define BLKDEV_DMA_ADDR (*(volatile unsigned int *)(BLKDEV_BASE + 0x38))
#define BLKDEV_BLKCNT   (*(volatile unsigned int *)(BLKDEV_BASE + 0x28))
#define BLKDEV_CAPACITY_LO (*(volatile unsigned int *)(BLKDEV_BASE + 0x40))
#define BLKDEV_CAPACITY_HI (*(volatile unsigned int *)(BLKDEV_BASE + 0x44))

/* Status bits */
#define STATUS_PRESENT  (1 << 0)
#define STATUS_READY    (1 << 1)
#define STATUS_BUSY     (1 << 2)
#define STATUS_ERROR    (1 << 3)
#define STATUS_COMPLETE (1 << 8)

/* Commands */
#define CMD_READ        0x01
#define CMD_WRITE       0x02
#define CMD_FLUSH       0x03

/* UART for output (TX at offset 0x10 per emulator layout) */
#define UART_BASE       0x10006000
#define UART_TX_DATA    (*(volatile unsigned int *)(UART_BASE + 0x10))
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

static void uart_put_hex(unsigned int val)
{
	const char hex[] = "0123456789ABCDEF";
	uart_puts("0x");
	for (int i = 28; i >= 0; i -= 4)
		uart_putc(hex[(val >> i) & 0xF]);
}

/* DMA buffer in low memory */
static unsigned char dma_buf[512] __attribute__((aligned(512)));

static void wait_ready(void)
{
	while (BLKDEV_STATUS & STATUS_BUSY)
		;
}

int main(void)
{
	unsigned int status;

	uart_puts("=== M65832 Block Device Baremetal Test ===\n");

	/* Test 1: Check device presence */
	status = BLKDEV_STATUS;
	uart_puts("Test 1: Status register: ");
	uart_put_hex(status);
	uart_puts("\n");

	if (!(status & STATUS_PRESENT)) {
		uart_puts("  No disk present. Run with --disk test.img\n");
		return 1;
	}
	uart_puts("  Disk present: OK\n");

	/* Test 2: Read capacity */
	uart_puts("Test 2: Capacity: ");
	uart_put_hex(BLKDEV_CAPACITY_LO);
	uart_puts(" sectors\n");

	/* Test 3: Write a test pattern to sector 0 */
	uart_puts("Test 3: Writing test pattern to sector 0...\n");
	for (int i = 0; i < 512; i++)
		dma_buf[i] = (unsigned char)(i & 0xFF);

	/* Set up DMA write */
	BLKDEV_ARG = 0;           /* Sector 0 */
	BLKDEV_RESP0 = 0;         /* High bits = 0 */
	BLKDEV_DMA_ADDR = (unsigned int)dma_buf;
	BLKDEV_BLKCNT = 1;        /* 1 sector */
	BLKDEV_CTRL = CMD_WRITE;  /* Start write */
	wait_ready();

	status = BLKDEV_STATUS;
	if (status & STATUS_ERROR) {
		uart_puts("  Write ERROR: ");
		uart_put_hex(status);
		uart_puts("\n");
		return 2;
	}
	uart_puts("  Write OK\n");

	/* Test 4: Read it back */
	uart_puts("Test 4: Reading back sector 0...\n");
	for (int i = 0; i < 512; i++)
		dma_buf[i] = 0;  /* Clear buffer */

	BLKDEV_ARG = 0;
	BLKDEV_RESP0 = 0;
	BLKDEV_DMA_ADDR = (unsigned int)dma_buf;
	BLKDEV_BLKCNT = 1;
	BLKDEV_CTRL = CMD_READ;
	wait_ready();

	status = BLKDEV_STATUS;
	if (status & STATUS_ERROR) {
		uart_puts("  Read ERROR: ");
		uart_put_hex(status);
		uart_puts("\n");
		return 3;
	}

	/* Test 5: Verify data */
	uart_puts("Test 5: Verifying data...\n");
	int errors = 0;
	for (int i = 0; i < 512; i++) {
		if (dma_buf[i] != (unsigned char)(i & 0xFF))
			errors++;
	}

	if (errors) {
		uart_puts("  VERIFY FAILED: ");
		uart_put_hex(errors);
		uart_puts(" byte mismatches\n");
		return 4;
	}
	uart_puts("  Verify OK - all 512 bytes match\n");

	/* Test 6: First bytes of read data */
	uart_puts("Test 6: First 16 bytes: ");
	for (int i = 0; i < 16; i++) {
		uart_put_hex(dma_buf[i]);
		uart_putc(' ');
	}
	uart_puts("\n");

	uart_puts("\nAll block device tests passed!\n");
	return 0;
}

// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 UART serial driver
 *
 * Simple UART for the M65832 platform (DE25).
 * Provides earlycon, console, and full tty serial port.
 *
 * Register layout (offsets from base):
 *   0x00  DATA     - TX/RX data (legacy, not used)
 *   0x04  STATUS   - Status register (R)
 *   0x08  CTRL     - Control register (R/W)
 *   0x0C  BAUD     - Baud rate divisor (R/W)
 *   0x10  TX_DATA  - Transmit data (W)
 *   0x14  RX_DATA  - Receive data (R)
 */

#include <linux/console.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>

#define DRIVER_NAME	"m65832-uart"
#define M65832_UART_NR	1

/* Register offsets */
#define M65832_UART_STATUS	0x04
#define M65832_UART_CTRL	0x08
#define M65832_UART_BAUD	0x0C
#define M65832_UART_TX		0x10
#define M65832_UART_RX		0x14

/* Status bits */
#define STATUS_RXRDY		(1 << 0)
#define STATUS_TXRDY		(1 << 1)
#define STATUS_RXFULL		(1 << 2)
#define STATUS_TXEMPTY		(1 << 3)
#define STATUS_RXERR		(1 << 4)
#define STATUS_TXBUSY		(1 << 5)

/* Control bits */
#define CTRL_RXIE		(1 << 0)
#define CTRL_TXIE		(1 << 1)
#define CTRL_ENABLE		(1 << 2)

/* Default base address (DE25 platform) */
#define M65832_UART_BASE	0x10006000
#define M65832_UART_SIZE	0x1000
#define M65832_UART_IRQ		7

/* ======================================================================
 * Low-level register access
 * ====================================================================== */

static inline u32 m65832_uart_readl(struct uart_port *port, unsigned int off)
{
	return readl(port->membase + off);
}

static inline void m65832_uart_writel(struct uart_port *port, unsigned int off,
				      u32 val)
{
	writel(val, port->membase + off);
}

/* ======================================================================
 * UART port operations
 * ====================================================================== */

static unsigned int m65832_uart_tx_empty(struct uart_port *port)
{
	return (m65832_uart_readl(port, M65832_UART_STATUS) & STATUS_TXEMPTY)
		? TIOCSER_TEMT : 0;
}

static void m65832_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	/* No modem control lines */
}

static unsigned int m65832_uart_get_mctrl(struct uart_port *port)
{
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void m65832_uart_stop_tx(struct uart_port *port)
{
	u32 ctrl = m65832_uart_readl(port, M65832_UART_CTRL);
	ctrl &= ~CTRL_TXIE;
	m65832_uart_writel(port, M65832_UART_CTRL, ctrl);
}

static void m65832_uart_start_tx(struct uart_port *port)
{
	unsigned char ch;

	while (uart_fifo_get(port, &ch)) {
		if (!(m65832_uart_readl(port, M65832_UART_STATUS) & STATUS_TXRDY))
			break;
		m65832_uart_writel(port, M65832_UART_TX, ch);
	}
}

static void m65832_uart_stop_rx(struct uart_port *port)
{
	u32 ctrl = m65832_uart_readl(port, M65832_UART_CTRL);
	ctrl &= ~CTRL_RXIE;
	m65832_uart_writel(port, M65832_UART_CTRL, ctrl);
}

static void m65832_uart_break_ctl(struct uart_port *port, int break_state)
{
	/* No break support */
}

static void m65832_uart_rx_chars(struct uart_port *port)
{
	u32 status;

	while ((status = m65832_uart_readl(port, M65832_UART_STATUS)) & STATUS_RXRDY) {
		u32 ch = m65832_uart_readl(port, M65832_UART_RX);
		unsigned int flag = TTY_NORMAL;

		port->icount.rx++;

		if (status & STATUS_RXERR) {
			port->icount.overrun++;
			flag = TTY_OVERRUN;
		}

		if (uart_handle_sysrq_char(port, ch & 0xFF))
			continue;

		uart_insert_char(port, status, STATUS_RXERR, ch & 0xFF, flag);
	}

	tty_flip_buffer_push(&port->state->port);
}

static irqreturn_t m65832_uart_irq(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	u32 status;

	uart_port_lock(port);

	status = m65832_uart_readl(port, M65832_UART_STATUS);

	if (status & STATUS_RXRDY)
		m65832_uart_rx_chars(port);

	if (status & STATUS_TXRDY)
		m65832_uart_start_tx(port);

	uart_port_unlock(port);

	return IRQ_HANDLED;
}

static int m65832_uart_startup(struct uart_port *port)
{
	int ret;

	ret = request_irq(port->irq, m65832_uart_irq, 0, DRIVER_NAME, port);
	if (ret)
		return ret;

	/* Enable UART and RX interrupt */
	m65832_uart_writel(port, M65832_UART_CTRL, CTRL_ENABLE | CTRL_RXIE);

	return 0;
}

static void m65832_uart_shutdown(struct uart_port *port)
{
	/* Disable interrupts */
	m65832_uart_writel(port, M65832_UART_CTRL, 0);
	free_irq(port->irq, port);
}

static void m65832_uart_set_termios(struct uart_port *port,
				    struct ktermios *termios,
				    const struct ktermios *old)
{
	unsigned int baud;

	baud = uart_get_baud_rate(port, termios, old, 9600, 115200);
	uart_update_timeout(port, termios->c_cflag, baud);
}

static const char *m65832_uart_type(struct uart_port *port)
{
	return "M65832-UART";
}

static void m65832_uart_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE)
		port->type = PORT_UNKNOWN + 1;
}

static void m65832_uart_release_port(struct uart_port *port)
{
}

static int m65832_uart_request_port(struct uart_port *port)
{
	return 0;
}

static const struct uart_ops m65832_uart_ops = {
	.tx_empty	= m65832_uart_tx_empty,
	.set_mctrl	= m65832_uart_set_mctrl,
	.get_mctrl	= m65832_uart_get_mctrl,
	.stop_tx	= m65832_uart_stop_tx,
	.start_tx	= m65832_uart_start_tx,
	.stop_rx	= m65832_uart_stop_rx,
	.break_ctl	= m65832_uart_break_ctl,
	.startup	= m65832_uart_startup,
	.shutdown	= m65832_uart_shutdown,
	.set_termios	= m65832_uart_set_termios,
	.type		= m65832_uart_type,
	.config_port	= m65832_uart_config_port,
	.release_port	= m65832_uart_release_port,
	.request_port	= m65832_uart_request_port,
};

/* ======================================================================
 * Console support (for printk output)
 * ====================================================================== */

static struct uart_port m65832_uart_port;

static void m65832_console_putchar(struct uart_port *port, unsigned char ch)
{
	while (!(m65832_uart_readl(port, M65832_UART_STATUS) & STATUS_TXRDY))
		cpu_relax();
	m65832_uart_writel(port, M65832_UART_TX, ch);
}

static void m65832_console_write(struct console *co, const char *s,
				 unsigned int count)
{
	struct uart_port *port = &m65832_uart_port;

	uart_console_write(port, s, count, m65832_console_putchar);
}

static int m65832_console_setup(struct console *co, char *options)
{
	struct uart_port *port = &m65832_uart_port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (!port->membase) {
		port->mapbase = M65832_UART_BASE;
		port->membase = ioremap(M65832_UART_BASE, M65832_UART_SIZE);
		if (!port->membase)
			return -ENOMEM;
	}

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver m65832_uart_driver;

static struct console m65832_console = {
	.name	= "ttyM",
	.write	= m65832_console_write,
	.device	= uart_console_device,
	.setup	= m65832_console_setup,
	.flags	= CON_PRINTBUFFER,
	.index	= -1,
	.data	= &m65832_uart_driver,
};

/* ======================================================================
 * Early console (earlycon) for boot messages before full driver init
 * ====================================================================== */

static void m65832_earlycon_write(struct console *co, const char *s,
				  unsigned int count)
{
	/* Direct MMIO write, no port structure needed */
	volatile u32 *status = (volatile u32 *)(M65832_UART_BASE + M65832_UART_STATUS);
	volatile u32 *tx = (volatile u32 *)(M65832_UART_BASE + M65832_UART_TX);

	while (count--) {
		while (!(*status & STATUS_TXRDY))
			;
		if (*s == '\n') {
			*tx = '\r';
			while (!(*status & STATUS_TXRDY))
				;
		}
		*tx = *s++;
	}
}

static int __init m65832_earlycon_setup(struct earlycon_device *dev,
					const char *options)
{
	dev->con->write = m65832_earlycon_write;
	return 0;
}

EARLYCON_DECLARE(m65832, m65832_earlycon_setup);

/* ======================================================================
 * UART driver registration
 * ====================================================================== */

static struct uart_driver m65832_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= DRIVER_NAME,
	.dev_name	= "ttyM",
	.nr		= M65832_UART_NR,
	.cons		= &m65832_console,
};

static int __init m65832_uart_init(void)
{
	struct uart_port *port = &m65832_uart_port;
	int ret;

	ret = uart_register_driver(&m65832_uart_driver);
	if (ret)
		return ret;

	/* Set up the port */
	port->type = PORT_UNKNOWN + 1;
	port->iotype = UPIO_MEM32;
	port->mapbase = M65832_UART_BASE;
	port->membase = ioremap(M65832_UART_BASE, M65832_UART_SIZE);
	if (!port->membase) {
		uart_unregister_driver(&m65832_uart_driver);
		return -ENOMEM;
	}
	port->irq = M65832_UART_IRQ;
	port->uartclk = 50000000;
	port->fifosize = 1;
	port->ops = &m65832_uart_ops;
	port->line = 0;
	port->flags = UPF_BOOT_AUTOCONF;

	ret = uart_add_one_port(&m65832_uart_driver, port);
	if (ret) {
		iounmap(port->membase);
		uart_unregister_driver(&m65832_uart_driver);
		return ret;
	}

	pr_info("m65832-uart: registered at 0x%08lx, IRQ %d\n",
		(unsigned long)port->mapbase, port->irq);

	return 0;
}

static void __exit m65832_uart_exit(void)
{
	uart_remove_one_port(&m65832_uart_driver, &m65832_uart_port);
	iounmap(m65832_uart_port.membase);
	uart_unregister_driver(&m65832_uart_driver);
}

module_init(m65832_uart_init);
module_exit(m65832_uart_exit);

MODULE_DESCRIPTION("M65832 UART serial driver");
MODULE_LICENSE("GPL");

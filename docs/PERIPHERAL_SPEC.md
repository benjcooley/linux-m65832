# M65832 Peripheral Specification

Hardware register definitions for UART and SD Card controllers.
These specifications define the MMIO interface for both VHDL/FPGA and C emulator implementations.

## 1. UART Controller

**Base Address:** `0x1000_6000`  
**Size:** 16 bytes  
**IRQ:** 7 (UART RX ready)

### 1.1 Register Map

| Offset | Name       | Access | Reset | Description |
|--------|------------|--------|-------|-------------|
| 0x00   | DATA       | R/W    | 0x00  | TX/RX Data Register |
| 0x04   | STATUS     | R      | 0x02  | Status Register |
| 0x08   | CTRL       | R/W    | 0x00  | Control Register |
| 0x0C   | BAUD       | R/W    | 0x1B  | Baud Rate Divisor (default: 115200 @ 50MHz) |

### 1.2 DATA Register (0x00)

```
Bits [7:0]:  TX_DATA / RX_DATA
             Write: Byte to transmit (if TXRDY=1)
             Read:  Received byte (if RXRDY=1)
Bits [31:8]: Reserved (read as 0)
```

### 1.3 STATUS Register (0x04) - Read Only

```
Bit 0: RXRDY    - RX data ready (1 = byte available in RX buffer)
Bit 1: TXRDY    - TX ready (1 = can accept byte for transmission)
Bit 2: RXFULL   - RX FIFO full (optional, for FIFO implementations)
Bit 3: TXEMPTY  - TX FIFO empty (all data transmitted)
Bit 4: RXERR    - RX error (framing/parity error, write 1 to STATUS to clear)
Bit 5: TXBUSY   - TX shift register active
Bits [31:6]: Reserved
```

### 1.4 CTRL Register (0x08)

```
Bit 0: RXIE     - RX interrupt enable (IRQ when RXRDY=1)
Bit 1: TXIE     - TX interrupt enable (IRQ when TXRDY=1)  
Bit 2: ENABLE   - UART enable (1 = enabled)
Bit 3: LOOPBACK - Loopback mode for testing
Bits [31:4]: Reserved
```

### 1.5 BAUD Register (0x0C)

```
Bits [15:0]: DIVISOR - Baud rate divisor
             Baud rate = CLK_FREQ / (16 * (DIVISOR + 1))
             
             For 50 MHz clock:
             - 115200 baud: DIVISOR = 26 (0x1A) → actual 116279 baud
             - 9600 baud:   DIVISOR = 325 (0x145)
             
Bits [31:16]: Reserved
```

### 1.6 VHDL Implementation Notes

```vhdl
-- SPI interface to board UART (directly from FPGA pins)
-- DE2-115: UART_TXD (GPIO_0[0]), UART_RXD (GPIO_0[1])
-- DE25-Nano: USB-C UART pins

entity m65832_uart is
    generic (
        CLK_FREQ    : integer := 50_000_000;
        FIFO_DEPTH  : integer := 16
    );
    port (
        clk         : in  std_logic;
        rst_n       : in  std_logic;
        
        -- CPU bus interface
        cs          : in  std_logic;
        addr        : in  std_logic_vector(3 downto 0);
        we          : in  std_logic;
        wdata       : in  std_logic_vector(31 downto 0);
        rdata       : out std_logic_vector(31 downto 0);
        
        -- Interrupt output
        irq         : out std_logic;
        
        -- UART pins
        uart_tx     : out std_logic;
        uart_rx     : in  std_logic
    );
end entity;
```

---

## 2. SD Card Controller (SPI Mode)

**Base Address:** `0x1000_A000`  
**Size:** 64 bytes  
**IRQ:** 11 (SD operation complete)

The SD controller uses SPI mode for simplicity. This requires only 4 pins and works with all SD/SDHC cards.

### 2.1 Register Map

| Offset | Name       | Access | Reset | Description |
|--------|------------|--------|-------|-------------|
| 0x00   | CTRL       | R/W    | 0x00  | Control Register |
| 0x04   | STATUS     | R      | 0x00  | Status Register |
| 0x08   | CMD        | R/W    | 0x00  | Command to send |
| 0x0C   | ARG        | R/W    | 0x00  | Command argument |
| 0x10   | RESP0      | R      | 0x00  | Response word 0 |
| 0x14   | RESP1      | R      | 0x00  | Response word 1 (R2 responses) |
| 0x18   | RESP2      | R      | 0x00  | Response word 2 (R2 responses) |
| 0x1C   | RESP3      | R      | 0x00  | Response word 3 (R2 responses) |
| 0x20   | DATA       | R/W    | 0x00  | Data register (32-bit access to FIFO) |
| 0x24   | BLKSIZE    | R/W    | 0x200 | Block size (default 512) |
| 0x28   | BLKCNT     | R/W    | 0x01  | Block count for multi-block |
| 0x2C   | TIMEOUT    | R/W    | 0xFFFF| Timeout value (in clock cycles / 256) |
| 0x30   | CLKDIV     | R/W    | 0x7C  | SPI clock divisor |
| 0x34   | FIFOCNT    | R      | 0x00  | FIFO word count |

### 2.2 CTRL Register (0x00)

```
Bit 0: ENABLE     - Controller enable
Bit 1: CARD_SEL   - Assert chip select (directly control CS line)
Bit 2: START_CMD  - Start command (write 1 to initiate, auto-clears)
Bit 3: START_RD   - Start block read (write 1 to initiate)
Bit 4: START_WR   - Start block write (write 1 to initiate)
Bit 5: ABORT      - Abort current operation
Bit 6: RESET_FIFO - Reset data FIFO
Bit 7: IRQ_EN     - Interrupt enable
Bits [31:8]: Reserved
```

### 2.3 STATUS Register (0x04) - Read Only

```
Bit 0: CARD_PRESENT - SD card detected (directly from card detect pin)
Bit 1: READY        - Controller idle, ready for command
Bit 2: BUSY         - Operation in progress
Bit 3: ERROR        - Error occurred (see bits [7:4])
Bit 4: CRC_ERR      - CRC error on data
Bit 5: TIMEOUT_ERR  - Operation timed out
Bit 6: CMD_ERR      - Command response error
Bit 7: FIFO_ERR     - FIFO overflow/underflow
Bit 8: COMPLETE     - Operation complete (write 1 to clear)
Bit 9: TX_FIFO_EMPTY- TX FIFO empty
Bit 10: TX_FIFO_FULL - TX FIFO full
Bit 11: RX_FIFO_EMPTY- RX FIFO empty
Bit 12: RX_FIFO_FULL - RX FIFO full
Bits [31:13]: Reserved
```

### 2.4 CMD Register (0x08)

```
Bits [5:0]:  CMD_INDEX - SD command number (0-63)
Bits [7:6]:  RESP_TYPE - Expected response type:
             00 = No response (e.g., CMD0)
             01 = R1 (48-bit, normal)
             10 = R2 (136-bit, CID/CSD)
             11 = R1b (R1 with busy)
Bits [31:8]: Reserved
```

### 2.5 Common SD Commands

| CMD | Name | Argument | Response | Description |
|-----|------|----------|----------|-------------|
| 0   | GO_IDLE | 0 | R1 | Reset card to idle state |
| 8   | SEND_IF_COND | 0x1AA | R7 | Voltage check (SDHC) |
| 9   | SEND_CSD | 0 | R2 | Read CSD register |
| 10  | SEND_CID | 0 | R2 | Read CID register |
| 17  | READ_SINGLE | Block addr | R1 | Read single block |
| 18  | READ_MULTIPLE | Block addr | R1 | Read multiple blocks |
| 24  | WRITE_SINGLE | Block addr | R1 | Write single block |
| 25  | WRITE_MULTIPLE | Block addr | R1 | Write multiple blocks |
| 55  | APP_CMD | 0 | R1 | Prefix for ACMD |
| 58  | READ_OCR | 0 | R3 | Read OCR register |
| ACMD41 | SD_SEND_OP_COND | 0x40000000 | R1 | Init (after CMD55) |

### 2.6 Typical Operation Sequence

**Card Initialization:**
```c
// 1. Power-on: Send 80+ clocks with CS high
sd_ctrl(SD_ENABLE | SD_RESET_FIFO);
delay_ms(10);

// 2. CMD0 - Go idle state (with CS low)
sd_cmd(0, 0);  // Expect R1 = 0x01 (idle)

// 3. CMD8 - Check voltage (SDHC detection)
sd_cmd(8, 0x000001AA);  // Expect R7

// 4. ACMD41 - Initialize (loop until ready)
do {
    sd_cmd(55, 0);          // APP_CMD prefix
    sd_cmd(41, 0x40000000); // HCS=1 for SDHC
} while (response & 0x01);  // Wait for idle bit clear

// 5. CMD58 - Read OCR to confirm SDHC
sd_cmd(58, 0);
```

**Block Read:**
```c
// 1. Send CMD17 with block address
sd_arg(block_num);
sd_cmd(17, RESP_R1);
sd_ctrl(SD_START_CMD);
while (sd_status() & SD_BUSY);

// 2. Start data read
sd_ctrl(SD_START_RD);
while (sd_status() & SD_BUSY);

// 3. Read 512 bytes from FIFO
for (int i = 0; i < 128; i++) {
    data[i] = sd_data();  // 32-bit reads
}
```

### 2.7 SPI Clock Speed

```
SPI_CLK = CLK_FREQ / (2 * (CLKDIV + 1))

For 50 MHz system clock:
- Initialization: CLKDIV = 124 → 200 kHz (< 400 kHz required)
- Normal operation: CLKDIV = 0 → 25 MHz (max for SPI mode)
```

### 2.8 VHDL Implementation Notes

```vhdl
-- DE2-115 SD Card signals (directly from FPGA pins)
-- directly from FPGA pins)
-- directly from FPGA pins)
-- directly from FPGA pins)
-- SD_CLK  : SPI clock
-- SD_CMD  : MOSI (directly from FPGA pins)
-- SD_DAT0 : MISO (directly from FPGA pins)
-- SD_DAT3 : Chip select (directly from FPGA pins)
-- directly from FPGA pins)
-- SD_WP_N : Write protect (directly from FPGA pins)
-- directly from FPGA pins)

entity m65832_sdcard is
    generic (
        CLK_FREQ    : integer := 50_000_000;
        FIFO_DEPTH  : integer := 512  -- Bytes (1 block)
    );
    port (
        clk         : in  std_logic;
        rst_n       : in  std_logic;
        
        -- CPU bus interface
        cs          : in  std_logic;
        addr        : in  std_logic_vector(5 downto 0);
        we          : in  std_logic;
        wdata       : in  std_logic_vector(31 downto 0);
        rdata       : out std_logic_vector(31 downto 0);
        
        -- Interrupt output
        irq         : out std_logic;
        
        -- SPI signals to SD card
        sd_clk      : out std_logic;
        sd_mosi     : out std_logic;
        sd_miso     : in  std_logic;
        sd_cs_n     : out std_logic;
        
        -- Card detect (directly from FPGA pins directly from FPGA directly from FPGA
        sd_cd       : in  std_logic   -- directly from FPGA pins (directly from FPGA
    );
end entity;
```

### 2.9 State Machine Overview

```
        ┌─────────┐
        │  IDLE   │◄──────────────────┐
        └────┬────┘                   │
             │ START_CMD              │
             ▼                        │
        ┌─────────┐                   │
        │SEND_CMD │─────────────────►─┤
        └────┬────┘  timeout/error    │
             │ response              │
             ▼                        │
        ┌─────────┐                   │
        │WAIT_RESP│─────────────────►─┤
        └────┬────┘  timeout/error    │
             │ response rx            │
             ▼                        │
      ┌──────┴──────┐                 │
      ▼             ▼                 │
 ┌─────────┐  ┌─────────┐            │
 │READ_DATA│  │WRITE_DAT│            │
 └────┬────┘  └────┬────┘            │
      │            │                  │
      └──────┬─────┘                  │
             ▼                        │
        ┌─────────┐                   │
        │COMPLETE │───────────────────┘
        └─────────┘
```

---

## 3. C Emulator Integration

Both peripherals should be implemented in the C emulator to match the VHDL behavior exactly:

```c
// emulator/peripherals/uart.c
typedef struct {
    uint8_t  data;
    uint32_t status;
    uint32_t ctrl;
    uint32_t baud;
    // Internal state
    uint8_t  rx_fifo[16];
    int      rx_head, rx_tail;
    uint8_t  tx_fifo[16];
    int      tx_head, tx_tail;
} uart_state_t;

uint32_t uart_read(uart_state_t *u, uint32_t addr);
void uart_write(uart_state_t *u, uint32_t addr, uint32_t data);
void uart_tick(uart_state_t *u);  // Called each cycle

// emulator/peripherals/sdcard.c
typedef struct {
    uint32_t ctrl;
    uint32_t status;
    uint32_t cmd;
    uint32_t arg;
    uint32_t resp[4];
    uint32_t blksize;
    uint32_t blkcnt;
    uint32_t timeout;
    uint32_t clkdiv;
    // Internal state
    FILE    *image;      // Backing file for SD image
    uint8_t  fifo[512];
    int      fifo_pos;
    int      state;
} sdcard_state_t;

uint32_t sdcard_read(sdcard_state_t *sd, uint32_t addr);
void sdcard_write(sdcard_state_t *sd, uint32_t addr, uint32_t data);
void sdcard_tick(sdcard_state_t *sd);
```

---

## 4. Linux Driver Notes

The Linux kernel will use these register definitions from `platform.h`:

```c
// arch/m65832/include/asm/platform.h (already defined)
#define M65832_UART_BASE     0x10006000
#define M65832_SD_BASE       0x1000A000

// UART registers
#define UART_DATA            0x00
#define UART_STATUS          0x04
#define UART_CTRL            0x08
#define UART_BAUD            0x0C

// SD card registers  
#define SD_CTRL              0x00
#define SD_STATUS            0x04
// ... etc
```

For early console, the kernel uses polled I/O:
```c
static void early_putchar(char c) {
    while (!(readl(uart_base + UART_STATUS) & UART_STATUS_TXRDY))
        ;
    writel(c, uart_base + UART_DATA);
}
```

---

## 5. Testing Checklist

### UART:
- [ ] TX single byte at 115200 baud
- [ ] RX single byte at 115200 baud
- [ ] TX FIFO fills correctly
- [ ] RX FIFO interrupt fires
- [ ] Baud rate changes correctly
- [ ] Loopback mode works

### SD Card:
- [ ] Card detect works
- [ ] CMD0 (GO_IDLE) returns R1=0x01
- [ ] CMD8 returns correct voltage
- [ ] ACMD41 initialization succeeds
- [ ] CMD17 reads single block correctly
- [ ] CMD24 writes single block correctly
- [ ] Multi-block read/write works
- [ ] Timeout detection works
- [ ] CRC error detection works

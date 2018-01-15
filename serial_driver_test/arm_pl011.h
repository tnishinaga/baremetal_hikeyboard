#pragma once

#include <stdint.h>

// registers
typedef struct {
    volatile uint32_t DR;
    volatile uint32_t RSR;
    volatile uint8_t  RESERVED0[16];
    volatile uint32_t FR;
    volatile uint8_t  RESERVED1[8];
    volatile uint32_t IBRD;
    volatile uint32_t FBRD;
    volatile uint32_t LCR_H;
    volatile uint32_t CR;
    volatile uint32_t IFLS;
    volatile uint32_t IMSC;
    volatile uint32_t RIS;
    volatile uint32_t MIS;
    volatile uint32_t ICR;
    volatile uint32_t DMACR;
} PL011_Type;


// register's bit
#define PL011_FR_RI          (1 << 8)
#define PL011_FR_TXFE        (1 << 7)
#define PL011_FR_RXFF        (1 << 6)
#define PL011_FR_TXFF        (1 << 5)
#define PL011_FR_RXFE        (1 << 4)
#define PL011_FR_BUSY        (1 << 3)
#define PL011_FR_DCD         (1 << 2)
#define PL011_FR_DSR         (1 << 1)
#define PL011_FR_CTS         (1 << 0)

#define PL011_LCRH_SPS       (1 << 7)
#define PL011_LCRH_WLEN_8    (3 << 5)
#define PL011_LCRH_WLEN_7    (2 << 5)
#define PL011_LCRH_WLEN_6    (1 << 5)
#define PL011_LCRH_WLEN_5    (0 << 5)
#define PL011_LCRH_FEN       (1 << 4)
#define PL011_LCRH_STP2      (1 << 3)
#define PL011_LCRH_EPS       (1 << 2)
#define PL011_LCRH_PEN       (1 << 1)
#define PL011_LCRH_BRK       (1 << 0)

#define PL011_CR_CTSEN       (1 << 15)
#define PL011_CR_RTSEN       (1 << 14)
#define PL011_CR_RTS         (1 << 11)
#define PL011_CR_DTR         (1 << 10)
#define PL011_CR_RXE         (1 << 9)
#define PL011_CR_TXE         (1 << 8)
#define PL011_CR_LBE         (1 << 7)
#define PL011_CR_SIRLP       (1 << 2)
#define PL011_CR_SIREN       (1 << 1)
#define PL011_CR_UARTEN      (1 << 0)


// functions
void pl011_init(PL011_Type *serial, uint64_t uart_clock);
int pl011_putc(int c, PL011_Type *serial);
int pl011_getc(PL011_Type *serial);

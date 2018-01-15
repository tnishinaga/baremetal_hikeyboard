#include "arm_pl011.h"

void pl011_init(PL011_Type *serial, uint64_t uart_clock)
{
    // disable uart
    serial->CR = 0;
    // set baudrate
    uint32_t baudrate = 115200;
    uint32_t bauddiv = (1000 * uart_clock) / (16 * baudrate);
    uint32_t ibrd = bauddiv / 1000;
    uint32_t fbrd = ((bauddiv - ibrd * 1000) * 64 + 500) / 1000;
    serial->IBRD = ibrd;
    serial->FBRD = fbrd;

    // wait until tx/rx buffer to empty
    while(  (serial->FR & PL011_FR_TXFE) == 0 ||
            (serial->FR & PL011_FR_RXFE) == 0) {
        volatile uint8_t c = serial->DR;
        (void)c;
    }
    serial->LCR_H = 0;
    serial->LCR_H = PL011_LCRH_WLEN_8 | PL011_LCRH_FEN;
    serial->CR = PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE;
}

int pl011_putc(int c, PL011_Type *serial)
{
    // wait TX FIFO buffer is not-full
    while((serial->FR & PL011_FR_TXFF) != 0);
    serial->DR = c & 0xff;
    return c & 0xff;
}

int pl011_getc(PL011_Type *serial)
{
    // wait until RX FIFO buffer gets data
    while((serial->FR & PL011_FR_RXFE) != 0);
    volatile int c = serial->DR & 0xff;
    return c;
}
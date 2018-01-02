#include <stdint.h>

typedef struct {
    volatile uint32_t GPIODATA[256];
    volatile uint32_t GPIODIR;
    volatile uint32_t GPIOIS;
    volatile uint32_t GPIOIBE;
    volatile uint32_t GPIOIEV;
    volatile uint32_t GPIOIE;
    volatile uint32_t GPIORIS;
    volatile uint32_t GPIOMIS;
    volatile uint32_t GPIOIC;
    volatile uint32_t GPIOAFSEL;
    volatile uint8_t  RESERVED0[220];
    volatile uint32_t GPIOIE2;
    volatile uint32_t GPIOIE3;
    volatile uint8_t  RESERVED1[40];
    volatile uint32_t GPIOMIS2;
    volatile uint32_t GPIOMIS3;
} PL061_Type;

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

// memmap
#define UART (PL011_Type *)(0x09000000)
#define GPIO (PL061_Type *)(0x09030000)



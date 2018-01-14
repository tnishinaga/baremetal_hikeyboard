#pragma once

#include <stdint.h>

// registers
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

// register's bit

// functions
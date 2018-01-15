#pragma once



#ifdef TARGET_QEMU
    #include "arm_pl011.h"
    #include "arm_pl061.h"

    // memmap
    #define UART ((PL011_Type *)0x09000000)
    #define GPIO ((PL061_Type *)0x09030000)

#elif TARGET_HIKEY
    #include "arm_pl011.h"
    #include "arm_pl061.h"

    // memmap
    #define UART0 ((PL011_Type *)0xF8015000)
    #define UART1 ((PL011_Type *)0xF7111000)
    #define UART2 ((PL011_Type *)0xF7112000)
    #define UART3 ((PL011_Type *)0xF7113000)
    #define UART4 ((PL011_Type *)0xF7114000)
    #define GPIO0 ((PL061_Type *)0xF8011000)
    #define GPIO1 ((PL061_Type *)0xF8012000)
    #define GPIO2 ((PL061_Type *)0xF8013000)
    #define GPIO3 ((PL061_Type *)0xF8014000)
    #define GPIO4 ((PL061_Type *)0xF7020000)
    #define GPIO5 ((PL061_Type *)0xF7021000)
    #define GPIO6 ((PL061_Type *)0xF7022000)
    #define GPIO7 ((PL061_Type *)0xF7023000)
    #define GPIO8 ((PL061_Type *)0xF7024000)
    #define GPIO9 ((PL061_Type *)0xF7025000)
    #define GPIO10 ((PL061_Type *)0xF7026000)
    #define GPIO11 ((PL061_Type *)0xF7027000)
    #define GPIO12 ((PL061_Type *)0xF7028000)
    #define GPIO13 ((PL061_Type *)0xF7029000)
    #define GPIO14 ((PL061_Type *)0xF702A000)
    #define GPIO15 ((PL061_Type *)0xF702B000)
    #define GPIO16 ((PL061_Type *)0xF702C000)
    #define GPIO17 ((PL061_Type *)0xF702D000)
    #define GPIO18 ((PL061_Type *)0xF702E000)
    #define GPIO19 ((PL061_Type *)0xF702F000)
    
#endif


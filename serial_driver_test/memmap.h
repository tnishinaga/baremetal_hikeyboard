#pragma once



#ifdef TARGET_QEMU
    #include "arm_pl011.h"
    #include "arm_pl061.h"

    // memmap
    #define UART ((PL011_Type *)0x09000000)
    #define GPIO ((PL061_Type *)0x09030000)
#endif


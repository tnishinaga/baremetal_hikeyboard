#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// memmap
#include "memmap.h"

// Peripheral drivers
#include "arm_pl011.h"


VOID
efi_panic(EFI_STATUS efi_status, INTN line)
{
    Print(L"panic at line:%d\n", line);
    Print(L"EFI_STATUS = %d\n", efi_status);
    while (1);
    // TODO: change while loop to BS->Exit
}

#define MY_EFI_ASSERT(status, line) if(EFI_ERROR(status)) efi_panic(status, line);

#ifdef TARGET_QEMU
    #define UART_CLOCK (24000000 * 3UL) // QEMU set apb-pclk to 24MHz(x3 PLL?)
    #define STDIO UART
#elif TARGET_HIKEY_CIRCUITCO
    #define UART_CLOCK (192 * 100000UL) // 19.2MHz
    #define STDIO UART3
#endif

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
    EFI_STATUS efi_status;

    InitializeLib(image_handle, systab);
    
    // get memory map
    UINTN memmap_size = sizeof(EFI_MEMORY_DESCRIPTOR);
    EFI_MEMORY_DESCRIPTOR *memmap = NULL;
    UINTN mapkey, desc_size;
    UINT32 desc_ver;

    efi_status = uefi_call_wrapper(BS->AllocatePool,
        3,
        EfiBootServicesData,
        memmap_size,
        (VOID **)&memmap
    );

    while (1) {
        efi_status = uefi_call_wrapper(
            BS->GetMemoryMap,
            5,
            &memmap_size,
            memmap,
            &mapkey,
            &desc_size,
            &desc_ver
        );
        if (efi_status == EFI_BUFFER_TOO_SMALL) {
            // get more buffer
            efi_status = uefi_call_wrapper(BS->FreePool,
                1,
                (VOID *)memmap
            );
            efi_status = uefi_call_wrapper(BS->AllocatePool,
                3,
                EfiBootServicesData,
                memmap_size,
                (VOID **)&memmap
            );
        } else {
            // error or success
            MY_EFI_ASSERT(efi_status, __LINE__);
            // success
            break;
        }
    }

    // Exit boot service
    uefi_call_wrapper(
        BS->ExitBootServices,
        2,
        image_handle,
        mapkey
    );

    PL011_Type *stdio = STDIO;

    pl011_init(stdio, UART_CLOCK);

    while(1) {
        // echo test
        volatile int c = pl011_getc(stdio);
        pl011_putc(c, stdio);
    } 

	return EFI_SUCCESS;
}

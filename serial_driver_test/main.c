#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "qemu_arm_virt.h"


VOID
efi_panic(EFI_STATUS efi_status, INTN line)
{
    Print(L"panic at line:%d\n", line);
    Print(L"EFI_STATUS = %d\n", efi_status);
    while (1);
    // TODO: change while loop to BS->Exit
}

#define MY_EFI_ASSERT(status, line) if(EFI_ERROR(status)) efi_panic(status, line);


#define UART_CLOCK (24000000 * 3UL) // QEMU set apb-pclk to 24MHz(x3 PLL?)


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

void pl011_init(PL011_Type *serial)
{
    // disable uart
    serial->CR = 0;
    // set baudrate
    uint32_t baudrate = 115200;
    uint32_t bauddiv = (1000 * UART_CLOCK) / (16 * baudrate);
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

    PL011_Type *stdio = UART;

    pl011_init(stdio);

    while(1) {
        // echo test
        volatile int c = pl011_getc(stdio);
        pl011_putc(c, stdio);
    } 

	return EFI_SUCCESS;
}

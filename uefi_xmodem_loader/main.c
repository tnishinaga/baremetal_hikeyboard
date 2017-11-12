#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_EOF 0x1a

#define XMODEM_BLK_SZ 128


VOID
efi_panic(EFI_STATUS efi_status)
{
    Print(L"panic\n");
    Print(L"EFI_STATUS = %d\n", efi_status);
    while (1);
    // TODO: change while loop to BS->Exit
}


VOID wait_ms(UINTN ms)
{
    EFI_STATUS efi_status;
    EFI_EVENT TimerEvent;
    UINTN Index = 0;
    efi_status = uefi_call_wrapper(BS->CreateEvent, 5, EFI_EVENT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (EFI_ERROR (efi_status)) {
        efi_panic(efi_status);
    }
    // 100ns * 10000 = 1ms
    efi_status = uefi_call_wrapper(BS->SetTimer, 3, TimerEvent, TimerRelative, ms * 10000);
    if (EFI_ERROR (efi_status)) {
        efi_panic(efi_status);
    }
    efi_status = uefi_call_wrapper(BS->WaitForEvent, 3, 1, &TimerEvent, &Index);
    if (EFI_ERROR (efi_status)) {
        efi_panic(efi_status);
    }
    // Timeout
    efi_status = uefi_call_wrapper(BS->CloseEvent, 1, TimerEvent);
    if (EFI_ERROR (efi_status)) {
        efi_panic(efi_status);
    }
}

INTN
efi_serial_getc_timeout(SERIAL_IO_INTERFACE *serialio)
{
    EFI_STATUS efi_status = -1;

    INTN buffer = 0;
    while (1) {
        UINTN buffer_size = 1;
        // read character
        efi_status = uefi_call_wrapper(
            serialio->Read,
            3,
            serialio, &buffer_size, &buffer
        );
        if (efi_status == EFI_SUCCESS && buffer_size > 0) {
            // success
            break;
        }
        if (EFI_ERROR(efi_status) && efi_status != EFI_TIMEOUT) {
            // efi_status == EFI_DEVICE_ERROR
            Print(L"serialio->Read return EFI_DEVICE_ERROR\n");
            efi_panic(efi_status);
        }
        // if efi_status == TIMEOUT then continue
        if (efi_status == EFI_TIMEOUT) {
            return EOF;
        }
    }
    return buffer;
}


INTN
efi_serial_getc(SERIAL_IO_INTERFACE *serialio)
{
    INTN c;
    do {
        c = efi_serial_getc_timeout(serialio);
    } while ( c == EOF );
    return c;
}


UINTN
efi_serial_putc(SERIAL_IO_INTERFACE *serialio, UINTN c)
{
    EFI_STATUS efi_status = -1;
    UINTN buffer_size = 1;
    while (1) {
        // read character
        efi_status = uefi_call_wrapper(
            serialio->Write,
            3,
            serialio, &buffer_size, &c
        );
        if (efi_status == EFI_SUCCESS && buffer_size > 0) {
            // success
            break;  
        }
        if (EFI_ERROR(efi_status) && efi_status != EFI_TIMEOUT) {
            // efi_status == EFI_DEVICE_ERROR
            Print(L"serialio->Read return EFI_DEVICE_ERROR\n");
            efi_panic(efi_status);
        }
        // if efi_status == TIMEOUT then continue
    }

    // wait until EFI_SERIAL_OUTPUT_BUFFER to empty
    UINT32 control = 0;
    while ( !(control & EFI_SERIAL_OUTPUT_BUFFER_EMPTY) ) {
        efi_status = uefi_call_wrapper(serialio->GetControl, 2, serialio, &control);
        switch (efi_status) {
            case EFI_SUCCESS:
                break;
            case EFI_UNSUPPORTED:
                // Print(L"serialio->GetControl is unsuppoerted\n");
                control = EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
                break;
            default:
                // error
                Print(L"serialio->GetControl return EFI_DEVICE_ERROR\n");
                efi_panic(efi_status);
        }
    }
    return 1;
}

VOID
xmodem_receive_start(SERIAL_IO_INTERFACE *serialio)
{
    while (1) {
        INTN header;
        wait_ms(3000);
        Print(L"Send NAK\n");
        efi_serial_putc(serialio, (UINTN)XMODEM_NAK);
        header = efi_serial_getc_timeout(serialio);
        Print(L"header = %d\n", header);
        if (header >= 0 && header == XMODEM_SOH){
            break;
        }
    }
}

VOID
uefi_xmodem_receive(SERIAL_IO_INTERFACE *serialio)
{
    xmodem_receive_start(serialio);

    INTN header = XMODEM_SOH;
    INTN stage = 0;
    while (header != XMODEM_EOF && stage < 3) {
        INTN blocknum = efi_serial_getc(serialio);
        Print(L"blocknum = %02x\n", blocknum);
        INTN blocknum_bitrev = efi_serial_getc(serialio);
        Print(L"~blocknum = %02x\n", blocknum_bitrev);
        UINT8 data[XMODEM_BLK_SZ];
        for (INTN i = 0; i < XMODEM_BLK_SZ; i++) {
            data[i] = efi_serial_getc(serialio) & 0xff;
        }
        Print(L"data hexdump\n");
        for (UINTN i = 0; i < XMODEM_BLK_SZ; i+=8) {
            for (UINTN j = 0; j < 8; j++) {
                Print(L"%02x ", data[i+j]);
            }
            Print(L"\n");
        }
        INTN checksum = efi_serial_getc(serialio);
        Print(L"checksum = %02x\n", checksum);

        efi_serial_putc(serialio, XMODEM_ACK);
        Print(L"Send ACK\n");
        header = efi_serial_getc(serialio);
        Print(L"header =  %02x\n", header);
        stage++;
    }

    efi_serial_putc(serialio, XMODEM_ACK);
    Print(L"Send ACK\n");

    return;
}


EFI_STATUS
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
    EFI_GUID serial_io_protocol = SERIAL_IO_PROTOCOL;
    EFI_STATUS efi_status;
    SERIAL_IO_INTERFACE *serialio;

    InitializeLib(image_handle, systab);

    // search handler
    UINTN handlers_size = 0;
    EFI_HANDLE *handlers = NULL;
    efi_status = uefi_call_wrapper(
        BS->LocateHandle,
        5,
        ByProtocol,
        &serial_io_protocol,
        0,
        &handlers_size,
        handlers
    );
    if (efi_status == EFI_BUFFER_TOO_SMALL) {
        efi_status = uefi_call_wrapper(BS->AllocatePool,
            3,
            EfiBootServicesData,
            handlers_size,
            (VOID **)&handlers
        );
        efi_status = uefi_call_wrapper(
            BS->LocateHandle,
            5,
            ByProtocol,
            &serial_io_protocol,
            0,
            &handlers_size,
            handlers
        );
    }
    if (handlers == NULL || EFI_ERROR(efi_status)) {
        FreePool(handlers);
        return efi_status;
    }
    Print(L"%d serial ports found\n", handlers_size / sizeof(EFI_HANDLE));


    // get SERIAL_IO_PROTOCOL
    // use COM0
	efi_status = uefi_call_wrapper(
		BS->HandleProtocol,
        3,
        handlers[1],
		&serial_io_protocol,
        (VOID **)&serialio
    );

    if (EFI_ERROR(efi_status)) {
        Print(L"efi_status is not EFI_SUCCESS\n");
        return efi_status;
    }
    FreePool(handlers);

    if (serialio == NULL) {
        Print(L"serialio == NULL\n");
        return -1;
    }

    // set timeout
    efi_status = uefi_call_wrapper(
		serialio->SetAttributes,
        7,
        serialio,
        115200, /* baudrate */
		0, /* FIFO depth default */
        0, /* Timeout default */
        NoParity,
        8, /* 8bit data */
        OneStopBit
    );
    if (EFI_ERROR(efi_status)) {
        Print(L"efi_status is not EFI_SUCCESS\n");
    }

    // xmodem_dump(serialio);
    uefi_xmodem_receive(serialio);

	return EFI_SUCCESS;
}

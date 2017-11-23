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

#define XMODEM_BLOCK_SIZE (1+2+128+1)
#define XMODEM_DATA_SIZE (128)

#define MY_DEBUG 1

EFI_FILE_PROTOCOL *file_protocol_root = 0;

#define MY_EFI_ASSERT(status, line) if(EFI_ERROR(status)) efi_panic(status, line);


typedef struct {
    UINT8 soh;
    UINT8 blknum;
    UINT8 blknum_rev;
    UINT8 data[128];
    UINT8 checksum;
} __attribute__((__packed__)) __attribute__((aligned(4))) XMODEM_BLOCK;


VOID
efi_panic(EFI_STATUS efi_status, INTN line)
{
    Print(L"panic at line:%d\n", line);
    Print(L"EFI_STATUS = %d\n", efi_status);
    while (1);
    // TODO: change while loop to BS->Exit
}


EFI_STATUS
search_serial_handlers(EFI_HANDLE **handlers, INTN *handler_items)
{
    EFI_STATUS efi_status;
    EFI_GUID serial_io_protocol = SERIAL_IO_PROTOCOL;

    // search handler
    UINTN handlers_size = 0;
    *handlers = NULL;
    efi_status = uefi_call_wrapper(
        BS->LocateHandle,
        5,
        ByProtocol,
        &serial_io_protocol,
        0,
        &handlers_size,
        *handlers
    );
    if (efi_status == EFI_BUFFER_TOO_SMALL) {
        efi_status = uefi_call_wrapper(BS->AllocatePool,
            3,
            EfiBootServicesData,
            handlers_size,
            (VOID **)handlers
        );
        efi_status = uefi_call_wrapper(
            BS->LocateHandle,
            5,
            ByProtocol,
            &serial_io_protocol,
            0,
            &handlers_size,
            *handlers
        );
    }
    if (*handlers == NULL || EFI_ERROR(efi_status)) {
        FreePool(*handlers);
        *handlers = NULL;
        return efi_status;
    }

    *handler_items = handlers_size / sizeof(EFI_HANDLE);
    return EFI_SUCCESS;
}


EFI_STATUS
open_serial_port(SERIAL_IO_INTERFACE **serialio, EFI_HANDLE *handlers, INTN port)
{
    EFI_STATUS efi_status;
    EFI_GUID serial_io_protocol = SERIAL_IO_PROTOCOL;

    efi_status = uefi_call_wrapper(
		BS->HandleProtocol,
        3,
        handlers[port],
		&serial_io_protocol,
        (VOID **)serialio
    );
    return efi_status;
}


INTN
serial_receive_wait(SERIAL_IO_INTERFACE *serialio, INTN timeout_ms)
{
    EFI_STATUS efi_status;
    EFI_EVENT TimerEvent;

    // Create Timer
    efi_status = uefi_call_wrapper(BS->CreateEvent, 5, EFI_EVENT_TIMER, 0, NULL, NULL, &TimerEvent);
    MY_EFI_ASSERT(efi_status, __LINE__);
    // 100ns * 10000 = 1ms
    efi_status = uefi_call_wrapper(BS->SetTimer, 3, TimerEvent, TimerRelative, timeout_ms * 10000);
    MY_EFI_ASSERT(efi_status, __LINE__);

    INTN result = 0;

    while(1) {
        UINT32 control;
        efi_status = uefi_call_wrapper(serialio->GetControl, 2, serialio, &control);
        if (efi_status == EFI_SUCCESS) {
            if (!(control & EFI_SERIAL_INPUT_BUFFER_EMPTY)) {
                // received
                result = 1;
                break;
            }
        } else if (efi_status == EFI_UNSUPPORTED) {
            Print(L"serialio->GetControl is unsuppoerted\n");
            result = -1;
            break;
        } else {
            // error
            efi_panic(efi_status, __LINE__);
        }

        // timeout check
        efi_status = uefi_call_wrapper(BS->CheckEvent, 1, TimerEvent);
        if (efi_status == EFI_SUCCESS) {
            // timeout
            result = 2;
            break;
        } else if (efi_status == EFI_NOT_READY) {
            continue;
        } else {
            // error
            efi_panic(efi_status, __LINE__);
        }
    }
    
    // Close time event
    efi_status = uefi_call_wrapper(BS->CloseEvent, 1, TimerEvent);
    MY_EFI_ASSERT(efi_status, __LINE__);

    if (result == 1) {
        // received
        return 0;
    } else if (result == 2) {
        // timeout
        return 1;
    } else {
        // unsupported
        return -1;
    }
}


EFI_STATUS
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
    EFI_STATUS efi_status;

    InitializeLib(image_handle, systab);

    EFI_HANDLE *serial_handlers;
    INTN serial_handler_items;

    efi_status = search_serial_handlers(&serial_handlers, &serial_handler_items);
    if (EFI_ERROR(efi_status)) {
        // error!
        Print(L"serial handlers not found!\n");
        return efi_status;
    }
    Print(L"%d serial ports found!\n", serial_handler_items);

    SERIAL_IO_INTERFACE *serialio;
    efi_status = open_serial_port(&serialio, serial_handlers, 0);

    // 3秒まつ
    INTN result = serial_receive_wait(serialio, 3000);
    if (result == 0) {
        Print(L"Serial received!\n");
    } else if (result == 1) {
        Print(L"Timeout!\n");
    } else {
        Print(L"Error!\n");
    }
    
    // free
    FreePool(serial_handlers);

	return EFI_SUCCESS;
}

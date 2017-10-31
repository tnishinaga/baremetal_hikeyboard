#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

VOID
efi_panic(EFI_STATUS efi_status)
{
    Print(L"panic\n");
    Print(L"EFI_STATUS = %d\n", efi_status);
    while (1);
    // TODO: change while loop to BS->Exit
}

UINTN
efi_serial_getc(SERIAL_IO_INTERFACE *serialio)
{
    EFI_STATUS efi_status = -1;
    UINT32 control = EFI_SERIAL_INPUT_BUFFER_EMPTY;

    while (control & EFI_SERIAL_INPUT_BUFFER_EMPTY) {
        efi_status = uefi_call_wrapper(serialio->GetControl, 2, serialio, &control);
        switch (efi_status) {
            case EFI_SUCCESS:
                break;
            case EFI_UNSUPPORTED:
                // Print(L"serialio->GetControl is unsuppoerted\n");
                control = 0;
                break;
            default:
                // error
                Print(L"serialio->GetControl return EFI_DEVICE_ERROR\n");
                efi_panic(efi_status);
        }
    }

    UINT8 buffer[1];
    while (1) {
        UINTN buffer_size = 1;
        // read character
        efi_status = uefi_call_wrapper(
            serialio->Read,
            3,
            serialio, &buffer_size, buffer
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
    return ((UINTN)buffer[0]);
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


    // get SERIAL_IO_PROTOCOL
    // use COM0
	efi_status = uefi_call_wrapper(
		BS->HandleProtocol,
        3,
        handlers[0],
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

    // echo back sample
    while(1) {
        UINT8 c;
        c = efi_serial_getc(serialio);
        efi_serial_putc(serialio, c);
    }

	return EFI_SUCCESS;
}

#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
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

    Print(L"efi_status is EFI_SUCCESS\n");

    if (serialio == NULL) {
        Print(L"serialio == NULL\n");
        return -1;
    }

    // echo back sample
    while(1) {
        UINT8 buffer[10];
        UINTN buffer_size = 1;
        UINT32 control = 0;
        efi_status = uefi_call_wrapper(serialio->GetControl, 2, serialio, &control);
        switch (efi_status) {
            case EFI_SUCCESS:
                while (control & EFI_SERIAL_INPUT_BUFFER_EMPTY) {
                    uefi_call_wrapper(serialio->GetControl, 2, serialio, &control);
                }
                break;
            case EFI_UNSUPPORTED:
                // Print(L"serialio->GetControl is unsuppoerted\n");
                break;
            default:
                // error
                Print(L"serialio->GetControl return EFI_DEVICE_ERROR\n");
                return efi_status;
        }
        // read charater
        efi_status = uefi_call_wrapper(
            serialio->Read,
            3,
            serialio, &buffer_size, buffer
        );
        switch (efi_status) {
            case EFI_TIMEOUT:
                continue;
            case EFI_DEVICE_ERROR:
                Print(L"serialio->Read return EFI_DEVICE_ERROR\n");
                return efi_status;
            default:
                ;
        }
            

        efi_status = uefi_call_wrapper(
            serialio->Write,
            3,
            serialio, &buffer_size, buffer
        );
        if (EFI_ERROR(efi_status)) {
            Print(L"serialio->Write is not EFI_SUCCESS\n");
            return efi_status;
        }
        // wait while done
        efi_status = uefi_call_wrapper(serialio->GetControl, 2, serialio, &control);
        switch (efi_status) {
            case EFI_SUCCESS:
                while (!(control & EFI_SERIAL_OUTPUT_BUFFER_EMPTY)) {
                    efi_status = uefi_call_wrapper(serialio->GetControl, 2, serialio, &control);
                }
                break;
            case EFI_UNSUPPORTED:
                // Print(L"serialio->GetControl is unsuppoerted\n");
                break;
            default:
                // error
                Print(L"serialio->GetControl return EFI_DEVICE_ERROR\n");
                return efi_status;
        }
    }

	return EFI_SUCCESS;
}

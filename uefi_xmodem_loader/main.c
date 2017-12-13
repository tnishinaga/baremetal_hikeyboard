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


UINTN
efi_debug_Print (
    IN CONST CHAR16   *fmt,
    ...
    )
{
    if (!MY_DEBUG) {
        return 0;
    }
    va_list     args;
    UINTN       back;
    CHAR16      strbuf[1024];
    EFI_STATUS  efi_status;

    va_start (args, fmt);
    back = VSPrint ((VOID *)strbuf, 0, fmt, args);

    EFI_FILE_PROTOCOL *LogFile = 0;
    CHAR16 *Path = L"log.txt";
    efi_status = uefi_call_wrapper(
        file_protocol_root->Open,
        5,
        file_protocol_root,
        &LogFile,
        Path,
        EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
        0
    );
    if (EFI_ERROR(efi_status)) {
        Print(L"Error at %d\n", __LINE__);
        return efi_status;
    }

    // write to file
    UINTN strbuf_size = StrLen(strbuf) * sizeof(CHAR16);
    efi_status = uefi_call_wrapper(
            LogFile->Write,
            3,
            LogFile,
            &strbuf_size,
            (VOID *)strbuf
    );
    if (EFI_ERROR(efi_status)) {
        Print(L"Error at %d\n", __LINE__);
        return efi_status;
    }

    efi_status = uefi_call_wrapper(
            LogFile->Flush,
            1,
            LogFile
    );

    efi_status = uefi_call_wrapper(LogFile->Close, 1, LogFile);
    if (EFI_ERROR(efi_status)) {
        Print(L"Error at %d\n", __LINE__);
        return efi_status;
    }
    
    va_end (args);
    return back;
}


VOID wait_ms(UINTN ms)
{
    EFI_STATUS efi_status;
    EFI_EVENT TimerEvent;
    UINTN Index = 0;
    efi_status = uefi_call_wrapper(BS->CreateEvent, 5, EFI_EVENT_TIMER, 0, NULL, NULL, &TimerEvent);
    MY_EFI_ASSERT(efi_status, __LINE__);
    // 100ns * 10000 = 1ms
    efi_status = uefi_call_wrapper(BS->SetTimer, 3, TimerEvent, TimerRelative, ms * 10000);
    MY_EFI_ASSERT(efi_status, __LINE__);
    efi_status = uefi_call_wrapper(BS->WaitForEvent, 3, 1, &TimerEvent, &Index);
    MY_EFI_ASSERT(efi_status, __LINE__);
    // Timeout
    efi_status = uefi_call_wrapper(BS->CloseEvent, 1, TimerEvent);
    MY_EFI_ASSERT(efi_status, __LINE__);
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

INTN
efi_serial_getc_timeout(SERIAL_IO_INTERFACE *serialio, INTN timeout_ms)
{
    EFI_STATUS efi_status = -1;

    INTN buffer = 0;
    UINTN buffer_size = 1;

    INTN recv_result = serial_receive_wait(serialio, timeout_ms);
    if (recv_result == 0) {
        // read character
        efi_status = uefi_call_wrapper(
            serialio->Read,
            3,
            serialio, &buffer_size, &buffer
        );
        // error check
        if (efi_status == EFI_DEVICE_ERROR) {
            // efi_status == EFI_DEVICE_ERROR
            Print(L"serialio->Read return EFI_DEVICE_ERROR\n");
            efi_panic(efi_status, __LINE__);
        }
        if (efi_status == EFI_TIMEOUT || buffer_size <= 0) {
            // timeout or unknown error
            return EOF;
        }
    } else if (recv_result == 1) {
        // timeout 
        return EOF;
    } else {
        // unsupported
        return EOF;
    }

    return buffer;
}
 

INTN
efi_serial_getc(SERIAL_IO_INTERFACE *serialio)
{
    INTN c;
    INTN default_timeout = 100;
    do {
        c = efi_serial_getc_timeout(serialio, default_timeout);
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
            efi_panic(efi_status, __LINE__);
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
                efi_panic(efi_status, __LINE__);
        }
    }
    return 1;
}


VOID
uefi_xmodem_receive(SERIAL_IO_INTERFACE *serialio)
{
    #define XMODEM_SERIAL_RETRY_MAX 20
    #define XMODEM_NAK_LIMIT 3

    INTN firsttime = 1;
    INTN nak_count = 0;

    efi_debug_Print(L"sizeof(XMODEM_BLOCK) = %d\n", sizeof(XMODEM_BLOCK));

    while (1) {
        if (firsttime == 1) {
            efi_serial_putc(serialio, XMODEM_NAK);
            wait_ms(100);
            firsttime = 0;
        }

        INTN c = efi_serial_getc_timeout(serialio, 3000);
        efi_debug_Print(L"c = %02x\n", c);

        if (c == EOF) {
            // Receive timeout
            // Send NAK
            efi_serial_putc(serialio, XMODEM_NAK);
            continue;
        } else if (c == XMODEM_SOH) {
            // receive start
            efi_debug_Print(L"Receive start\n");
            XMODEM_BLOCK blk;
            UINT8 *blk_p = (UINT8 *)(&blk);
            for (INTN i = 0; i < XMODEM_BLOCK_SIZE; i++) {
                blk_p[i] = 0xff;
            }
            blk.soh = c;
            INTN recv_byte;
            for (recv_byte = 1; recv_byte < XMODEM_BLOCK_SIZE; recv_byte++) {
                c = efi_serial_getc_timeout(serialio, 10000);
                if (c == EOF) {
                    // timeout
                    break;
                } else {
                    blk_p[recv_byte] = (UINT8)c;
                }
            }
            if (recv_byte != XMODEM_BLOCK_SIZE) {
                efi_debug_Print(L"Send NAK at %d\n", __LINE__);
                efi_debug_Print(L"recv_byte = %d\n", recv_byte);
                for (INTN i = 0; i < XMODEM_BLOCK_SIZE; i+=8) {
                    for (INTN j = 0; j < 8; j++) {
                        efi_debug_Print(L"%02x ", blk_p[i+j]);
                    }
                    efi_debug_Print(L"\n");
                }
                efi_serial_putc(serialio, XMODEM_NAK);
                nak_count++;
                if (XMODEM_NAK_LIMIT <= nak_count) {
                    return;
                }
                continue;
            }
            // debug dump
            for (INTN i = 0; i < XMODEM_BLOCK_SIZE; i+=8) {
                for (INTN j = 0; j < 8; j++) {
                    efi_debug_Print(L"%02x ", blk_p[i+j]);
                }
                efi_debug_Print(L"\n");
            }
            // finish
            efi_serial_putc(serialio, XMODEM_ACK);
            wait_ms(100);
        } else if (c == XMODEM_EOT) {
            // finish
            efi_serial_putc(serialio, XMODEM_ACK);
            break;
        } else if (c == XMODEM_CAN) {
            // cancel
            return;
        } else {
            // skip
            continue;
        }
    }
}


EFI_STATUS
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
    EFI_STATUS efi_status;

    InitializeLib(image_handle, systab);

    // log file io protocol
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
    EFI_GUID simple_file_system_protocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
    EFI_LOADED_IMAGE *loaded_image;
    EFI_FILE_IO_INTERFACE *volume;

    efi_status = uefi_call_wrapper(
        BS->HandleProtocol,
        3,
        image_handle,
        &loaded_image_protocol,
        (VOID **)&loaded_image
    );
    MY_EFI_ASSERT(efi_status, __LINE__);
    efi_status = uefi_call_wrapper(
        BS->HandleProtocol,
        3,
        loaded_image->DeviceHandle,
        &simple_file_system_protocol,
        (VOID *)&volume
    );
    MY_EFI_ASSERT(efi_status, __LINE__);
    // volume open
    efi_status = uefi_call_wrapper(
        volume->OpenVolume,
        2,
        volume,
        &file_protocol_root
    );
    MY_EFI_ASSERT(efi_status, __LINE__);

    // Serial io
    EFI_HANDLE *serial_handlers;
    INTN serial_handler_items;
    SERIAL_IO_INTERFACE *serialio;

    efi_status = search_serial_handlers(&serial_handlers, &serial_handler_items);
    if (EFI_ERROR(efi_status)) {
        // error!
        Print(L"serial handlers not found!\n");
        return efi_status;
    }
    Print(L"%d serial ports found!\n", serial_handler_items);

    // Open xmodem serial port
    efi_status = open_serial_port(&serialio, serial_handlers, 0);

    // Reset
    efi_status = uefi_call_wrapper(serialio->Reset, 1, serialio);
    MY_EFI_ASSERT(efi_status, __LINE__);

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
    MY_EFI_ASSERT(efi_status, __LINE__);

    // xmodem_dump(serialio);
    uefi_xmodem_receive(serialio);

    efi_status = uefi_call_wrapper(file_protocol_root->Close, 1, file_protocol_root);
    MY_EFI_ASSERT(efi_status, __LINE__);

	return EFI_SUCCESS;
}

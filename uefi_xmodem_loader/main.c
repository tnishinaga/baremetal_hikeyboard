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


typedef struct {
    UINT8 soh;
    UINT8 blknum;
    UINT8 blknum_rev;
    UINT8 data[128];
    UINT8 checksum;
} __attribute__((__packed__)) __attribute__((aligned(4))) XMODEM_BLOCK;


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
uefi_xmodem_receive(SERIAL_IO_INTERFACE *serialio)
{
    #define XMODEM_SERIAL_RETRY_MAX 10
    #define XMODEM_NAK_LIMIT 3

    INTN xmodem_nak_count = 0;
    INTN first_flag = 1;
    INTN next_block_number = 1;

    while (1) {
        INTN retry;
        INTN c;

        if (first_flag == 1){
            wait_ms(2000);
            Print(L"Send NAK at line:%d\n", __LINE__);
            efi_serial_putc(serialio, XMODEM_NAK);
            for (retry = 0; retry < XMODEM_SERIAL_RETRY_MAX; retry++) {
                c = efi_serial_getc_timeout(serialio);
                if (c == XMODEM_SOH || c == XMODEM_EOT) {
                    break;
                }
            } // retry end
        } else {
            c = efi_serial_getc_timeout(serialio);
        }
        
        

        if ( c == XMODEM_SOH ) {
            first_flag = 0;
            // receive 2 + 128 bytes
            UINT8 buf[XMODEM_BLOCK_SIZE * 2] = {0};
            buf[0] = XMODEM_SOH;
            INTN receive_bytes = 1;
            for (retry = 0; retry < XMODEM_SERIAL_RETRY_MAX; retry++) {
                while (receive_bytes < XMODEM_BLOCK_SIZE) {
                    INTN c = efi_serial_getc_timeout(serialio);
                    if ( c < 0 ) {
                        continue;
                    }
                    buf[receive_bytes] = c;
                    receive_bytes++;
                }
            } // retry end
            if ( (receive_bytes < XMODEM_BLOCK_SIZE)) {
                if (xmodem_nak_count < XMODEM_NAK_LIMIT) {
                    Print(L"Send NAK at line:%d\n", __LINE__);
                    efi_serial_putc(serialio, XMODEM_NAK);
                    xmodem_nak_count++;
                    continue; // continue from top
                } else {
                    // Cancel
                    Print(L"xmodem receive failure!!\n");
                    efi_serial_putc(serialio, XMODEM_CAN);
                    break; // exit while(1) loop
                }
            }
            // else 
            // dump received data
            for ( INTN i = 0; i < XMODEM_BLOCK_SIZE; i += 8) {
                for ( INTN j = 0; j < 8; j++ ) {
                    if ( (i+j) > XMODEM_BLOCK_SIZE) {
                        break;
                    }
                    Print(L"%02x ", buf[i+j]);
                }
                Print(L"\n");
            }
            // error check
            XMODEM_BLOCK *blk = (XMODEM_BLOCK *)buf;
            UINT8 blknum_xor = blk->blknum ^ blk->blknum_rev;
            UINTN checksum = 0;
            for (INTN i = 0; i < XMODEM_DATA_SIZE; i++ ) {
                checksum = (checksum + (UINTN)blk->data[i]) % 256;
            }
            if ((blknum_xor != 0xff) ||
                (blk->blknum != next_block_number) ||
                (blk->checksum != checksum)
            ) {
                // error
                if (blknum_xor != 0xff) {
                    Print(L"Error: block number != ~(block number)\n");
                    Print(L"blk->blknum : %02x, blk->blknum_rev : %02x\n", blk->blknum, blk->blknum_rev);
                }
                if (blk->blknum != next_block_number) {
                    Print(L"Error: block number != next_block_number\n");
                    Print(L"blk->blknum : %02x, next_block_number : %02x\n", blk->blknum, next_block_number);
                }
                if (blk->checksum != checksum) {
                    Print(L"Error: checksum missmatch\n");
                    Print(L"blk->checksum : %02x, checksum : %02x\n", blk->checksum, checksum);
                }
                // send NAK
                Print(L"Send NAK at line:%d\n", __LINE__);
                efi_serial_putc(serialio, XMODEM_NAK);
                xmodem_nak_count++;
                continue; // continue from top
            }
        } else if (c == XMODEM_EOT) {
            // end
            efi_serial_putc(serialio, XMODEM_ACK);
            break;
        } else {
            // skip
            continue;
        }
        Print(L"Receive success!\n");
        next_block_number = (next_block_number + 1) % 256;
        efi_serial_putc(serialio, XMODEM_ACK);
    }
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

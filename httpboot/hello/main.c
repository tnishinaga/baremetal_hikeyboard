#include <efi.h>
#include <efilib.h>
#include <stdint.h>


EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	InitializeLib(image_handle, systab);

    Print(L"Hello!\n");

    while(1){}

	return EFI_SUCCESS;
}

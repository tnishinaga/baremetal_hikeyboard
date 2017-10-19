#include <efi.h>
#include <efilib.h>
#include <stdint.h>

typedef struct {
    volatile uint8_t reserved0[4];
    volatile uint32_t GPIODATA_0;
    volatile uint32_t GPIODATA_1;
    volatile uint8_t reserved1[4];
    volatile uint32_t GPIODATA_2;
    volatile uint8_t reserved2[12];
    volatile uint32_t GPIODATA_3;
    volatile uint8_t reserved3[28];
    volatile uint32_t GPIODATA_4;
    volatile uint8_t reserved4[60];
    volatile uint32_t GPIODATA_5;
    volatile uint8_t reserved5[124];
    volatile uint32_t GPIODATA_6;
    volatile uint8_t reserved6[252];
    volatile uint32_t GPIODATA_7;
    volatile uint8_t reserved7[508];
    volatile uint32_t GPIODIR;
    volatile uint32_t GPIOIS;
    volatile uint32_t GPIOIBE;
    volatile uint32_t GPIOIEV;
    volatile uint32_t GPIOIE;
    volatile uint32_t GPIORIS;
    volatile uint32_t GPIOMIS;
    volatile uint32_t GPIOIC;
    volatile uint32_t GPIOAFSEL;
    volatile uint8_t reserved8[220];
    volatile uint32_t GPIOIE2;
    volatile uint32_t GPIOIE3;
    volatile uint8_t reserved9[40];
    volatile uint32_t GPIOMIS2;
    volatile uint32_t GPIOMIS3;
} GPIO_Type;

#define GPIO0	((GPIO_Type *)0xF8011000)
#define GPIO1	((GPIO_Type *)0xF8012000)
#define GPIO2	((GPIO_Type *)0xF8013000)
#define GPIO3	((GPIO_Type *)0xF8014000)
#define GPIO4	((GPIO_Type *)0xF7020000)
#define GPIO5	((GPIO_Type *)0xF7021000)
#define GPIO6	((GPIO_Type *)0xF7022000)
#define GPIO7	((GPIO_Type *)0xF7023000)
#define GPIO8	((GPIO_Type *)0xF7024000)
#define GPIO9	((GPIO_Type *)0xF7025000)
#define GPIO10	((GPIO_Type *)0xF7026000)
#define GPIO11	((GPIO_Type *)0xF7027000)
#define GPIO12	((GPIO_Type *)0xF7028000)
#define GPIO13	((GPIO_Type *)0xF7029000)
#define GPIO14	((GPIO_Type *)0xF702A000)
#define GPIO15	((GPIO_Type *)0xF702B000)
#define GPIO16	((GPIO_Type *)0xF702C000)
#define GPIO17	((GPIO_Type *)0xF702D000)
#define GPIO18	((GPIO_Type *)0xF702E000)
#define GPIO19	((GPIO_Type *)0xF702F000)

// USER LED
// GPIO4_0, 1, 2, 3

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	InitializeLib(image_handle, systab);

	// set GPIO4_[0-3] to output
	GPIO4->GPIODIR = 0x0f;
	while (1) {
		for (volatile int i = 0; i < 10000000; i++);
		GPIO4->GPIODATA_0 = 0x0f;
		GPIO4->GPIODATA_1 = 0x0f;
		GPIO4->GPIODATA_2 = 0x0f;
		GPIO4->GPIODATA_3 = 0x0f;
		for (volatile int i = 0; i < 10000000; i++);
		GPIO4->GPIODATA_0 = 0;
		GPIO4->GPIODATA_1 = 0;
		GPIO4->GPIODATA_2 = 0;
		GPIO4->GPIODATA_3 = 0;
	}

	return EFI_SUCCESS;
}

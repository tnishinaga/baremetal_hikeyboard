CROSS_COMPILE=aarch64-linux-gnu-
PREFIX=/usr/aarch64-linux-gnu/
ARCH=aarch64

CC= $(CROSS_COMPILE)gcc
AS= $(CROSS_COMPILE)gcc
CPP= $(CROSS_COMPILE)gcc
LD= $(CROSS_COMPILE)ld
OBJDUMP= $(CROSS_COMPILE)objdump
OBJCOPY= $(CROSS_COMPILE)objcopy

# libgcc
LIBGCC= $(shell $(CC) -print-libgcc-file-name)
# Linker script
LDSCRIPT= $(PREFIX)/lib/elf_aarch64_efi.lds
# PE header and startup code
STARTOBJ= $(PREFIX)/lib/crt0-efi-aarch64.o
# include header
EFI_INCLUDE= $(PREFIX)/include/efi/
INCLUDES= -I. \
	-I$(EFI_INCLUDE) \
	-I$(EFI_INCLUDE)/aarch64 \
	-I$(EFI_INCLUDE)/protocol

# CFLAGS
CFLAGS= -DCONFIG_$(ARCH) \
	-std=c11 -g3 -O2 -Wall -Wextra -Werror \
	-fshort-wchar -fno-strict-aliasing \
	-fno-merge-all-constants -ffreestanding \
	-fno-stack-protector -fno-stack-check
# LDFLAGS
LDFLAGS= -nostdlib --warn-common --no-undefined \
	--fatal-warnings --build-id=sha1 \
	-shared -Bsymbolic
# set EFI_SUBSYSTEM: Application(0x0a)
LDFLAGS+= --defsym=EFI_SUBSYSTEM=0x0a
LDFLAGS+=-L$(PREFIX)/lib 



all: main.efi main.efi.debug

# rebuild shared object to PE binary
%.efi: %.so
	$(OBJCOPY)	-j .text	\
				-j .sdata	\
				-j .data	\
				-j .dynamic	\
				-j .dynsym	\
				-j .rel		\
				-j .rela	\
				-j .rel.*	\
				-j .rela.*	\
				-j .rel*	\
				-j .rela* 	\
				-j .reloc 	\
				-O binary  $*.so $@

# Extract debug symboles
%.efi.debug: %.so
	$(OBJCOPY)	-j .debug_info		\
				-j .debug_abbrev	\
				-j .debug_aranges	\
				-j .debug_line		\
				-j .debug_str		\
				-j .debug_ranges	\
				-j .note.gnu.build-id \
				-O binary  $*.so $@

# build shared object
%.so: %.o
	$(LD) $(LDFLAGS) $(STARTOBJ) $^ -o $@	\
		-lefi -lgnuefi $(LIBGCC) 			\
		-T $(LDSCRIPT)


%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# clean rule
.PHONY: clean
clean:
	rm -f *.o *.efi *.efi.debug
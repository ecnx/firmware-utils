# Firmware Uitls Makefile
INCLUDES=-I include
INDENT_FLAGS=-br -ce -i4 -bl -bli0 -bls -c4 -cdw -ci4 -cs -nbfda -l100 -lp -prs -nlp -nut -nbfde -npsl -nss
CC=gcc
LD=gcc
CFLAGS=-c -Wall -Wextra -O2 -ffunction-sections -fdata-sections
LDFLAGS=-s -Wl,--gc-sections -Wl,--relax

TRXCRC32_OBJS = \
	release/trxcrc32.o \
	release/crc32.o

BINHDR_OBJS = \
	release/binhdr.o

TLMD5_OBJS = \
	release/tlmd5.o \
	release/md5.o

BCMCRC32_OBJS = \
	release/bcmcrc32.o \
	release/crc32.o

all: trxcrc32 tlmd5 binhdr bcmcrc32

prepare:
	@mkdir -p release

trxcrc32: prepare
	@echo "  CC    src/trxcrc32.c"
	@$(CC) $(CFLAGS) $(INCLUDES) src/trxcrc32.c -o release/trxcrc32.o
	@echo "  CC    src/crc32.c"
	@$(CC) $(CFLAGS) $(INCLUDES) src/crc32.c -o release/crc32.o
	@echo "  LD    release/trxcrc32"
	@$(LD) -o release/trxcrc32 $(TRXCRC32_OBJS) $(LDFLAGS)

tlmd5: prepare
	@echo "  CC    src/tlmd5.c"
	@$(CC) $(CFLAGS) $(INCLUDES) src/tlmd5.c -o release/tlmd5.o
	@echo "  CC    src/md5.c"
	@$(CC) $(CFLAGS) $(INCLUDES) src/md5.c -o release/md5.o
	@echo "  LD    release/tlmd5"
	@$(LD) -o release/tlmd5 $(TLMD5_OBJS) $(LDFLAGS)

binhdr: prepare
	@echo "  CC    src/binhdr.c"
	@$(CC) $(CFLAGS) $(INCLUDES) src/binhdr.c -o release/binhdr.o
	@echo "  LD    release/binhdr"
	@$(LD) -o release/binhdr $(BINHDR_OBJS) $(LDFLAGS)

bcmcrc32:
	@echo "  CC    src/bcmcrc32.c"
	@$(CC) $(CFLAGS) $(INCLUDES) src/bcmcrc32.c -o release/bcmcrc32.o
	@echo "  CC    src/crc32.c"
	@$(CC) $(CFLAGS) $(INCLUDES) src/crc32.c -o release/crc32.o
	@echo "  LD    release/bcmcrc32"
	@$(LD) -o release/bcmcrc32 $(BCMCRC32_OBJS) $(LDFLAGS)

install:
	@cp -v release/trxcrc32 /usr/bin/trxcrc32
	@cp -v release/tlmd5 /usr/bin/tlmd5
	@cp -v release/binhdr /usr/bin/binhdr
	@cp -v release/bcmcrc32 /usr/bin/bcmcrc32

uninstall:
	@rm -fv /usr/bin/trxcrc32
	@rm -fv /usr/bin/tlmd5
	@rm -fv /usr/bin/binhdr
	@rm -fv /usr/bin/bcmcrc32

indent:
	@indent $(INDENT_FLAGS) ./*/*.h
	@indent $(INDENT_FLAGS) ./*/*.c
	@rm -rf ./*/*~

clean:
	@echo "  CLEAN ."
	@rm -rf release

analysis:
	@scan-build make
	@cppcheck --force */*.h
	@cppcheck --force */*.c

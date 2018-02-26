# Makefile for interrupt demo

CROSSCOMPILE = arm-linux-gnueabihf-
AS   = $(CROSSCOMPILE)as
LD   = $(CROSSCOMPILE)ld.bfd
DUMP = $(CROSSCOMPILE)objdump -d
GDB  = $(CROSSCOMPILE)gdb
CC   = $(CROSSCOMPILE)gcc 
CXX  = $(CROSSCOMPILE)g++
OBJCOPY = $(CROSSCOMPILE)objcopy

####################################################################
# write program to stm32f103 using ST-Link with openocd, do folloing
# telnet localhost 4444
# reset halt
# flash write_image erase shell.bin 0x08000000
# reset run
####################################################################
# write program to stm32f103 using UART (A9/A10) wire using USB-Serial converter
# BOOT0 switch set to 1
# stm32ld /dev/ttyUSB0 115200 ./shell.bin
# BOOT0 switch back to 0
# press reset button
# reference for stm32ld is 'https://github.com/jsnyder/stm32ld'
####################################################################
# write program from gdb with openocd
# gdb --eval-command="target remote localhost:3333" shell.elf
# (gdb) monitor halt
# (gdb) load
# restart gdb
####################################################################

#CFLAGS   = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration -fno-builtin-printf -O2
#CXXFLAGS = -mcpu=cortex-m3 -mthumb -fno-builtin-printf -std=c++14 -O2
CFLAGS   = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration -fno-builtin-printf -g -O0
CXXFLAGS = -mcpu=cortex-m3 -mthumb -fno-builtin-printf -std=c++14 -fno-threadsafe-statics -g -O0
LDFLAGS = -nostdlib -nostartfiles

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

OBJS = crt0.o main.o prf.o spi.o uart.o stream.o command_processor.o

all: shell.elf shell.dump shell.bin

main.o: tokenizer.hpp

shell.dump:	shell.elf
	$(DUMP) shell.elf >shell.dump

shell.bin:	shell.elf
	$(OBJCOPY) shell.elf shell.bin -O binary

shell.elf: $(OBJS) stm32.ld
	$(LD) -T stm32.ld -o shell.elf $(OBJS) -Map=shell.map

.PHONY: clean
clean:
	rm -f *.o shell.elf shell.bin shell.dump *~

install:
	stm32ld /dev/ttyUSB0 115200 ./shell.bin

flash: shell.elf
	openocd ${OCDCFG} -c 

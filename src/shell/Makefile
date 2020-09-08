# Makefile for shell on STM32F103

# ARM-Cross compiler download from:
# https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
#
# <<CAUTION for Windows users
#
# If you are using ARM GNU Toolchain installed under "C:\Program Files (x86)", Makefile cannot
# find compiler location. In such a case, you need to add and export PATH environment variable
# as below in the bash running in the terminal.
#
# export PATH=$PATH:"/c/Program Files (x86)/GNU Arm Embedded Toolchain/9 2020-q2-update/bin"
#
# >>CAUTION for Windows users

ifeq (${OS},Windows_NT)
	GCCARM := $(firstword $(wildcard /c/gcc-arm-none-*))
else 
	GCCARM := $(firstword $(wildcard /usr/local/gcc-arm-none-* /opt/gcc-arm-none-*))
endif

ifeq (${GCCARM},)
	CROSSCOMPILE = arm-none-eabi-
else
	CROSSCOMPILE = ${GCCARM}/bin/arm-none-eabi-
endif

AS   = $(CROSSCOMPILE)as
LD   = $(CROSSCOMPILE)ld.bfd
DUMP = $(CROSSCOMPILE)objdump
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
# /usr/arm-none-eabi/bin/arm-none-eabi-gdb --eval-command="target remote localhost:3333" shell.elf
# (gdb) monitor halt
# (gdb) load
# restart gdb
####################################################################

INCLUDE = -I.. -I../utils
#CFLAGS   = -mcpu=cortex-m3 -mfloat-abi=hard -mthumb -Wno-implicit-function-declaration -nostdlib -nodefaultlibs -g -O2

CFLAGS   = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration -nostdlib -nodefaultlibs -g -O2
CXXFLAGS = -mcpu=cortex-m3 -mthumb -std=c++17 -fno-threadsafe-statics -fno-exceptions -fno-unwind-tables -g -O2 ${INCLUDE}

#LDFLAGS = -Tstm32.ld -g -Wl,-Map=shell.map,--cref -nostdlib -nostartfiles -static ${INCLUDE} -Xlinker --gc-sections -fno-exceptions
LDFLAGS = -Tstm32.ld -g -Wl,-Map=shell.map,--cref -nostartfiles -static ${INCLUDE} -Xlinker --gc-sections -fno-exceptions -lm #--verbose 

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

OBJS = crt0.o main.o prf.o spi.o uartx.o stream.o command_processor.o can.o gpio.o gpio_mode.o atexit.o adc.o memset.o i2c.o \
	rcc.o dma.o ad5593.o ad5593_command.o bmp280.o bmp280_command.o i2c_command.o i2c_string.o date_command.o timer.o \
	rcc_status.o gpio_command.o timer_command.o rtc.o system_clock.o can_command.o \
	date_time.o bkp.o 

MOBJS = e_log.o e_log10.o

all: shell.elf shell.dump shell.bin

main.o: tokenizer.hpp gpio_mode.hpp stm32f103.hpp
gpio_mode.o: gpio_mode.hpp stm32f103.hpp
can.o: can.hpp stm32f103.hpp stm32f103.hpp
adc.o: adc.hpp stm32f103.hpp stm32f103.hpp
i2c.o: i2c.hpp stm32f103.hpp dma.hpp dma_channel.hpp stm32f103.hpp
rcc.o: rcc.hpp stm32f103.hpp
rcc_status.o: rcc.hpp stm32f103.hpp debug_print.hpp
rtc.o: rtc.hpp stm32f103.hpp
dma.o: dma.hpp dma_channel.hpp stm32f103.hpp
ad5593.o: ad5593.hpp stm32f103.hpp
bmp280.o: bmp280.hpp stm32f103.hpp
timer.o: timer.hpp stm32f103.hpp
system_clock.o: system_clock.hpp

uartx.s : uartx.cpp
	$(CXX) $(CXXFLAGS) -S $<

e_log.o: ../math/e_log.c
	$(CC) $(CFLAGS) -I../common -o $@ -c $<

e_log10.o: ../math/e_log10.c
	$(CC) $(CFLAGS) -I../common -o $@ -c $<

date_time.o: ../date_time/date_time.cpp ../date_time/date_time.hpp
	$(CXX) $(CXXFLAGS) -o date_time.o -c $<

shell.dump: shell.elf
	$(DUMP) -D -h shell.elf >shell.dump

shell.bin: shell.elf 
	$(OBJCOPY) shell.elf shell.bin -O binary

shell.elf: $(OBJS) ${MOBJS} stm32.ld Makefile
	$(CXX) $(LDFLAGS) -o shell.elf $(OBJS) ${MOBJS}

.PHONY: clean
clean:
	rm -f *.o shell.elf shell.bin shell.dump *~

install:
	stm32ld /dev/ttyUSB0 115200 ./shell.bin

flash: shell.elf
	openocd ${OCDCFG} -c

echo:
	@echo "GCCARM: " ${GCCARM}
	@echo "CROSSCOMPILE: " ${CROSSCOMPILE}
	@echo "OS: " ${OS}

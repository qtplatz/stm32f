# stm32f c++ baremetal
Yet another stm32f103 (Blue Pill) sandbox based on C++

This project is an experimental work for studying stm32f103 functionality and c++ capability on the bare metal.

On this project, I'm using arm-linux-gnueabihf cross tools installed on x86_64 debian-9 linux without linking standard libraries.  Use of C++11 and above is an excellent advantage over C for a bare-metal environment such as;
- std::atomic<> operation provides a unique spin-lock mechanism without using inline assembly language.
- Template-based meta programming may choose the right algorithm and value at compile-time that may reduce run time cost.
- Appropriate use of 'constexpr' declaration makes it clear that is placed on ROM (flash) and determined at compile time.
- The binary footprint is still small enough.
- constructor/destructor combination will make it easy for a scoped lock/unlock mechanism.

std::atomic<> operation provides an excellent spin-lock mechanism without using inline assembly language.
Template-based metaprogramming may choose the right algorithm and value at compile-time that may reduce run time cost.
Appropriate use of 'constexpr' declaration makes it clear that it is placed on ROM (flash) and is determined at compile time.
The binary footprint is still small enough.
The Constructor/destructor combination will make it easy for a scoped lock/unlock mechanism.
The minor problem is that the class constructor for the global scoped class object is not called due to anything to do in the start-up code. As a workaround, all global scope class objects were called new operator against .bss section pre-allocated memory before the first use of the object.


Project status:

- SPI -- Connect SPI1 and SPI2 by wire, and transmit data has been tested
- I2C -- Read temperature and pressure data from BMP280 has been tested
- RTC seems to only work on LSI clock (either HSE and LSE clock did not work so far).
 * Tiny calendar calculation class 'data_time' has been implemented.
 * The system_clock class has been implemented that is used together with std::chrono
- CAN -- Loopback test has been briefly tested without wire.

Reference projects:
https://github.com/trebisky/stm32f103
https://github.com/fcayci/stm32f1-bare-metal
http://www.stm32duino.com/viewtopic.php?t=72


Toolchain:
Download from:
https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads

Setup OpenOCD:
https://github.com/rogerclarkmelbourne/Arduino_STM32/wiki/Programming-an-STM32F103XXX-with-a-generic-%22ST-Link-V2%22-programmer-from-Linux
https://sourceforge.net/projects/openocd/files/openocd/0.10.0/openocd-0.10.0.tar.bz2/download

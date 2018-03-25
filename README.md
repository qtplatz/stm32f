# stm32f c++ baremetal
Yet another stm32f103 (Blue Pill) sandbox based on C++

This project is an experimental work for study not only stm32f103 functionality but also c++ capability on the bare metal.

On this project, I'm using arm-linux-gnueabihf cross tools installed on x86_64 debian-9 linux without linking standard libraris.  Use of C++11 and above is a grate advantage over C for bare metal environment such as;
- std::atomic<> operation provides a excellent spin-lock mechanism without using inline assembly language.
- Template based meta programming may choose right algorithm and value at compile time that may reduce run time cost.
- Appropriate use of 'constexpr' declaration make it clear that is placed on ROM (flash) and it determined at compile time. 
- The binary footprint is still enough small.
- constractor/destractor combination will make it easy for scoped lock/unlock mechanism.

A minor problem is, the class constractor for global scoped class object does not be called due to nothing to do in start-up code.  As workaround, all global scope class objects were called new operator against .bss segmente pre-allocated memory before fist use of the object.

Project status:
- SPI -- Connect SPI1 and SPI2 by wire, and transmit data has been tested
- I2C -- Read temperature and pressure data from BMP280 has been tested
- RTC -- It seems only works on LSI clock (either HSE and LSE clock did not work so far).
  * Tiny caalender calculation class 'data_time' has been implemented.
  * system_clock class has been implemented that is use togather eith std::chrono
- CAN -- Loopback test has been briefly tested without wire.

Reference projects:
https://github.com/trebisky/stm32f103
https://github.com/fcayci/stm32f1-bare-metal
http://www.stm32duino.com/viewtopic.php?t=72
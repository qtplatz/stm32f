# stm32f c++ baremetal
Yet another stm32f103 (Blue Pill) sandbox based on C++

This project is an experimental work for study not only stm32f103 functionality but also c++ capability on bare metal.

On this project, I'm using arm-linux-gnueabihf cross tools installed on x86_64 debian-9 linux, so that
none of standard libraries were linked.
See src/shell/Makefile for details.

Reference projects:
https://github.com/trebisky/stm32f103
https://github.com/fcayci/stm32f1-bare-metal

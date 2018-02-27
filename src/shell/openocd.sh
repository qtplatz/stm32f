#!/bin/bash
#openocd need root permission or libusb_error_access
sudo openocd -f /usr/local/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/local/share/openocd/scripts/target/stm32f1x.cfg

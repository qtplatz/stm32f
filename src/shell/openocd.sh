#!/bin/bash
#openocd need root permission or libusb_error_access

if [ -d /usr/local/share/openocd ]; then
   CFG=/usr/local/share/openocd
elif [ -d /usr/share/openocd ]; then
	CFG=/usr/share/openocd
fi

sudo openocd -f $CFG/scripts/interface/stlink-v2.cfg -f $CFG/scripts/target/stm32f1x.cfg

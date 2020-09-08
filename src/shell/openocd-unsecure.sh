#!/bin/bash

SUDO=""
CFG=""

case "$(uname -s)" in
    Linux*|Darwin*)
		SUDO=sudo
		dirs=("/usr/local/share", "/usr/share" )
		;;
    CYGWIN*|MINGW*)
		dirs=("/mingw64/share" )		
		;;
    *)
esac

for dir in ${dirs[@]}; do
	if [ -d ${dir}/openocd ]; then
		CFG=${dir}/openocd; break;
	fi
done

set -x
${SUDO} openocd -f ./openocd.cfg -f $CFG/scripts/interface/stlink-v2.cfg -f $CFG/scripts/target/stm32f1x.cfg 

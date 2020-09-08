#!/bin/sh

if [[ -z "$OCDHOST" ]]; then
	OCDHOST=localhost
fi

case "$(uname -s)" in
    Linux*|Darwin*)
		GCCARM=/usr/local/gcc-arm-none-eabi-9-2020-q2-update
		CROSSCOMPILE="arm-none-eabi-"
		;;
    CYGWIN*|MINGW*)
		GCCARM=/c/gcc-arm-none-eabi-9-2020-q2-update
		CROSSCOMPILE="arm-none-eabi-"
		;;
    *)
esac

GDB=${GCCARM}/bin/${CROSSCOMPILE}gdb
TARGET=$(basename $(pwd)).elf

if [[ $# -gt 1 ]]; then
	set -x
	${GDB} --eval-command="target remote $OCDHOST:3333" $@
else
	set -x	
	${GDB} --eval-command="target remote $OCDHOST:3333" ${TARGET}
fi

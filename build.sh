#!/bin/bash

check_fail() {
	if [ "$?" != "0" ]; then
		echo $1
		exit
	fi
}

rm -rf images

CORES=`grep processor /proc/cpuinfo | wc -l`
MAKEOPT=$(($CORES + 1))

make clean
make -j$MAKEOPT
check_fail "Compilation failed, can't continue."

./sprdump images
check_fail "Program execution failed, can't open a test image."

feh images/Creatures/12874_s1.bmp  # Snake


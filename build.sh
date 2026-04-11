#!/bin/bash

cmake -DPICO_SDK_PATH=$HOME/pico-sdk -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -B build
make -C build

OUTPUT_FILE="build/LUOS.uf2"
DEVICE_MOUNTS_BY_NAME="RPI-RP2"

DEVICE=$(lsblk -o NAME,LABEL -ln | grep -m1 $DEVICE_MOUNTS_BY_NAME | awk '{print "/dev/"$1}')

while true; do
	if [ -n "$DEVICE" ]; then
#		pkexec mount "$DEVICE" "$MOUNT_POINT"
		udisksctl mount -b "$DEVICE"
		MOUNT_POINT=$(mount | grep "$DEVICE" | awk '{print $3}')

		if mountpoint -q "$MOUNT_POINT"; then
			cp $OUTPUT_FILE "$MOUNT_POINT/"

			sync
			umount "$MOUNT_POINT"
			sleep 1
			break
		fi
	fi
	sleep 1
	DEVICE=$(lsblk -o NAME,LABEL -ln | grep -m1 $DEVICE_MOUNTS_BY_NAME | awk '{print "/dev/"$1}')
done

# https://www.pragmaticlinux.com/2023/01/how-to-list-all-serial-ports-on-linux/
# PORT=$(ls -l /sys/class/tty/*/device/driver | grep -v "/drivers/port" | grep "ch341-uart" | awk '{print $9}' | awk -F'/' '{print "/dev/" $5}')
# while [ "$PORT" ]; do
# 	PORT=$(ls -l /sys/class/tty/*/device/driver | grep -v "/drivers/port" | grep "ch341-uart" | awk '{print $9}' | awk -F'/' '{print "/dev/" $5}')
# done
# konsole --hold -e screen "$PORT"

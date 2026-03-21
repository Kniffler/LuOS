#!/bin/bash

cmake -DPICO_SDK_PATH=$HOME/pico-sdk -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -B build
make -C build

OUTPUT_FILE="build/LUOS.uf2"
DEVICE_MOUNTS_BY_NAME="RPI-RP2"

while true; do
	DEVICE=$(lsblk -o NAME,LABEL -ln | grep -m1 $DEVICE_MOUNTS_BY_NAME | awk '{print "/dev/"$1}')

	if [ -n "$DEVICE" ]; then

#		 pkexec mount "$DEVICE" "$MOUNT_POINT"
		udisksctl mount -b "$DEVICE"
		MOUNT_POINT=$(mount | grep "$DEVICE" | awk '{print $3}')

		if mountpoint -q "$MOUNT_POINT"; then
			cp $OUTPUT_FILE "$MOUNT_POINT/"

			sync
			umount "$MOUNT_POINT"
			exit 0
		fi
	fi
	sleep 1
done

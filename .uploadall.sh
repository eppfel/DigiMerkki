#!/bin/bash

echo "Uploading app too all usb devices at /dev/cu.usbserial-*"

# this function is called when Ctrl-C is sent
function trap_ctrlc ()
{
    exit 2
}
trap "trap_ctrlc" 2

#check if platformio is availible
which pio
if [[ $? -ne 0 ]] ; then
	echo "Platformio is not installed!"
	exit 2 
fi

#for each usb serial device availible run the build scripts
for serialport in /dev/cu.usbserial-*
do
#	echo $serialport
	pio run -t upload --upload-port $serialport
done
exit 0

#!/bin/bash

ports=/dev/cu.usbserial-*

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

echo "Erasing and then uploading files and app to the following devices:"

#print each usb serial device
for serialport in /dev/cu.usbserial-*
do
	echo $serialport
done

#for each usb serial device availible run the build scripts
for serialport in /dev/cu.usbserial-*
do
#	echo $serialport
	pio run -t erase --upload-port $serialport
	pio run -t uploadfs --upload-port $serialport
	pio run -t upload --upload-port $serialport
done
exit 0

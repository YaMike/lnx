#!/bin/sh
# script for flashing new Mega firmware if it exists at specified path below via mega_loader.ko driver.
# by Michael Likholet <m.likholet@ya.ru>

FW_PATH=/home/root/VTSH-A.bin
OLD_FW_PATH=/tmp/old_fw.bin
DRV_NAME=mega_loader.ko
DRV_PATH=/home/root/$DRV_NAME
DRV_IFACE=/dev/mega
SCRIPT="`basename $0`"

Usage() {
	echo -e "Usage: mega_load.sh\n"\
			 "	Checks for new firmware $FW_PATH,\n"\
			 "	If it exists script loads driver $DRV_PATH\n"\
			 "	and checks for difference of Mega firmware.\n"\
			 "	If there is a difference script loads a new firmware.\n"
	exit 0;
}

[ $# -gt 0 ] && Usage

[ ! -f $FW_PATH ] && echo "$SCRIPT: No new firmware were found at $FW_PATH. Script finished normally." && exit 0

# else we have a firmware by this path. So, load driver and check for existing firmware.
[ ! -f $DRV_PATH ] && echo "$SCRIPT: No driver $DRV_PATH were found! Terminating!" && exit 1

# driver exists. So, load it.
insmod $DRV_NAME
RET=$?
[ $RET -ne 0 ] && echo "Cannot load driver $DRV_NAME, insmod returns code $RET" && exit 2

# driver loaded. Download old firmware.
NEW_FW_SIZE="`ls -l $FW_PATH|awk '{print $5;}'`"
echo -n "Downloading old firmware (~10 seconds)..."
cat $DRV_IFACE | head -c $NEW_FW_SIZE > "$OLD_FW_PATH.$NEW_FW_SIZE"
echo "done."

# check for difference
diff $FW_PATH "$OLD_FW_PATH.$NEW_FW_SIZE"
if [ $? -eq 0 ];
then
	echo "New Mega firmware and old Mega firmware are the same. Exit."
	rmmod $DRV_NAME
	echo ""	
	exit 0
fi

# new firmware is different. So, loading it.
echo -n "Loading new firmware to Mega (~10 seconds)..."
cat $FW_PATH > $DRV_IFACE
echo "done."

# check it.
echo "Init firmware check (~10 seconds)..."
cat $DRV_IFACE | head -c $NEW_FW_SIZE > "$OLD_FW_PATH.$NEW_FW_SIZE"
diff $FW_PATH "$OLD_FW_PATH.$NEW_FW_SIZE"
if [ $? -eq 0 ];
then
	echo "Firmware check successful. Exit." 
	rmmod $DRV_NAME
	# rm $FW_PATH
	echo ""
	exit 0
fi

echo "There is a difference between currently uploaded firmware and its downloaded image from Mega! Flashing or check fail."
rmmod $DRV_NAME
echo ""
exit 3

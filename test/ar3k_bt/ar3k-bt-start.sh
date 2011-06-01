#!/bin/sh

sudo pkill -9 hciattach-ar3k.bin
sudo su -c "echo 1 > /sys/class/rfkill/rfkill0/state"
sudo chmod 777 /dev/ttymxc2
sudo /unit_tests/hciattach-ar3k.bin -n ttymxc2 ath3k 3000000 flow sleep &

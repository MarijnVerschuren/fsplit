#! /bin/bash

make
rmmod fsplit
insmod ./bin/fsplit.ko
ls -la /dev/fsplit
./bin/fsplit_daemon.elf

#!/bin/bash


#if [ "$(ifconfig -a | grep p4p2)" = "" ]
#then exit 1
#fi

#if [ "$(ifconfig -a | grep p3p2)" = "" ]
#then exit 2
#fi
#
#git pull
#sudo mknod /dev/fromclick2 c 241 2
#sudo mknod /dev/fromclick3 c 241 3
# where X is the minor device number (i.e. 0, 1, 2)

#
##make clean
#curdir=`pwd`
#cd ~/dpdk-1.3/DPDK
# ./setarg
#cd $curdir

./configure --disable-linuxmodule --enable-experimental --enable-user-multithread --disable-dynamic-linking CPPFLAGS=-I/home/justine/dpdk-1.3/DPDK/x86_64-default-linuxapp-gcc/include LDFLAGS=-L/home/justine/dpdk-1.3/DPDK/x86_64-default-linuxapp-gcc/lib

make
success=$?

if [ $success -ne 0 ]
then
 exit 6
fi

#cat /dev/fromclick2 >control_packets.pcap &
#pcappid=$!

#sudo ./userlevel/click -I [p4p2,p3p2] -j 4 -f two_input.click

#kill $pcappid

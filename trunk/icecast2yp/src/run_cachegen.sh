#!/bin/sh
PID=`pidof yp-cachegen`
if [ ! -z "$PID" ]
then
	echo "Already runnning!"
	exit
fi
/home/oddsock/bin/yp_cachegen

#!/bin/bash

if [ -z "$ENVIRONMENT" ]
then
	echo "Environment undefined."
	exit 1
fi

LOCKFILE=/tmp/${ENVIRONMENT}_check_servers.lock
LOGFILE=/tmp/${ENVIRONMENT}_check_servers.log

if [ -f "$LOCKFILE" ]
then
	echo "An instance is already running."
	exit 2
else
	echo "Creating lockfile..."
	touch "$LOCKFILE"
	echo "Running check_servers.php..."
	/usr/bin/php5 `dirname $0`/check_servers1.php > $LOGFILE 2>&1
date >> /tmp/${ENVIRONMENT}_list
cat $LOGFILE >> /tmp/${ENVIRONMENT}_list
	rm "$LOCKFILE"
	echo "OK."
fi

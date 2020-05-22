#!/bin/bash

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################
#
# Set up virtual can device "elmcan" as sink
# for the elm2canbridge
#


#Default dev, can be overriden by commandline
DEV=elmcan


virtualCanConfigure() {
	echo "Setting up VIRTUAL CAN"
	sudo  modprobe -n --first-time vcan &> /dev/null
	loadmod=$?
	if [ $loadmod -eq 0 ]
	then
		echo "Virtual CAN module not yet loaded. Loading......"
		sudo modprobe vcan
	fi


	ifconfig "$DEV" &> /dev/null
	noif=$?
	if [ $noif -eq 1 ]
	then
		echo "Virtual CAN interface not yet existing. Creating..."
		sudo ip link add dev "$DEV" type vcan
	fi
	sudo ip link set "$DEV" up
}


echo "Preparing to bring up vcan interface $DEV"

#If up?
up=$(ifconfig "$DEV" 2> /dev/null | grep NOARP | grep -c RUNNING)

if [ $up -eq 1 ]
then
   echo "Interface already up. Exiting"
   exit
fi

virtualCanConfigure

echo "Done."



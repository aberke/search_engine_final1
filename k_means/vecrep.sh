#!/bin/bash

# This example assumes that you code in Python
if [ ! -f searchio.so ];
then
	echo
	echo "*** WARNING: PLEASE MAKE FIRST ***"
	echo "Our project includes a C module that must be compiled."
	echo "Run \"make\" before trying to use this script."
	echo
	exit
fi

# Main program to be executed
MAIN="vecrep.py"

# Call $MAIN and pass all the script arguments
python $MAIN $@


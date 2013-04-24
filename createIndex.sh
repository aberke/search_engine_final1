#!/bin/bash

# This example assumes that you code in Python

# Update the PYTHONPATH as needed
#OLD=$PYTHONPATH
#export PYTHONPATH="/course/cs158/src/lib/btrees/py26"

# Main program to be executed
MAIN="createIndex.py"

# Call $MAIN and pass all the script arguments
python $MAIN $@

# Restore previous PYTHONPATH, if you modified the line above
#export PYTHONPATH=$OLD

#!/bin/bash
#new comment
filesdir=$1
searchstr=$2
if [ -n "$filesdir" ] && [ -n "$searchstr" ]
then 
	echo ""
else
	echo "ERROR: Invalid number of inputs"
	exit 1
fi
if [ -d $filesdir ] 
then
	X=$(find "$filesdir" -type f | wc -l)
	Y=$(grep -r $filesdir -e $searchstr | wc -l)
	echo "The number of files are "$X" and the number of matching lines are $Y"
else
	echo "ERROR: Not valid directory"
	exit 1
fi


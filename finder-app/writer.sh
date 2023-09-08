#!/bin/sh
writefile=$1
writestr=$2
cwd=$(pwd)
if [ -n "$writefile" ] && [ -n "$writestr" ]
then 
	cd #
	echo "writing file $writefile"
	mkdir -p "$(dirname $writefile)" &&  touch $writefile
	echo "$writestr" > $writefile
else
	echo "ERROR: Invalid number of inputs"
	exit 1
fi
cd $cwd


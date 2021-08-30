#!/bin/bash

if [ $# -lt 2 ]
then
	echo writefile and writestr were not specified, exitting.
	exit 1
fi

writefile=$1
writestr=$2

dir=$(dirname $writefile)
file=$(basename $writefile)

if [ -d $dir ]
then 
	cd $dir
	echo $writestr > $file

	if [ $? -eq 0 ]
	then 
		echo "Successfully wrote to file"
		exit 0
	else
		echo "File could not be created"
		exit 1
	fi

else
	echo $dir is not a valid directory on the file system, creating...
	mkdir -p $dir
	cd $dir
	echo $writestr > $file

	if [ $? -eq 0 ]
	then 
		echo "Successfully wrote to file"
		exit 0
	else
		echo "File could not be created"
		exit 1
	fi
fi



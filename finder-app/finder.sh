#!/bin/bash

if [ $# -lt 2 ]
then
	echo filesdir and searchstr were not specified, exitting.
	exit 1
fi

filesdir=$1
searchstr=$2

if [ -d $filesdir ]
then 
	cd $filesdir
else
	echo $filesdir is not a valid directory on the file system.
	exit 1
fi

i=0
j=0

matchlist=$(grep -r $searchstr -c | cut -d":" -f2)

i=$(grep -r $searchstr -c | cut -d":" -f2 | wc -l)

for ((num = 1; num <= $i; num++ ))
do
	k=$(echo $matchlist | cut -d" " -f$num)
	j=$(($j + $k))
done

echo The number of files are $i and the number of matching lines are $j


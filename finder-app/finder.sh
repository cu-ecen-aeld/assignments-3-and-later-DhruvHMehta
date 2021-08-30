#!/bin/bash

if [ $# -lt 2 ]
then
	echo filesdir and searchstr were not specified, exitting.
	echo Usage: ./finder.sh /path/to/directory/to/search string_to_search.
	exit 1
fi

filesdir=$1
searchstr=$2

if [ -d $filesdir ]
then 
	cd $filesdir
else
	echo $filesdir is not a valid directory on the file system, exitting.
	echo Please enter a valid directory/path.
	exit 1
fi

i=0
j=0

matchlist=$(grep -r $searchstr -c | cut -d":" -f2)

iter=$(grep -r $searchstr -c | cut -d":" -f2 | wc -l)
i=$(grep -r $searchstr -c -l | wc -l)

for ((num = 1; num <= $iter; num++ ))
do
	k=$(echo $matchlist | cut -d" " -f$num)
	j=$(($j + $k))
done

echo The number of files are $i and the number of matching lines are $j


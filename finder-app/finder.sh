#!/bin/sh

if [ $# -lt 2 ]
then
	echo FILESDIR and SEARCHSTR were not specified, exitting.
	echo Usage: ./finder.sh /path/to/directory/to/search string_to_search.
	exit 1
fi

echo "after lt"

FILESDIR=$1
SEARCHSTR=$2

if [ -d $FILESDIR ]
then 
	cd $FILESDIR
else
	echo $FILESDIR is not a valid directory on the file system, exitting.
	echo Please enter a valid directory/path.
	exit 1
fi

echo "after filesdir"

i=$(grep -rcl "${SEARCHSTR}" "${FILESDIR}" | wc -l)

echo "aftre first i"
j=$(grep "${SEARCHSTR}" $(find "${FILESDIR}" -type f) | wc -l)

echo The number of files are $i and the number of matching lines are $j


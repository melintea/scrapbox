#!/bin/sh

#
# Delete web site byproducts.
# Usage: ./cleanup.sh
# Keep in sync with makesite.sh
#

EXTRA_FILES="doxygen.* html_dot.html ../architecture/doxygen.*"

files1=`ls *.body`
files2=`ls ../architecture/*.body`

# files to work on
files="$files1 $files2"
# resulting files
nfiles=`echo $files | sed s/.body/.html/g`
nfiles="$nfiles $EXTRA_FILES"

rm $nfiles
echo "Site cleanup done: $nfiles"

#! /bin/sh

#
# Usage: ./tools/detab.sh <file>
# Remplace les tabs avec 4 espaces
# 
#

TMPFILE=detabbed

echo "*** Detabbing $1..."
mv $1 $TMPFILE 
cat $TMPFILE | perl -e 'while(<>) {s/\t/    /g; print;}' > $1
rm $TMPFILE


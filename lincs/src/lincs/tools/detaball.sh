#! /bin/sh

#
# Usage: ./tools/detaball.sh <file>
# Remplace les tabs avec 4 espaces pour tous les .c & .h
# 
#

files=`find . -name '*.[ch]'`

for f in $files; do
    #echo "*** $f"
    ./tools/detab.sh $f
done

make | grep war

#! /bin/sh

#
# Usage: ./tools/install.sh <dir>
# 
#


./tools/scrub.sh
./tools/bootstrap
./configure --prefix=$1 --exec-prefix=$1
make 
make install

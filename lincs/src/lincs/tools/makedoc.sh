#! /bin/sh

#
# Usage: ./tools/makedist.sh
# Va produire la documentation avec doxygen et le site 
#


set -x

cd doc/site 
./makesite.sh

cd ../
doxygen ./doxygen.config

#! /bin/sh

#
# Usage: ./tools/makedist.sh
# Va produire une archive $ARCH.gz
#

ARCH=tiger-snapshot.tar

set -x

./tools/scrub.sh
./tools/bootstrap

cd ..
tar cvf $ARCH tiger
gzip $ARCH
ls -l $ARCH*

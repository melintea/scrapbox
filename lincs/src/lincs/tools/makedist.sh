#! /bin/sh

#
# Usage: ./tools/makedist.sh
# Va produire une archive $ARCH.gz
#

ARCH=confi-snapshot.tar

set -x

./tools/scrub.sh
./tools/bootstrap

cd ..
tar cvf $ARCH lincs
gzip $ARCH
ls -l $ARCH*

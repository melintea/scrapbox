#! /bin/sh

#
#  Testing the emulation lib.  
#

# 
# This file is part of LinCS/tiger.  
# 
# LinCS/tiger is distributed under the terms of the GNU Lesser General Public
# License version 2 or any later version.  See the file COPYING.LIB for copying 
# permission or http://www.gnu.org. 
#                                                                            
# THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  
# IMPLIED, without even the implied warranty of MERCHANTABILITY or FITNESS 
# FOR A PARTICULAR PURPOSE.  ANY USE IS AT YOUR OWN RISK. 
#                                                                            
# Permission to modify the code and to distribute modified code is granted, 
# provided the above notices are retained, and a notice that the code was 
# modified is included with the above copyright notice. 
# 
 
#set -x


PREFIX=$CFGSINSTALL
FILENAME='/etc/dummy'

if test -z $CFGSINSTALL; then
    $TSTDIR/print_red "$0 must be run from within ./run_tests"
    exit 1;
fi

echo "**** Starting test $0"
./run_command ldd -r $PREFIX"/"lib/cfgs_emul.so

cd $PREFIX || exit 1


echo ""
echo "**** " bin/cfgs_cat $FILENAME
txt=`bin/cfgs_cat $FILENAME`
bin/cfgs_cat $FILENAME
if test $? -ne 0; then
    $TSTDIR/print_red "bin/cfgs_cat $FILENAME"
    exit 1;
fi
lines=`echo $txt | grep Regenerated | grep $FILENAME | grep cfgs_cat | wc -l`
if test $lines -ne 1; then
    $TSTDIR/print_red "bin/cfgs_cat $FILENAME"
    echo $lines
    exit 1;
fi


echo ""
echo "**** " LD_PRELOAD=$PREFIX"/"lib/cfgs_emul.so cat $FILENAME
echo ""
txt=`LD_PRELOAD=$PREFIX"/"lib/cfgs_emul.so cat $FILENAME`
LD_PRELOAD=$PREFIX"/"lib/cfgs_emul.so cat $FILENAME
if test $? -ne 0; then
    $TSTDIR/print_red "bin/cfgs_cat $FILENAME"
    exit 1;
fi
lines=`echo $txt | grep Regenerated | grep $FILENAME | grep cfgs_cat | wc -l`
if test $lines -ne 1; then
    $TSTDIR/print_red "bin/cfgs_cat $FILENAME"
    echo $lines
    exit 1;
fi


$TSTDIR/print_blue "**** Ending test $0"


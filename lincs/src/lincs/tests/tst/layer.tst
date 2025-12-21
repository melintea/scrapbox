#! /bin/sh

#
# Verify that values with the same name can be stored in different layers. 
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
 
#set -x -v


PREFIX=$CFGSINSTALL

VALNAME="$CFGS_TST_PREFIX/network/eth0/ip"
VALUE1='123.345.567.789'
VALUE2='xxx.xxx.xxx.xxx'
#layer 1 is the default one 
LAYER2='layer2'



if test -z $CFGSINSTALL; then
    $TSTDIR/print_red "$0 must be run from within ./run_tests"
    exit 1;
fi

echo "*** Starting test $0"
cd $PREFIX || exit 1


#
# cleanup
#
$RUN_COMMAND bin/cfgs_tool --remove value "name="$VALNAME 
lines=`bin/cfgs_tool --get value "name="$VALNAME | grep $VALNAME | wc -l`
if test $lines -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --remove value name=$VALNAME "
    exit 1;
fi
$RUN_COMMAND bin/cfgs_tool --remove value "name="$VALNAME "layer=$LAYER2"
lines=`bin/cfgs_tool --get value "name="$VALNAME layer=$LAYER2 | grep $VALNAME | wc -l`
if test $lines -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --remove value name=$VALNAME layer=$LAYER2"
    exit 1;
fi


#
# set val in layer 'default'
#
$RUN_COMMAND bin/cfgs_tool --set value "name="$VALNAME  "value="$VALUE1
if test $? -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --set value name=$VALNAME  value=$VALUE1"
    exit 1;
fi
echo "**** " bin/cfgs_tool --get value "name="$VALNAME 
txt=`bin/cfgs_tool --get value "name="$VALNAME `
echo $txt
lines=`echo $txt | grep $VALNAME | grep $VALUE1 | wc -l`
if test $lines -ne 1; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --set value name=$VALNAME  value=$VALUE1"
    exit 1;
fi

#
# set val in layer LAYER2
#
$RUN_COMMAND bin/cfgs_tool --set value "name="$VALNAME  "value="$VALUE2 "layer=$LAYER2"
if test $? -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --set value name=$VALNAME  value=$VALUE2" "layer=$LAYER2"
    exit 1;
fi
echo "**** " bin/cfgs_tool --get value "name="$VALNAME "layer=$LAYER2"
txt=`bin/cfgs_tool --get value "name="$VALNAME "layer=$LAYER2" `
echo $txt
lines=`echo $txt | grep $VALNAME | grep $VALUE2 | wc -l`
if test $lines -ne 1; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --set value name=$VALNAME  value=$VALUE2 layer=$LAYER2"
    exit 1;
fi


#
# cleanup
#
$RUN_COMMAND bin/cfgs_tool --remove value "name="$VALNAME 
lines=`bin/cfgs_tool --get value "name="$VALNAME | grep $VALNAME | wc -l`
if test $lines -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --remove value name=$VALNAME "
    exit 1;
fi
$RUN_COMMAND bin/cfgs_tool --remove value "name="$VALNAME "layer=$LAYER2"
lines=`bin/cfgs_tool --get value "name="$VALNAME layer=$LAYER2 | grep $VALNAME | wc -l`
if test $lines -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --remove value name=$VALNAME layer=$LAYER2"
    exit 1;
fi


$TSTDIR/print_blue "**** Ending test $0"


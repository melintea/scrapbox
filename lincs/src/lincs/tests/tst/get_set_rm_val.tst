#! /bin/sh

#
# 
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

VALNAME2="$CFGS_TST_PREFIX/network/eth0/mask"
VALUE3='255.255.255.0'

VALPAT1='*'



if test -z $CFGSINSTALL; then
    $TSTDIR/print_red "$0 must be run from within ./run_tests"
    exit 1;
fi

echo "*** Starting test $0"

cd $PREFIX || exit 1
rm -rf values
ls -ld *
$TSTDIR/print_blue cfgs_info
bin/cfgs_info

#echo "**** " bin/cfgs_tool --get value "name="$VALNAME 
rez=`bin/cfgs_tool --get value "name="$VALNAME`
echo $rez
lines=`echo $rez | wc -l`
if test $lines -ne 1; then
    $TSTDIR/print_red "FAILED: lines must be 1"
    exit 1;
fi

#
# set a val
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
# change the val
#
$RUN_COMMAND bin/cfgs_tool --set value "name="$VALNAME  "value="$VALUE2
if test $? -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --set value name=$VALNAME  value=$VALUE1"
    exit 1;
fi
echo "**** " bin/cfgs_tool --get value "name="$VALNAME 
txt=`bin/cfgs_tool --get value "name="$VALNAME `
echo $txt
lines=`echo $txt | grep $VALNAME | grep $VALUE2 | wc -l`
if test $lines -ne 1; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --set value name=$VALNAME  value=$VALUE2"
    exit 1;
fi

#
# add a second val
#
$RUN_COMMAND bin/cfgs_tool --set value "name="$VALNAME2  "value="$VALUE3
if test $? -ne 0; then
   $TSTDIR/print_red "FAILED: bin/cfgs_tool --set value name=$VALNAME2  value=$VALUE3"
   exit 1;
fi
echo "**** " bin/cfgs_tool --get value "name="$VALNAME2 
txt=`bin/cfgs_tool --get value "name="$VALNAME2 `
echo $txt
lines=`echo $txt | grep $VALNAME2 | grep $VALUE3 | wc -l`
if test $lines -ne 1; then
   $TSTDIR/print_red "FAILED: bin/cfgs_tool --set value name=$VALNAME2  value=$VALUE3"
   exit 1;
fi

#
# test pattern. expect 2 vals
#
$RUN_COMMAND bin/cfgs_tool --get value "name="$VALPAT1
txt=`bin/cfgs_tool --get value "name="$VALPAT1 `
echo $txt
lines=`echo $txt | grep $VALNAME | grep $VALUE2 | grep $VALNAME2 | grep $VALUE3 | wc -l`
if test $lines -ne 1; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --get value name=$VALPAT1"
    exit 1;
fi

#
# rm a val
#
$RUN_COMMAND bin/cfgs_tool --remove value "name="$VALNAME 
$RUN_COMMAND bin/cfgs_tool --get value "name="$VALPAT1 
txt=`bin/cfgs_tool --get value "name="$VALPAT1 `
lines=`echo $txt | grep $VALNAME2 | wc -l`
if test $lines -ne 1; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --remove value name=$VALNAME "
    exit 1;
fi
lines=`echo $txt | grep $VALNAME | wc -l`
if test $lines -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --remove value name=$VALNAME "
    exit 1;
fi

#
# rm second val
#
$RUN_COMMAND bin/cfgs_tool --remove value "name="$VALNAME2 
$RUN_COMMAND bin/cfgs_tool --get value "name="$VALPAT1 
txt=`bin/cfgs_tool --get value "name="$VALPAT1 `
lines=`echo $txt | grep $VALNAME2 | wc -l`
if test $lines -ne 0; then
    $TSTDIR/print_red "FAILED: bin/cfgs_tool --remove value name=$VALNAME2 "
    exit 1;
fi

$TSTDIR/print_blue "**** Ending test $0"


#!/bin/sh

#
#  Testing daemon/client robustness. 
#  Files starting with c_ are to be sent by dcli to the daemon. 
#  Files starting with d_ are to be sent by dsrv to a client 
#

# 3704bin.rawt                 binary data, under max limit     
# 5024bin.rawt                 bin data, over max lim  
# 8687bin.rawt                                

if test -z $CFGSINSTALL; then
    $TSTDIR/print_red "$0 must be run from within ./run_tests"
    exit 1;
fi

files=`ls -1 rawt/*.rawt | grep -v '/c_'`

for f in $files
do
    $TSTDIR/print_blue "*** $f"
    ./dsrv $f &
    sleep 1 #await server startup
    
    $CFGSINSTALL/bin/cfgs_tool --get value 'name=/tests/dummy'
    sleep 5 #until fixing rawt.c as to first read rq and then send 'answer' FIXME 
    
    # Should probably compare results against a master.  
    # For now, the test is successfull if ret code is 0 FIXME
    if test $? -ne 0 ; then
        $TSTDIR/print_red "*** $f FAILED"
        exit 1
    fi
done

exit 0


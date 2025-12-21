#!/bin/sh

#
#  Testing daemon/client robustness. 
#  Files starting with c_ are to be sent by dcli to the daemon. 
#  Files starting with d_ are to be sent by dsrv to a client 
#

# 3704bin.rawt                 binary data, under max limit     
# 5024bin.rawt                 bin data, over max lim  
# 8687bin.rawt                                
# c_getval.rawt                  good tags & content-length  
# c_getval_badtag.rawt           good content-length, bad tags in       
# c_getval_less.rawt  rawt*      good tags, bad content-length            
# c_getval_badtag_more.rawt      bad tag, content-length more than actual data             
# c_getval_more.rawt             good tag, content-length more than data      

files=`ls -1 rawt/*.rawt | grep -v '/d_'`

ps -ef | grep configd

for f in $files
do
    $TSTDIR/print_blue "*** $f"
    ./dcli $f
    
    # Should probably compare results against a master.  
    # For now, the test is successfull if the daemon is alive.  
    dpid=`pidof cfgs_configd`
    if test x"$dpid" != x""; then 
        echo "*** Daemon alive: $dpid"
    else
        $TSTDIR/print_red "**** cfgs_configd died during tests.  FAILED"
        exit 1
    fi
done

ps -ef | grep configd
exit 0


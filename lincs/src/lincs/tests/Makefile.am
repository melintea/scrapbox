## Makefile.am -- Process this file with automake to produce Makefile.in

include ../Makefile.globals
ifndef CFLAGS_EXTRA
  CFLAGS_EXTRA = 
@ENDIF@
ifndef LDFLAGS_EXTRA
  LDFLAGS_EXTRA = 
@ENDIF@


CFLAGS    =  $(TOP_CFLAGS) $(CFLAGS_EXTRA)
INCLUDES  =  $(TOP_INCLUDES)


noinst_PROGRAMS       = dcli dsrv notif_test multicmd


EXTRA_DIST = \
    _trap_test1 \
    _trap_test2 \
    daemon_signal \
    daemon_stress \
    manual_check \
    print_blue \
    print_red \
    run_command \
    run_test \
    run_tests \
    test_to_core \
    tests.config \
    rawt.bsd.c \
    rawt/3704bin.rawt \
    rawt/5024bin.rawt \
    rawt/8687bin.rawt \
    rawt/c_getval.rawt \
    rawt/c_getval_badtag.rawt \
    rawt/c_getval_badtag_more.rawt \
    rawt/c_getval_less.rawt \
    rawt/c_getval_more.rawt \
    tst/attrib.tst \
    tst/c_proto.tst \
    tst/d_proto.tst \
    tst/get_set_rm_val.tst \
    tst/layer.tst \
    tst/libemul.tst \
    multi/set.multi \
    multi/set.err.multi 


dcli_SOURCES      = rawt.c 
dcli_LDFLAGS      = 
dcli_CFLAGS       = -DTCLIENT
dcli_LDADD        = 
dcli_DEPENDENCIES = 

dsrv_SOURCES      = rawt.c 
dsrv_LDFLAGS      = 
dsrv_CFLAGS       = -DTSERVER
dsrv_LDADD        = 
dsrv_DEPENDENCIES = 

notif_test_SOURCES      = notif_test.c 
notif_test_LDFLAGS      = $(TOP_LINKDIRS) -lcst -lexpat -lpthread -ldl $(LDFLAGS_EXTRA)
#notif_test_CFLAGS       = $(TOP_CFLAGS) $(CFLAGS_EXTRA)
notif_test_LDADD        = $(top_srcdir)/lincs/lib_cfgs_client/libcsc.la @LIBLTDL@
notif_test_DEPENDENCIES = $(top_srcdir)/lincs/lib_cfgs_client/libcsc.la @LIBLTDL@

multicmd_SOURCES      = multicmd.c 
multicmd_LDFLAGS      = $(TOP_LINKDIRS) -lcst -lexpat -lpthread -ldl $(LDFLAGS_EXTRA)
#multicmd_CFLAGS       = $(TOP_CFLAGS) $(CFLAGS_EXTRA)
multicmd_LDADD        = $(top_srcdir)/lincs/lib_cfgs_client/libcsc.la @LIBLTDL@
multicmd_DEPENDENCIES = $(top_srcdir)/lincs/lib_cfgs_client/libcsc.la @LIBLTDL@



tests: check
test:  check

check: dsrv dcli  
	./run_tests


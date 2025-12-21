## Makefile.am -- Process this file with automake to produce Makefile.in

include ../../../Makefile.globals
ifndef CFLAGS_EXTRA
  CFLAGS_EXTRA = 
@ENDIF@
ifndef LDFLAGS_EXTRA
  LDFLAGS_EXTRA = 
@ENDIF@


CFLAGS    =  $(TOP_CFLAGS) -fPIC $(CFLAGS_EXTRA)
INCLUDES  =  $(TOP_INCLUDES)
LDFLAGS   =  $(TOP_LINKDIRS) -lcst -lexpat -lpthread $(LDFLAGS_EXTRA)


lib_LTLIBRARIES       = cfgs_fs_bk.la


#
# backend sample/test 
#
cfgs_fs_bk_la_SOURCES     = cfgs_fs_bk.c fs.c fs.h
cfgs_fs_bk_la_LDFLAGS     = -module $(LDFLAGS_EXTRA)
cfgs_fs_bk_la_LIBADD      = 


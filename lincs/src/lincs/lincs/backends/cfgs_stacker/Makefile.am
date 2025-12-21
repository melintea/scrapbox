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


lib_LTLIBRARIES       = cfgs_stacker.la


#
# stacker backend 
#
cfgs_stacker_la_SOURCES     = cfgs_stacker.c 
cfgs_stacker_la_LDFLAGS     = -module $(LDFLAGS_EXTRA)
cfgs_stacker_la_LIBADD      = 


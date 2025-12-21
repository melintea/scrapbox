## Makefile.am -- Process this file with automake to produce Makefile.in

include ../../Makefile.globals
ifndef CFLAGS_EXTRA
  CFLAGS_EXTRA = 
@ENDIF@
ifndef LDFLAGS_EXTRA
  LDFLAGS_EXTRA = 
@ENDIF@


CFLAGS    =  $(TOP_CFLAGS) -fPIC $(CFLAGS_EXTRA)
INCLUDES  =  $(TOP_INCLUDES)
LDFLAGS   =  $(LDFLAGS_EXTRA)


lib_LTLIBRARIES       = cfgs_emul.la 


#
# cfgs_emulation/interception module 
#
cfgs_emul_la_SOURCES     = open.c 
cfgs_emul_la_LDFLAGS     = -module $(LDFLAGS_EXTRA) 
cfgs_emul_la_LIBADD      = 


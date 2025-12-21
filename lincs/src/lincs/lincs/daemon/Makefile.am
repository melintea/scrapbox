## Makefile.am -- Process this file with automake to produce Makefile.in

include ../../Makefile.globals
ifndef CFLAGS_EXTRA
  CFLAGS_EXTRA = 
@ENDIF@
ifndef LDFLAGS_EXTRA
  LDFLAGS_EXTRA = 
@ENDIF@


CFLAGS    =  $(TOP_CFLAGS) $(CFLAGS_EXTRA)
INCLUDES  =  $(TOP_INCLUDES)
LDFLAGS   =  $(TOP_LINKDIRS) -lcst -lexpat -lpthread -ldl $(LDFLAGS_EXTRA)


bin_PROGRAMS       = cfgs_configd


#
# daemon 
#
cfgs_configd_SOURCES      = cfgs_configd.c 
cfgs_configd_LDFLAGS      = $(LDFLAGS_EXTRA) #-static
cfgs_configd_LDADD        = $(top_srcdir)/lincs/lib_cfgs_tools/libcst.la @LIBLTDL@
cfgs_configd_DEPENDENCIES = $(top_srcdir)/lincs/lib_cfgs_tools/libcst.la @LIBLTDL@


## Makefile.am -- Process this file with automake to produce Makefile.in

include ../../../Makefile.globals
ifndef CFLAGS_EXTRA
  CFLAGS_EXTRA = 
@ENDIF@
ifndef LDFLAGS_EXTRA
  LDFLAGS_EXTRA = 
@ENDIF@


CFLAGS    =  $(TOP_CFLAGS) $(CFLAGS_EXTRA)
INCLUDES  =  $(TOP_INCLUDES)
LDFLAGS   =  $(TOP_LINKDIRS) -lcst -lexpat -lpthread -ldl $(LDFLAGS_EXTRA)


bin_PROGRAMS       = cfgs_info


#
# get value 
#
cfgs_info_SOURCES      = cfgs_info.c 
cfgs_info_LDFLAGS      = $(LDFLAGS_EXTRA) #-static
cfgs_info_LDADD        = $(top_srcdir)/lincs/lib_cfgs_client/libcsc.la @LIBLTDL@
cfgs_info_DEPENDENCIES = $(top_srcdir)/lincs/lib_cfgs_client/libcsc.la @LIBLTDL@


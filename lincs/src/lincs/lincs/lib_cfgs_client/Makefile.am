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
LDFLAGS   =  $(TOP_LINKDIRS) -lcst -lexpat -lpthread $(LDFLAGS_EXTRA)

include_HEADERS = *.h 

lib_LTLIBRARIES   = libcsc.la


##
##  config client API lib
##
libcsc_la_SOURCES     = cfgs_client_api.c 

#getval_LDADD          = $(top_srcdir)/lincs/lib_cfgs_client/libcsc.la \
#                        @LIBLTDL@ #@LIBADD_DL@
#getval_DEPENDENCIES   = $(top_srcdir)/lincs/backends/cfgs_fs_bk/cfgs_fs_bk.la \
#                        @LIBLTDL@ 



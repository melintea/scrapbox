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
LDFLAGS   =  $(TOP_LINKDIRS) -lexpat -lpthread -ldl $(LDFLAGS_EXTRA) 


include_HEADERS = *.h 


lib_LTLIBRARIES   = libcst.la


##
##  tools lib: list, hash, errors, etc.
##
libcst_la_SOURCES     =  cfgs_error.c  cfgs_mem.c    cfgs_dlist.c \
                         cfgs_hash.c   cfgs_daemon.c cfgs_log.c \
                         cfgs_val.c    cfgs_str.c    cfgs_backend.c \
                         cfgs_sock.c   filters.c     cfgs_protocol.c \
                         cfgs_cache.c  cfgs_mutex.c  http_protocol.c 
#libcst_la_LDFLAGS    =  -dlopen $(top_srcdir)/lincs/backends/cfgs_fs_bk/cfgs_fs_bk.la \
#                        $(LDFLAGS_EXTRA)  



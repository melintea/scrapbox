## Makefile.am -- Process this file with automake to produce Makefile.in

include ../Makefile.globals


CFLAGS    =  $(TOP_CFLAGS) $(CFLAGS_EXTRA)
INCLUDES  =  $(TOP_INCLUDES) -I../tvision/examples/tcombo \
             -I /usr/local/include/cfgs 
LDFLAGS   =  $(TOP_LINKDIRS) $(LDFLAGS_EXTRA) -lstdc++ -lncurses -lm -lgpm \
             -lcsc -lcst -lexpat -lpthread -ldl 


bin_PROGRAMS       = tiger


#
# TUI editor
#
tiger_SOURCES      = lincs.cpp   lincs.hpp   tiger.cpp     tiger.hpp    \
                     tv_tui.cpp  tv_tui.hpp  tcolortx.h    tcolortx.cpp \
                     tv_entrydlg.cpp  tv_entrydlg.hpp \
                     tv_adddlg.cpp    tv_adddlg.hpp \
                     tv_infosdlg.cpp  tv_infosdlg.hpp \
                     tv_logindlg.cpp  tv_logindlg.hpp \
                     tv_policydlg.cpp tv_policydlg.hpp \
                     $(top_srcdir)/tvision/examples/tcombo/nmcmbobx.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/nmcmbovw.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/nmcmbown.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/nmsinput.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/scombobx.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/scombovw.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/scombown.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/ssinputl.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/tcmbovwr.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/tcmbowin.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/tcombobx.cpp \
                     $(top_srcdir)/tvision/examples/tcombo/tsinputl.cpp \
                     #list_view.cpp list_view.h 
tiger_LDFLAGS      = $(LDFLAGS_EXTRA) #-static 
                     #-lrhtv -lstdc++ -lncurses -lm -lgpm
tiger_LDADD        = $(top_srcdir)/tvision/makes/librhtv.a @LIBLTDL@
tiger_DEPENDENCIES = $(top_srcdir)/tvision/makes/librhtv.a @LIBLTDL@


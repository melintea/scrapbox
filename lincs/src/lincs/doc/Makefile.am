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


EXTRA_DIST =  \
    doxygen.config \
    doxygen.css \
    index.html \
    architecture/API.txt \
    architecture/arch.dia \
    architecture/arch.jpg \
    architecture/architecture.body \
    architecture/architecture.dia \
    architecture/architecture2.dia \
    architecture/boot.jpg \
    architecture/boot_use.dia \
    architecture/daemon.jpg \
    architecture/dirs.txt \
    architecture/ed.dia \
    architecture/emul.jpg \
    architecture/etape2.txt \
    architecture/knot.txt \
    architecture/normal.jpg \
    architecture/pam.dia \
    architecture/proto_use.dia \
    architecture/protocol.dia \
    architecture/rpc.xml \
    architecture/usecases.dia \
    architecture/vocabulaire.txt \
    site/about.body \
    site/body_sep1.dot \
    site/body_sep2.dot \
    site/cleanup.sh \
    site/contact.body \
    site/docs.body \
    site/download.body \
    site/faq.body \
    site/footer.dot \
    site/header.dot \
    site/index.body \
    site/install.body \
    site/links.body \
    site/lynxx.jpg \
    site/makesite.sh \
    site/menu.dot 

# default target
all: site_html

docs: site_html doxygen_docs

site_html: 
	cd ./site && ./makesite.sh

doxygen_docs: 
	doxygen ./doxygen.config


clean: 
	cd ./site && ./cleanup.sh
	rm -rf html latex man 
    

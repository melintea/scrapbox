## Makefile.am -- Process this file with automake to produce Makefile.in

include ../Makefile.globals


CFLAGS    =  $(TOP_CFLAGS) $(CFLAGS_EXTRA)
INCLUDES  =  $(TOP_INCLUDES)


EXTRA_DIST =  \
    doxygen.config \
    doxygen.css \
    index.html \
    architecture/tiger.body \
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
    

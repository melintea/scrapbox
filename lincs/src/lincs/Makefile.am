
## Makefile.am -- Process this file with automake to produce Makefile.in


##
## FIXME: this is useless, get rid of. 
## Set $(CFLAGS_EXTRA), $(LDFLAGS_EXTRA) to alter all the binaries of the
##

include_HEADERS = lincs/cfgs/*.h 


SUBDIRS   = @subdirs@ lincs tests doc


EXTRA_DIST =  \
    depcomp \
    Makefile.globals \
    HACKING 


#check:
test:
	cd tests && make test

docs:
	cd doc && make docs
    
docdist: docs
	@cp -pR doc Docs-$(PACKAGE)-$(VERSION) 
	@tar cf Docs-$(PACKAGE)-$(VERSION).tar Docs-$(PACKAGE)-$(VERSION) 
	@gzip Docs-$(PACKAGE)-$(VERSION).tar 
	@rm -rf Docs-$(PACKAGE)-$(VERSION) 
	@echo "Doc archive is: "
	@ls -l Docs-$(PACKAGE)-$(VERSION)*



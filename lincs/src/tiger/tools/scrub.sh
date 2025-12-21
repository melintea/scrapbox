#! /bin/sh

#
#
#

set -x

make clean

rm Makefile Makefile.in aclocal.m4
rm config.* configure libtool ltconfig 
rm stamp-h*
rm config/*
rm -rf autom4te.cache
rm -rf libltdl 

rm depcomp install-sh ltmain.sh missing mkinstalldirs

find . -name Makefile | xargs rm 
find . -name Makefile.in | xargs rm 

rm -rf tvision
cvs update -d tvision

find . -name .deps | xargs rm -rf
find . -name .libs | xargs rm -rf

rm tiger/tiger

rm -rf doc/html doc/latex doc/man



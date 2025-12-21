#!/bin/csh 
#
# ./msnews.sh file 'subj subj'
#

#work/inet/
setenv NEWSHOST  "news.polymtl.ca"
setenv NEWSGROUP "soc.culture.romanian,soc.culture.canada,soc.culture.usa,soc.culture.europe,soc.culture.french,soc.culture.german"
setenv NEWSFROM  "bing@mail.cxo"
setenv MAILHOST  "mailhost.info.polymtl.ca"
setenv MAILFROM  "root@localhost"

setenv SUBJECT   "$2"
echo $SUBJECT

./anews $1 



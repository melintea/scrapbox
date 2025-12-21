#!/usr/local/bin/bash 
#
# ./msnews.sh file 'subj subj'
#

#work/inet/
export NEWSHOST="news.polymtl.ca"
#export NEWSGROUP="soc.culture.romanian,soc.culture.canada,soc.culture.usa,soc.culture.europe,soc.culture.french,soc.culture.german"
export NEWSFROM="bing@mail.cxo"
export MAILHOST="mailhost.info.polymtl.ca"
export MAILFROM="root@localhost"
export SUBJECT="$2"
export $SUBJECT

AGROUPS="soc.culture.romanian soc.culture.usa soc.culture.canada soc.culture.french soc.culture.french soc.culture.austria soc.culture.belgium soc.culture.british soc.culture.bulgaria soc.culture.ecsd soc.culture.europe soc.culture.european soc.culture.german soc.culture.greek soc.culture.intercultural soc.culture.israel soc.culture.italian soc.culture.magyar soc.culture.misc soc.culture.multicultural soc.culture.netherlands soc.culture.nordic soc.culture.occitan soc.culture.polish soc.culture.portuguese soc.culture.spain soc.culture.swiss soc.culture.us" 

#IFS=:
for g in $AGROUPS; do
    export NEWSGROUP="$g"
    echo "$NEWSGROUP: ./anews $1 -$SUBJECT-"
    ./anews $1 
done



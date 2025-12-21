#!/bin/bash 

#
# Sync local tree with remote tree.  Copy over any new or modified file but
# do not delete any file that is not in remote.  
#

#
# $Id: syncltree.sh,v 1.7 2005/08/20 00:45:13 amelinte Exp $
#
# Copyright (c) 2005 Aurelian Melinte. 
#
# This tool is distributed under the terms of the GNU Lesser General Public
# License version 2 or any later version - see http://www.gnu.org. 
#                                                                            
# THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  
# IMPLIED, without even the implied warranty of MERCHANTABILITY or FITNESS 
# FOR A PARTICULAR PURPOSE.  ANY USE IS AT YOUR OWN RISK. 
#                                                                            
# Permission to modify the code and to distribute modified code is granted, 
# provided the above notices are retained, and a notice that the code was 
# modified is included with the above copyright notice. 
#

#change to 1 => do not carry on commands: copy, etc. 
TRACEONLY="${TRACEONLY=0}"


usage() 
{
cat<<_EOT
$0: sync local tree with remote.  
$Id: syncltree.sh,v 1.7 2005/08/20 00:45:13 amelinte Exp $
Usage:
    syncltree 'local' 'remote'
Output: 
    +  added to local (exists in remote but not in local)
    -  does not exists in remote but exists in local (do nothing)
    <  remote is newer, copy over
    >  local is newer, do nothing

_EOT
}


ESC=""
function print_red 
{
    /bin/echo "$ESC[01;31m$@$ESC[00m"
    #/bin/echo "\a"
}


function print_blue 
{
    /bin/echo "$ESC[01;34m$@$ESC[00m"
}


mtimes()
{
    rfile_mt=`perl -e '$mtime=(stat($ARGV[0]))[9]; print "$mtime"' "${rfile}"`
    lfile_mt=`perl -e '$mtime=(stat($ARGV[0]))[9]; print "$mtime"' "${lfile}"`
}


sync_file()
{
    if [ ! -f "${rfile}" ] && [ -f "${lfile}" ]; then
        #remote does not exist and local does
        echo "- ${lfile}"
    elif [ -f "${rfile}" ] && [ ! -f "${lfile}" ]; then 
	    #local does not exist but remote does
        echo "+ ${rfile}"
        if [ ${TRACEONLY} = 0 ]; then
            cp -p "${rfile}" "${lfile}"
            touch -r "${rfile}" "${lfile}"
            echo "  "`ls -l "${lfile}"`
        fi
	#remote newer
    elif [ "${rfile}" -nt "${lfile}" ]; then
        echo "< ${rfile}"
        if [ ${TRACEONLY} = 0 ]; then
            cp -p "${rfile}" "${lfile}"
            touch -r "${rfile}" "${lfile}"
            echo "  "`ls -l "${lfile}"`
        fi
	#local newer
    elif [ "${rfile}" -ot "${lfile}" ]; then
        echo "> ${rfile}"
    else
        if [ "${TRACEONLY}" = 1 ]; then
            echo "= ${rfile}"
		fi
    fi
}


sync_dir()
{
    #FIXME: beef func
    # create local dir if does not exist 
    if [ "${TRACEONLY}" = 0 ]; then
        mkdir -p "${lfile}"
	else
        echo "d ${rfile} <=> ${lfile}"  
    fi
}


sync_object()
{
#    ${rfile}=$1
#    ${lfile}=$2

    if [ -d "${rfile}" ]; then
        sync_dir
    elif [ -f "${rfile}" ]; then
        sync_file 
    elif [ ! -f "${rfile}" ] && [ -f "${lfile}" ]; then
        #remote does not exists but local does
        sync_file 
    else
        #echo "? sync_object error!"
        echo "* "`ls -l "${rfile}"`
    fi
}


#
#
#
if [ $# != 2 ]; then
    usage
    exit 1
fi

OLD_IFS="${IFS}"
IFS=$'\012'
remote="$2"
local="$1"

print_blue "*** Syncing  $local  from  $remote..."
files=`find ${remote}`

# list local files that are not in remote
lfiles=`find ${local}`
for lfile in ${lfiles}; do
    rfile=`echo ${lfile} | sed s=${local}=${remote}=`
    if [ ! -f ${rfile} ] && [ ! -d ${lfile} ]; then
        #echo "- ${lfile}"
        files="${files}"$'\012'"${rfile}"
    fi
done

#echo "${files}"

#process remotes
for rfile in ${files}; do
    lfile=`echo "${rfile}" | sed s=${remote}=${local}=`
    sync_object "${rfile}" "${lfile}"
done


print_blue "*** Done"


#end

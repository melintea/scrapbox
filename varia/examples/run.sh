#!/bin/bash
set -e
set -x

#
# Compile and exec.
# Usage: ./run file.cpp
#

if [ $# -ne 1 ]; then
  echo "Usage: `basename $0` <cpp-file>"
  exit 1
fi

compiler=g++

lptInc=../../../include

target=$(basename -s \.cpp $1)

${compiler} --version

#TODO: -mavx2 
${compiler} $1 -std=c++20 -g -O3 \
  -Wall -Wextra -Werror -pedantic -Wno-deprecated-volatile \
  -pedantic \
  -isystem ${lptInc} \
  -lpthread \
  -o ${target} 

./${target} 

rm ./${target}

#!/bin/bash

CXX=g++
#CXX=clang++

rm a.out
${CXX} -std=c++20 -ggdb *.cpp || exit 1
cgdb ./a.out

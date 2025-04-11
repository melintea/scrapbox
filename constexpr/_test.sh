#!/bin/bash

#CXX=g++
CXX=clang++

rm a.out
${CXX} -std=c++20 overflow.cpp 
./a.out

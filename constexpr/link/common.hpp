// --- common.hpp
#pragma once

/*
$ nm -C a.out | grep _c
36:0000000000002004 r _c
37:0000000000002008 r _c
38:000000000000200c r _c
*/
constexpr int _c = 42;

extern const int* func1();
extern const int* func2();


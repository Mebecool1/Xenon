#!/bin/bash
g++ src/xenc.cpp -o xenc -O3 -march=x86-64-v3 -mavx2 -funroll-loops -ffast-math -fuse-ld=lld
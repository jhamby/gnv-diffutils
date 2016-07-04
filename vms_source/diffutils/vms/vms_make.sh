#!/bin/bash

# In case the timestamps get messedup from unpacking/copying etc.
touch configure.ac
sleep 1
touch aclocal.m4
sleep 1
touch lib/config.hin
sleep 1
touch Makefile.in
touch lib/Makefile.in
touch src/Makefile.in
touch tests/Makefile.in
touch doc/Makefile.in
touch man/Makefile.in
sleep 1
touch configure
touch lib/configure
sleep 1
touch config.status
sleep 1
touch Makefile
touch lib/Makefile
touch src/Makefile
touch tests/Makefile
touch doc/Makefile
touch man/Makefile
sleep 1
touch man/cmp.1 man/diff.1 man/diff3.1 man/sdiff.1

export GNV_OPT_DIR=.
make

cp src/cmp src/cmp.exe
cp src/diff src/diff.exe
cp src/diff3 src/diff3.exe
cp src/sdiff src/sdiff.exe

#make install

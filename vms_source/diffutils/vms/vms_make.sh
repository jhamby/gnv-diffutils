#!/bin/bash

set -x

# In case the timestamps get messedup from unpacking/copying etc.
touch configure.ac
sleep 2
touch aclocal.m4
sleep 2
touch lib/config.hin
sleep 2
touch Makefile.in
touch lib/Makefile.in
touch src/Makefile.in
touch tests/Makefile.in
touch doc/Makefile.in
touch man/Makefile.in
sleep 2
touch configure
touch lib/configure
# Handle bad clock skew for NFS served volumes.
sleep 45
touch config.status
ls --full-time config.status
ls --full-time configure
sleep 2
touch Makefile
touch lib/Makefile
touch src/Makefile
touch tests/Makefile
touch doc/Makefile
touch man/Makefile
sleep 2
touch man/cmp.1 man/diff.1 man/diff3.1 man/sdiff.1
touch doc/diffutils.info

# Replacement VMS routines
cc -c -I lib -o lib/progname.o vms/vms_progname.c
cc -c -o gnv_vms_iconv_wrapper.o vms/gnv_vms_iconv_wrapper.c

export GNV_OPT_DIR=.
make

cp src/cmp src/cmp.exe
cp src/diff src/diff.exe
cp src/diff3 src/diff3.exe
cp src/sdiff src/sdiff.exe

set +x

#make install

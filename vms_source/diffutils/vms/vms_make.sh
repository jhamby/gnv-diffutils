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
touch po/Makefile.in
sleep 2
touch configure
touch lib/configure
ls --full-time configure
# Handle bad clock skew for NFS served volumes.
sleep 30
touch config.status
ls --full-time config.status
sleep 2
touch Makefile
touch lib/Makefile
touch src/Makefile
touch tests/Makefile
touch doc/Makefile
touch man/Makefile
touch po/Makefile
sleep 2
touch man/cmp.1 man/diff.1 man/diff3.1 man/sdiff.1
touch doc/diffutils.info

# Replacement VMS routines
cc -c -I lib -o lib/progname.o vms/vms_progname.c
cc -c -o gnv_vms_iconv_wrapper.o vms/gnv_vms_iconv_wrapper.c
cc -c -o vms_popen_hack.o vms/vms_popen_hack.c
cc -c -o vms_get_foreign_cmd.o vms/vms_get_foreign_cmd.c
cc -c -o vms_execvp_hack.o vms/vms_execvp_hack.c
cc -c -o vms_vm_pipe.o vms/vms_vm_pipe.c
cc -c -o vms_terminal_io.o vms/vms_terminal_io.c
cc -c -o vms_fname_to_unix.o vms/vms_fname_to_unix.c

export GNV_OPT_DIR=.
make

cp src/cmp src/cmp.exe
cp src/diff src/diff.exe
cp src/diff3 src/diff3.exe
cp src/sdiff src/sdiff.exe

set +x

#make install

#!/bin/bash

#sleep 1
#touch configure.
#sleep 1
#touch config.status
#sleep 1
#touch Makefile.

#touch lib/Makefile.in
#touch Makefile.in
#sleep 1

#touch configure
#sleep 1
#touch config.status
#sleep 1

#touch lib/Makefile.in
#touch Makefile
#sleep 1

pushd tests
set -o pipefail
make check 2>&1 | tee make_check.out

popd


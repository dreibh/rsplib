#!/bin/bash

if [ -e /usr/bin/dpkg-buildflags ] ; then
   export CPPFLAGS=`dpkg-buildflags --get CPPFLAGS`
   export CFLAGS=`dpkg-buildflags --get CFLAGS`
   export CXXFLAGS=`dpkg-buildflags --get CXXFLAGS`
   export LDFLAGS=`dpkg-buildflags --get LDFLAGS`
fi

# ------ Obtain number of cores ---------------------------------------------
# Try Linux
cores=`getconf _NPROCESSORS_ONLN 2>/dev/null`
if [ "$cores" == "" ] ; then
   # Try FreeBSD
   cores=`sysctl -a | grep 'hw.ncpu' | cut -d ':' -f2 | tr -d ' '`
fi
if [ "$cores" == "" ] ; then
   cores="1"
fi

# ------ Configure and build ------------------------------------------------
./bootstrap && \
./configure --enable-kernel-sctp --enable-static --disable-shared --enable-qt --enable-csp --disable-hsmgtverify --enable-test-programs $@ && \
( gmake -j$cores || make -j$cores )

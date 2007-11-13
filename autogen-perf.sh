#!/bin/sh

GCC_OPT="-march=pentium4 -msse2"

./bootstrap && env CC="g++" CFLAGS="$GCC_OPT" CXXFLAGS="$GCC_OPT" \
./configure --enable-static --disable-shared \
            --enable-kernel-sctp \
            --enable-qt --enable-csp --disable-hsmgtverify \
            --disable-maintainer-mode $@ && \
make

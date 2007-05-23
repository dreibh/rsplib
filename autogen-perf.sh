#!/bin/sh

GCC_OPT="-march=athlon-xp"

./bootstrap && env CC="g++" CFLAGS="$GCC_OPT" CXXFLAGS="$GCC_OPT" \
./configure --enable-static --disable-shared \
            --enable-kernel-sctp \
            --enable-qt --enable-csp --disable-hsmgtverify \
            --disable-maintainer-mode $@ && \
make

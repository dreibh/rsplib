#!/bin/sh

export CFLAGS="-mtune=pentium3m -ftree-vectorize"
export CXXFLAGS="-mtune=pentium3m -ftree-vectorize"
./bootstrap && ./configure --enable-static --disable-shared --enable-qt --enable-csp --disable-hsmgtverify --disable-maintainer-mode $@ && make

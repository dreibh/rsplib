#!/bin/sh

./bootstrap && ./configure --disable-kernel-sctp --enable-static --disable-shared --enable-qt --enable-csp --enable-hsmgtverify $@ && make

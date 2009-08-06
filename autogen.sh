#!/bin/sh

./bootstrap && \
env CC=g++ ./configure --disable-kernel-sctp --enable-static --disable-shared --enable-qt --enable-csp --disable-hsmgtverify $@ && \
( gmake || make )

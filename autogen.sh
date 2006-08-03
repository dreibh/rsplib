#!/bin/sh

./bootstrap && ./configure --enable-static --disable-shared --enable-qt --enable-csp --enable-hsmgtverify $@ && make

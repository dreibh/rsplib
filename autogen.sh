#!/bin/sh
./bootstrap && ./configure --disable-shared --enable-static --enable-csp --enable-qt $@ && gmake

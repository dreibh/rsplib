#!/bin/sh
./bootstrap && ./configure --disable-shared --enable-static && gmake

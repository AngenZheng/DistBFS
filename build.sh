#!/bin/sh
WORKDIR=.
mpicc -I/usr/local/include -I"$WORKDIR/include" -O0 -g3 -Wall -c -fmessage-length=0 src/*.c
mv *.o build
mpicc build/*.o -o BFS -lmpi -lm

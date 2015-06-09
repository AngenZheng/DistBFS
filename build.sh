#!/bin/sh
WORKDIR=.
mpicc -I/usr/local/include -I"$WORKDIR" -O0 -g3 -Wall -c -fmessage-length=0 *.c 
mv *.o build/
mpicc build/*.o -o BFS -lmpi -lzoltan -lm -lmetis

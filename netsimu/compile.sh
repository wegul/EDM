#!/bin/bash

if [ ! -e bin ];
then
    mkdir bin
fi

if [ ! -e obj ];
then
    mkdir obj
fi

echo -e '\n*** Compiling [make clean; make all] ***\n'
make clean
make all 

#!/bin/bash

DATE=`date +%b_%d_%Y`
make clean
pushd ..
tar cf rainbowcrackalack_$DATE.tar rainbowcrackalack/
bzip2 -9 rainbowcrackalack_$DATE.tar
popd

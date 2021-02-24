#!/bin/bash

CP=/bin/cp
MAKE=/usr/bin/make
MKTEMP=/bin/mktemp
RM=/bin/rm

TEMPDIR=`$MKTEMP -d -t rcrackalack_XXXXXXXXXXX`
$CP -R /usr/include/CL $TEMPDIR
$MAKE CC=x86_64-w64-mingw32-gcc WINDOWS_BUILD=1 CL_INCLUDE=$TEMPDIR -j 16
RET=$?
x86_64-w64-mingw32-strip *.exe
$RM -rf $TEMPDIR

exit $RET

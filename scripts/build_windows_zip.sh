#!/bin/bash

CP=/bin/cp
CUT=/usr/bin/cut
GREP=/bin/grep
MKDIR=/bin/mkdir
MKTEMP=/bin/mktemp
MV=/bin/mv
RM=/bin/rm
ZIP=/usr/bin/zip


if [ ! -f Makefile ]; then
	echo "This script must be run in the top-level source directory."
	exit -1
fi

if [ ! -f $ZIP ]; then
	echo "zip program not found.  Install with: apt install zip"
	exit -1;
fi

# Clean the directory, then build the Windows executables.
make clean
./make_windows.sh

# Ensure that the build succeeded.
if [[ ($? != 0) || (! -f crackalack_gen.exe) ]]; then
	exit 0
fi

# Get the version number out of version.h.
VERSION=`$GREP "#define VERSION " version.h | $CUT -f2 -d"\""`
if [[ $VERSION == '' ]]; then
	echo "Failed to extract version number.  :("
	exit -1;
fi

# Make a temporary directory, with "Rainbow Crackalack vX/" within it.
TEMPDIR=`$MKTEMP -d`
SUBDIR="Rainbow Crackalack $VERSION"
$MKDIR $TEMPDIR/"$SUBDIR"

# Copy in the exe files, along with the entire CL directory.
$CP *.exe $TEMPDIR/"$SUBDIR"
$CP shared.h $TEMPDIR/"$SUBDIR"
$CP -R CL $TEMPDIR/"$SUBDIR"
$RM -f $TEMPDIR/"$SUBDIR"/CL/*~

# Make a zip file of the "Rainbow Crackalack vX" directory.
pushd $TEMPDIR
ZIP_FILENAME=Rainbow_Crackalack_Win64_$VERSION.zip
$ZIP -r $ZIP_FILENAME "$SUBDIR"
popd


if [ ! -f $TEMPDIR/$ZIP_FILENAME ]; then
	echo -e "\n\nFailed to create $ZIP_FILENAME!\n"
	exit -1
fi

$MV $TEMPDIR/$ZIP_FILENAME .
$RM -rf $TEMPDIR

echo -e "\n\nSuccessfully created $ZIP_FILENAME!\n"
exit 0


#!/bin/bash

#
# This script will sort a directory of rainbow tables in parallel.
#

GREP=/bin/grep
MKDIR=/bin/mkdir
MV=/bin/mv
PS=/bin/ps
RMDIR=/bin/rmdir
RTSORT=rainbowcrack.rtsort
SLEEP=/bin/sleep

# Each core loads 1 GB of tables.  Physical machine has 32 GB of RAM, so we subtract
# two and divide by two to ensure everything stays out of swap.
NUM_CORES=15

COMMON_DIR=snap/rainbowcrack/common


if [[ $# != 2 ]]; then
    echo "Usage: $0 raw_rt_dir sorted_rt_dir"
    echo
    echo "Note: the raw & unsorted RT directory must be owned and writeable by the invoking process.  The tables are moved to the specified sorted directory and then replaced with sorted chains.  The specified sorted directory is relative to the snap/rainbowcrack/common/ directory, whereas the unsorted dir is not."
    echo
    exit -1
fi

raw_dir=$1
sorted_dir=$2

pushd $COMMON_DIR > /dev/null
if [[ ! -d $sorted_dir ]]; then
    $MKDIR $sorted_dir

    if [[ -d $sorted_dir ]]; then
	echo "Created output directory: $sorted_dir"
    else
	popd > /dev/null
	echo "Failed to create output directory: $sorted_dir"
	exit -1
    fi
fi

# Create the temporary directories.
temp_dirs=()
for i in `seq 1 $NUM_CORES`; do
    temp_dir=$sorted_dir/$i
    if [[ ! -d $temp_dir ]]; then
	$MKDIR $temp_dir
	echo "Created temp dir: $temp_dir"
    fi
    temp_dirs+=($temp_dir)
done
popd > /dev/null

# Move each RT file into one of the temp dirs.
i=0
for rt in $raw_dir/*.rt; do
    dest_dir=${temp_dirs[$i]}
    let "i++"
    if [[ $i == $NUM_CORES ]]; then
	i=0
    fi

    $MV $rt $COMMON_DIR/$dest_dir
done

# Spawn one process per temp dir.
echo "Spawning processes..."
let "core_minus_one=$NUM_CORES-1"
for i in `seq 0 $core_minus_one`; do
    process_dir=${temp_dirs[$i]}
    echo "Process dir: $process_dir"
    $RTSORT $process_dir &
done

# Sleep 15 seconds, then check if all the sort processes have finished.
echo "Waiting for processes to finish..."
while true; do
  jobs=`$PS ax | $GREP rtsort | $GREP -v grep`
  if [[ $jobs == "" ]]; then
    break
  fi
  $SLEEP 15
done

echo "All processes finished."

# Move the sorted files from their temporary dirs into the final sorted dir.
pushd $COMMON_DIR > /dev/null
for i in `seq 0 $core_minus_one`; do
    process_dir=${temp_dirs[$i]}
    echo "Moving $process_dir/*.rt to $sorted_dir..."
    $MV $process_dir/*.rt $sorted_dir
    $RMDIR $process_dir
done
popd > /dev/null

echo "Done."


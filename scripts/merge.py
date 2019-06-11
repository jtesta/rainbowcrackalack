#!/usr/bin/python3

import os, shutil, subprocess, sys

CRACKALACK_VERIFY = './crackalack_verify'
ONE_GB = 1024 * 1024 * 1024
MERGE_DIR_RELATIVE = 'merge'
COMMON_DIR_ABS = os.path.join(os.getcwd(), 'snap/rainbowcrack/common')


# Returns the lowest-indexed file from the list of input files.  The returned file name
# is removed from the dictionary, hence subsequent calls will return the next lowest-
# numbered file.  Returns None when no file names remain.
def get_next_input_file(input_files):
    lowest_index = 9999999999
    found = False
    ret = None
    for index in input_files:
        if index < lowest_index:
            lowest_index = index
            found = True

    if found:
        ret = input_files[lowest_index]
        del input_files[lowest_index]

    return ret


# Merges tables into a single 1 GB file, then moves it to the output directory.
def merge_tables(merge_dir_abs, input_dir_abs, output_dir_abs, output_index):
    try:
        proc = subprocess.run(['rainbowcrack.rtmerge', MERGE_DIR_RELATIVE])
        if proc.returncode != 0:
            print("rtmerge returned non-zero exit code (%u).  Terminating..." % proc.returncode)
            exit(-1)
    except FileNotFoundError as e:
        print("Error: could not run rainbowcrack.rtmerge!")
        exit(-1)
    except subprocess.SubprocessError as e:
        print("Error: SubprocessError: %s" % e)
        exit(-1)

    # Find the file that rtmerge created.
    merged_filename_abs = None
    for f in os.listdir(COMMON_DIR_ABS):
        if f.endswith('.rt'):
            merged_filename_abs = os.path.join(COMMON_DIR_ABS, f)

    if merged_filename_abs is None:
        print("Error: failed to find merged filename!")
        exit(-1)

    # Run crackalack_verify --sorted on the merged file.
    try:
        proc = subprocess.run([CRACKALACK_VERIFY, '--sorted', merged_filename_abs])
        if proc.returncode != 0:
            print("crackalack_verify returned non-zero exit code (%u).  Terminating..." % proc.returncode)
            exit(-1)
    except FileNotFoundError as e:
        print("Error: could not run crackalack_verify!")
        exit(-1)
    except subprocess.SubprocessError as e:
        print("Error: SubprocessError: %s" % e)
        exit(-1)

    # If the file is greater than 1 GB, then this is a hard error.  Otherwise, it is
    # normal for the last run to yield a single file less than 1 GB.
    merged_size = os.path.getsize(merged_filename_abs)
    if merged_size > ONE_GB:
        print("\nError: %s is greater than 1 GB!: %u" % (merged_filename_abs, merged_size))
        exit(-1)
    elif merged_size < ONE_GB:
        print("\n  !! Warning: %s is less than 1 GB (%u).  Make sure this is only happening once at the end.\n\n" % (merged_filename_abs, merged_size))

    # Find the last underscore so we can rename the output file with the appropriate
    # index number.
    filename = os.path.basename(merged_filename_abs)
    upos = filename.rfind('_')
    if upos == -1:
        print("Error: could not parse filename: %s" % filename)
        exit(-1)

    # Move the merged file from the working directory to the output directory.
    merged_filename_abs_new = os.path.join(output_dir_abs, "%s_%u.rt" % (filename[0:upos], output_index))
    print("Moving merged table from %s to %s..." % (merged_filename_abs, merged_filename_abs_new))
    shutil.move(merged_filename_abs, merged_filename_abs_new)

    # Delete all the working files.
    print("Deleting merge directory...\n")
    shutil.rmtree(merge_dir_abs)
    os.mkdir(merge_dir_abs)


# Puts rainbow tables into the "merge" directory.  Ensures that their sizes add up to
# 1 GB exactly, and returns the left-over chains.
#
# Returns a tuple:
#     boolean: True when there are files to be merged, False when no files are left.
#       index: The index of the input file to resume processing on.
#    filename: The filename that contained the left-over bytes from this block
#        data: The chains that overflowed from this block.
def set_merged_dir(input_files, merge_dir_abs, extra_filename, extra_data):
    ret = False
    leftover_filename = None
    leftover_data = None
    merge_size = 0

    # If any data from the last invokation needs to be carried over, add it to the
    # working directory first.
    if extra_data is not None:
        extra_filename_abs = os.path.join(merge_dir_abs, extra_filename)
        print("Writing chains from previous block to: %s" % extra_filename_abs)
        with open(extra_filename_abs, "wb") as f:
            merge_size = f.write(extra_data)
        print("merge_size starting at: %u" % merge_size)
        ret = True

    # Copy files to the working directory until >= 1 GB is reached.
    while True:
        rt_file = get_next_input_file(input_files)
        if rt_file is None:
            print("Input files exhausted.")
            break

        rt_file_size = os.path.getsize(rt_file)
        merge_size += rt_file_size

        print("Copying %s to %s..." % (rt_file, merge_dir_abs))
        shutil.copy(rt_file, merge_dir_abs)
        ret = True

        # If we surpassed 1 GB, store the excess for the next invokation.
        if merge_size > ONE_GB:
            extra_bytes = merge_size - ONE_GB
            offset = rt_file_size - extra_bytes

            print("Source files are greater than 1GB: %u; extra bytes: %u" % (merge_size, extra_bytes))
            rt_file = os.path.join(merge_dir_abs, os.path.basename(rt_file))
            print("Truncating %s from %u to %u..." % (rt_file, rt_file_size, offset))
            data = None
            with open(rt_file, "r+b") as f:
                f.seek(offset, 0)
                leftover_data = f.read(extra_bytes)
                f.truncate(offset)

            leftover_filename = os.path.basename(rt_file)
            xpos = leftover_filename.find('x')
            upos = leftover_filename.rfind('_')
            if (xpos == -1) or (upos == -1):
                print("Error: could not parse filename: %s" % leftover_filename)
                exit(-1)

            leftover_filename = "%s%u%s" % (leftover_filename[0:xpos+1], len(leftover_data) / 16, leftover_filename[upos:])
            print("Leftover filename: %s; leftover data: %u bytes" % (leftover_filename, len(leftover_data)))
            break
        elif merge_size == ONE_GB:
            print("Source files are exactly 1 GB!")
            break

    return ret, leftover_filename, leftover_data



if len(sys.argv) != 3:
    print("Usage: %s /path/to/input_dir /path/to/output_dir" % sys.argv[0])
    print()
    print("  input_dir:  the directory holding pruned rainbow tables to merge.")
    print("  output_dir: the directory to put the merged files in.")
    print()
    exit(-1)

input_dir = sys.argv[1]
output_dir = sys.argv[2]


# Ensure we can find the common directory.
if not os.path.isdir(COMMON_DIR_ABS):
    print("Error: could not find directory!: %s" % COMMON_DIR_ABS)
    exit(-1)

# Create the working directory, if it doesn't already exist.
merge_dir_abs = os.path.join(COMMON_DIR_ABS, MERGE_DIR_RELATIVE)
if not os.path.isdir(merge_dir_abs):
    os.mkdir(merge_dir_abs)

# Create the output directory if it doesn't exist already.
if not os.path.isdir(output_dir):
    print("Creating output directory: %s" % output_dir)
    os.mkdir(output_dir)

# Ensure we can find the crackalack_verify program.
if not os.path.isfile(CRACKALACK_VERIFY):
    print('Error: could not find the %s program.  This needs to be in the current directory in order to continue.' % CRACKALACK_VERIFY)
    exit(-1)

# Ensure that the common directory does not have any *.rt nor *.rtc files before
# starting.  This way, we don't end up confused if anything goes wrong during
# processing.
for f in os.listdir(COMMON_DIR_ABS):
    if f.endswith('.rt') or f.endswith('.rtc'):
        print("Error: %s must not have any *.rt or *.rtc files!  Terminating before doing any processing." % COMMON_DIR_ABS)
        exit(-1)

# Keys are table indices, values are absolute paths to tables.  Example:
#   1000: '/path/to/dir/ntlm_ascii-32-95#8-8_0_422000x6830556_1000.rt',
#   1001: '/path/to/dir/ntlm_ascii-32-95#8-8_0_422000x6830556_1001.rt',
input_files = {}
lowest_index = 9999999999
for f in os.listdir(input_dir):
    upos = f.rfind('_')
    dotpos = f.rfind('.')
    if (upos == -1) or (dotpos == -1):
        print("  !! Warning: could not parse index from filename: %s.  Skipping..." % f)
        continue

    index = int(f[upos+1:dotpos])
    if index < lowest_index:
        lowest_index = index

    input_file = os.path.join(input_dir, f)
    input_file_size = os.path.getsize(input_file)
    if input_file_size > ONE_GB:
        print("Error: input file is larger than 1 GB: %s" % input_file)
        exit(-1)
    elif input_file_size == ONE_GB:
        print("Note: skipping file because it is exactly 1 GB already: %s" % input_file)
        continue

    input_files[index] = input_file


output_index = lowest_index
extra_data = None
extra_filename = None
while True:
    # Copy tables from our source directory to the working directory.  Truncate the
    # last file, if necessary, to ensure that no more than a 1 GB file is created.
    ret, extra_filename, extra_data = set_merged_dir(input_files, merge_dir_abs, extra_filename, extra_data)

    # If no more files are left to process, we're done.
    if not ret:
        break

    # Merge the tables into a single 1 GB file.  The last invokation will likely create
    # a smaller file.
    merge_tables(merge_dir_abs, input_dir, output_dir, output_index)
    output_index += 1

# The merge directory should be empty, so remove it.
os.rmdir(merge_dir_abs)
print("\nFinished merging successfully.\n")

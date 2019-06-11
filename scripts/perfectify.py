#!/usr/bin/python3

#
# This is a wrapper script for the perfectify executable.  Responsible for
# moving unpruned files around, running perfectify on them, then verifying them
# for correctness.
#

import os, shutil, subprocess, sys, time


# The number of input rainbow tables that should be processed at a time.
num_input_files = 0


def p(msg=''):
    print(msg, flush=True)


# Returns a list of highest-numbered unpruned files, or None if no more exist.
def get_unpruned_files(unpruned_dir):
    highest_index = -1
    ret = None

    rt_dict = {}
    for filename in os.listdir(unpruned_dir):
        upos = filename.rfind('_')
        if filename.endswith('.rt') and upos != -1:
            # Parse the index from the filename.  Example:
            # 'ntlm_ascii-32-95#8-8_0_422000x66966623_799.rt' => 799
            index = int(filename[upos+1:-3])

            rt_dict[index] = os.path.join(unpruned_dir, filename)

            # If this is the highest index we've seen so far, update it, and
            # remember its filename.
            if index > highest_index:
                highest_index = index

    if highest_index == -1:
        return None

    ret = [rt_dict[highest_index]]
    while (len(ret) < num_input_files) and (highest_index >= 0):
        highest_index -= 1
        if highest_index in rt_dict:
            ret.insert(0, rt_dict[highest_index])

    return ret


if len(sys.argv) != 3:
    p("Usage: %s unpruned_dir pruned_dir" % sys.argv[0])
    p()
    p("  unpruned_dir: the directory with unpruned RT files.")
    p("    pruned_dir: the directory to put pruned tables.")
    p()
    p("This script will automate the perfectify executable to prune one file at a time by comparing it to all the pruned tables.  Then the newly-pruned table will be moved to the pruned directory.  This is repeated until no more files exist in the unpruned directory.")
    p()
    exit(-1)

unpruned_dir = sys.argv[1]
pruned_dir = sys.argv[2]

# Ensure our executables exist.
if not os.path.exists("../perfectify"):
    p("Error: could not find 'perfectify' executable.")
    exit(-1)

if not os.path.exists("../crackalack_verify"):
    p("Error: could not find 'crackalack_verify' executable.")
    exit(-1)


# Base the number of input files to process at a time on the total memory size of the
# system.  Reserve 2 GB for the system, and 1 GB for the table to compare against.
total_memory_gb = int((os.sysconf('SC_PAGE_SIZE') * os.sysconf('SC_PHYS_PAGES')) / (1024**3))
num_input_files = total_memory_gb - 2 - 1
if num_input_files < 1:
    num_input_files = 1


# Print the options.
p()
p("Unpruned RT directory: %s" % unpruned_dir)
p("Pruned RT directory: %s" % pruned_dir)
p("Number of input files processed at a time: %u" % num_input_files)

# Loop through the sorted & unpruned source RT files.
files_pruned = 0
total_start_time = time.time()
while True:
    sources = []
    destinations = []

    block_start_time = time.time()
    rt_source_abs = get_unpruned_files(unpruned_dir)
    if rt_source_abs is None:
        p("No unpruned tables remain.")
        break

    #shutil.move(rt_source_abs, rt_destination_abs)
    p("\n\n----------------\n")
    p("Pruning %s...\n\n" % "\n".join(rt_source_abs))

    try:
        args = ['../perfectify'] + rt_source_abs + [pruned_dir]
        proc = subprocess.run(args)
        if proc.returncode != 0:
            p("perfectify returned non-zero exit code (%u).  Terminating..." % proc.returncode)
            exit(-1)
    except FileNotFoundError as e:
        p("Error: could not run ../perfectify!")
        exit(-1)
    except subprocess.SubprocessError as e:
        p("Error: SubprocessError: %s" % e)
        exit(-1)

    for source_file in rt_source_abs:
        sources.append(source_file)

        if not os.path.exists(source_file):
            p("\nFile was possibly fully pruned(!): %s\n" % source_file)
        else:

            # Ensure that the pruned file is a multiple of the chain size.
            file_size = os.path.getsize(source_file)
            if (file_size % 16) != 0:
                p("Error: pruned file is not a multiple of the chain size!: %u" % file_size)
                exit(-1)

            num_chains = file_size / 16

            # Parses the prefix and suffix around the number of chains.
            # Example: ntlm_ascii-32-95#8-8_0_422000x[num chains]_0.rt
            prefix = source_file[0:source_file.rfind('x')+1]
            suffix = source_file[source_file.rfind('_'):]
            rt_source_abs_new = "%s%u%s" % (prefix, num_chains, suffix)

            # Rename the file with the correct number of chains.  Then move it to
            # the pruned directory.
            rt_destination_abs = shutil.move(source_file, rt_source_abs_new)
            rt_destination_abs = shutil.move(rt_destination_abs, pruned_dir)
            destinations.append(rt_destination_abs)
            try:
                proc = subprocess.run(['../crackalack_verify', '--sorted', rt_destination_abs])
                if proc.returncode != 0:
                    p("crackalack_verify returned non-zero exit code (%u).  Terminating..." % proc.returncode)
                    exit(-1)
            except FileNotFoundError as e:
                p("Error: could not run ../crackalack_verify!")
                exit(-1)
            except subprocess.SubprocessError as e:
                p("Error: SubprocessError: %s" % e)
                exit(-1)

    p("\nSuccessfully pruned in %u seconds: \n  %s \n    =>\n  %s\n" % (int(time.time() - block_start_time), "\n  ".join(sources), "\n  ".join(destinations)))
    files_pruned += len(rt_source_abs)

p("\n\nAll %u files pruned in %u seconds.\n" % (files_pruned, int(time.time() - total_start_time)))
exit(0)

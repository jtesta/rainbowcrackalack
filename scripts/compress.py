#!/usr/bin/python3
#
# Rainbow Crackalack: compress.py
# Copyright (C) 2019  Joe Testa <jtesta@positronsecurity.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms version 3 of the GNU General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#


# This program *only* works on the snap-packaged version of the rainbowcrack tools.
# Install them with:
#
#     # snap install --beta rainbowcrack


# TODO: verify RTC files.

import os, shutil, subprocess, sys, tempfile, time


RT2RTC = 'rainbowcrack.rt2rtc'
COMMON_DIR = 'snap/rainbowcrack/common'


def p(s):
  print(s, flush=True)


# Compresses a single RT file in a temp directory using the most efficient values for
# the start and end bits.
#
# Starting with values of 1 each, the rt2rtc program is run on the RT file.  It will
# output the optimal values, which we will parse.  On the third invokation, we will
# have all of the optimal values, and the RTC file will be successfully written to the
# snap/rainbowcrack/common directory.
#
# Terminates the program on error, or returns a tuple containing: the path of the RTC
# file (relative the 'common' directory), the start bits, and the end bits.
def compress_rt(relative_temp_dir):
  start_bits = 1
  end_bits = 1
  rtc_path = None
  for i in range(0, 3):
    args = [RT2RTC, relative_temp_dir, '-s', str(start_bits), '-e', str(end_bits)]

    proc = subprocess.run(args, stdout=subprocess.PIPE)
    output = proc.stdout.decode('ascii')

    # Loop through all the lines of the output of the rt2rtc program.  Parse out the
    # start and end bit values.
    for line in output.split("\n"):
      if line.find('minimal value of start_point_bits is ') != -1:
        start_bits = int(line[41:])
      if line.find('minimal value of end_point_bits   is ') != -1:
        end_bits = int(line[41:])
      if line.find('writing ') != -1:
        rtc_path = line.strip()[8:-3]

    if rtc_path is not None:
      break

  if rtc_path is None:
    print("ERROR: failed to compress file.  :(")
    exit(-1)

  return rtc_path, start_bits, end_bits


# Moves an RTC file from the 'common' directory to the result directory.  Renames the
# file if the destination already exists.  Returns the absolute path of the RTC file
# in the result directory.
def move_rtc(rtc_filename, result_dir):
  rtc_src_path = os.path.join(COMMON_DIR, rtc_filename)

  rtc_dst_path = None
  index = 1
  moved = False
  while moved is False:
    rtc_dst_path = os.path.join(result_dir, rtc_filename)
    if not os.path.exists(rtc_dst_path):
      shutil.move(rtc_src_path, rtc_dst_path)
      moved = True
    else:
      upos = rtc_filename.rfind('_')
      rtc_filename = "%s_%u.rtc" % (rtc_filename[:upos], index)
      index += 1

  return rtc_dst_path


if len(sys.argv) != 4:
  print("\nThis program runs rainbowcrack's rt2rtc program on a directory of RT files.\n\nBecause rt2rtc does not easily handle the case when two RT files produce the same RTC filename, and because it is not clear if it automatically uses the optimal start and end bit values, this program is preferred for compressing rainbow tables.\n\nIt depends on the 'rainbowcrack' snap package (as root, run \"snap install --beta rainbowcrack\").  Hence, it works on Linux only.\n\nPlace a directory of RT files in the ~/snap/rainbowcrack/common/ directory, and run with:\n\n  $ python3 %s [--keep|--delete] relative_rt_directory absolute_rtc_directory\n\nNote that the --keep argument will cause the source files to be preserved after compression; --delete will delete the originals after compression.  Also note that the RT input directory path must be relative to the 'common' directory, but the RTC output directory path is absolute.  i.e.: If the RT files are in ~/snap/rainbowcrack/common/rt_files/, and the output should go in ~/rtc (and source files should be deleted after compression), run the program with:\n\n  $ python3 %s --delete rt_files ~/rtc\n" % (sys.argv[0], sys.argv[0]))
  exit(0)

keep_or_delete = sys.argv[1]
rt_dir = sys.argv[2]  # Relative to COMMON_DIR
result_dir = sys.argv[3]  # Absolute path.

if (keep_or_delete != '--keep') and (keep_or_delete != '--delete'):
  print("Error: first argument must be either '--keep' or '--delete'")
  exit(-1)

keep_sources = True if keep_or_delete == '--keep' else False


# Ensure that the common directory exists.
if not os.path.exists(COMMON_DIR):
  print("Error: cannot find rainbowcrack\'s common directory: %s.  Did you install the rainbowcrack snap package? (Hint: snap install --beta rainbowcrack)\n\nOtherwise, ensure you are in the current user's base home directory.\n" % COMMON_DIR)
  exit(-1)

# Look for existing *.rtc files in the common dir.  Terminate if any are found.
for source_filename in os.listdir(COMMON_DIR):
  if source_filename.endswith('.rtc'):
    p("\nError: an RTC file already exists in %s: %s" % (COMMON_DIR, source_filename))
    exit(-1)

# Check that the RT directory exists within the common directory.
if not os.path.isdir(os.path.join(COMMON_DIR, rt_dir)):
  print("Error: could not find directory named %s in %s." % (rt_dir, COMMON_DIR))
  exit(-1)

# If the result directory path exists, ensure it is a diretory.
if os.path.exists(result_dir) and not os.path.isdir(result_dir):
  print('Error: %s exists, but is not a directory.' % result_dir)
  exit(-1)

# Create a temp dir to copy each RT file into during processing.
temp_dir_abs = tempfile.mkdtemp(dir='snap/rainbowcrack/common')

# Get the relative path (with respect to the common dir) of the temp dir.
relative_temp_dir = temp_dir_abs[temp_dir_abs.rfind('/') + 1:]

# Create the result directory if it does not already exist.
if not os.path.isdir(result_dir):
  os.mkdir(result_dir)

print("\nRTC files will be stored in %s." % result_dir)
total_rt_bytes = 0
total_rtc_bytes = 0

# Put all the source filenames into an array.  Since files will be
# moving in and out of here, this ensures that os.listdir() doesn't
# get confused and returns the same file multiple times.
source_filenames = []
for source_filename in os.listdir(os.path.join(COMMON_DIR, rt_dir)):
  if not source_filename.endswith('.rt'):
    continue

  source_filenames.append(source_filename)

# Sort the filenames by the table index (i.e.: 'table_0.rt', 'table_1.rt', etc).
source_filenames = sorted(source_filenames, key = lambda x: int(x[x.rfind('_')+1:x.rfind('.')]))

# Process each table.
for source_filename in source_filenames:
  source_filename_abs = os.path.join(COMMON_DIR, rt_dir, source_filename)
  source_filename_temp_abs = os.path.join(temp_dir_abs, source_filename)
  shutil.move(source_filename_abs, source_filename_temp_abs)

  compress_start = time.time()
  p("\nCompressing %s..." % source_filename)
  rtc_filename_relative, unused1, unused2 = compress_rt(relative_temp_dir)
  p("  Compression finished in %.1f seconds." % float(time.time() - compress_start))

  # Move the RTC into result directory.
  rtc_filename_abs = move_rtc(rtc_filename_relative, result_dir)

  source_size = os.path.getsize(source_filename_temp_abs)

  if keep_sources:
    # Move the source file back.
    shutil.move(source_filename_temp_abs, source_filename_abs)
  else:
    # Delete the source file.
    os.remove(source_filename_temp_abs)

  rtc_size = os.path.getsize(rtc_filename_abs)
  p("  Compressed size is %.2f GB." % float(rtc_size / (1024 ** 3)))

  total_rtc_bytes += rtc_size
  total_rt_bytes += source_size


os.rmdir(temp_dir_abs)

print("\n\n-------------------------------------\n")
print("Raw table size:        %.2f GB" % (total_rt_bytes / (1024 ** 3)))
print("Compressed table size: %.2f GB" % (total_rtc_bytes / (1024 ** 3)))
print("Compression rate:      %.0f%%" % (((total_rt_bytes - total_rtc_bytes) / total_rt_bytes) * 100))
exit(0)

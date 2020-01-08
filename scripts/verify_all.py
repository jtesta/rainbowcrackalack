#!/usr/bin/python3
#
# Rainbow Crackalack: verify_all.py
# Copyright (C) 2019-2020  Joe Testa <jtesta@positronsecurity.com>
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

#
# This script will run "crackalack_verify" on all *.rt and *.rtc files
# in a target directory.
#

import os, subprocess, sys, time


CRACKALACK_VERIFY = '../crackalack_verify'

def print_usage():
    print("This script will run crackalack_verify on all sorted *.rt and *.rtc files in a target directory.\n\nUsage: %s [--raw|--quick|--sorted] /path/to/dir" % sys.argv[0])
    exit(-1)


if len(sys.argv) != 3:
    print_usage()

verification_type = sys.argv[1]
target_dir = sys.argv[2]

if verification_type not in ['--raw', '--quick', '--sorted']:
    print_usage()

if not os.path.exists(CRACKALACK_VERIFY):
    print("Error: %s not found!" % CRACKALACK_VERIFY)
    exit(-1)

# Count the total number of files in the directory.
num_files = 0
for file in os.listdir(target_dir):
    if not (file.endswith('.rt') or file.endswith('.rtc')):
        continue

    num_files += 1

start_time = time.time()
file_number = 0
for file in os.listdir(target_dir):
    if not (file.endswith('.rt') or file.endswith('.rtc')):
        continue

    try:
        file_number += 1
        print("[%u of %u] Verifying %s..." % (file_number, num_files, file))

        file_start_time = time.time()

        # Run the crackalack_verify tool on the target table.  Notice that stderr is not redirected, hence it will be printed to the user automatically if there is a problem.
        proc = subprocess.run([CRACKALACK_VERIFY, verification_type, os.path.join(target_dir, file)], stdout=subprocess.PIPE)
        if proc.returncode != 0:
            print("\n\nFAILED!  %s return non-zero exit code: %d\nStdout: [%s]\nStderr: [%s]\n" % (CRACKALACK_VERIFY, proc.returncode, proc.stdout.decode('ascii'), proc.stdout.decode('ascii')))
            exit(-1)

        print("Finished in %.1f seconds.\n" % float(time.time() - file_start_time))
    except FileNotFoundError as e:
        print("Error: could not run %s!" % CRACKALACK_VERIFY)
        exit(-1)
    except subprocess.SubprocessError as e:
        print("Error: SubprocessError: %s" % e)
        exit(-1)

print("\n\nSUCCESS.  Processed %u files in %.1f minutes (verification type: %s)." % (num_files, float(time.time() - start_time) / 60.0, verification_type[2:]))

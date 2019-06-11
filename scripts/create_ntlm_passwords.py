#!/usr/bin/python3
#
# Rainbow Crackalack: create_ntlm_passwords.py
# Copyright (C) 2018-2019  Joe Testa <jtesta@positronsecurity.com>
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


# This tool generates random passwords of a certain length and calculates their NTLM
# hashes.  It is useful for testing the effectiveness of a set of rainbow tables.

import hashlib, os, sys

CHARSET_ASCII_32_95 = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"

passwords = []
hashes = []

if (len(sys.argv) != 3):
    print("Usage: %s [number of chars] [number of passwords]\n\nProgram will generate the specified number random passwords, each composed of the specified number of characters.  For example, to generate 1,000 passwords of length 8:\n\n  %s 8 1000\n" % (sys.argv[0], sys.argv[0]))
    sys.exit(-1)

num_pw_chars = int(sys.argv[1])
num_passwords = int(sys.argv[2])
if (num_pw_chars < 1) or (num_pw_chars > 16):
    print("Number of characters must be between 1 and 16.")
    sys.exit(-1)
elif (num_passwords < 1):
    print("Number of passwords must be greater than 0.")
    sys.exit(-1)

print("Generating %d %d-character random passwords..." % (num_passwords, num_pw_chars))
for i in range(0, num_passwords):
    rand_bytes = os.urandom(num_pw_chars)
    password = ''
    for j in range(num_pw_chars):
        password = password + CHARSET_ASCII_32_95[ rand_bytes[j] % 95 ]
    passwords.append(password)
    hashes.append(hashlib.new('md4', password.encode('utf-16le')).hexdigest())

hash_filename = "random_ntlm_hashes_%d_chars.txt" % num_pw_chars
passwords_filename = "random_passwords_%d_chars.txt" % num_pw_chars

# Write the hashes to the file.
with open(hash_filename, 'w') as f:
    f.write("\n".join(hashes))

# In a separate file, write the corresponding plaintext passwords.
with open(passwords_filename, 'w') as f:
    f.write("\n".join(passwords))

print("\nNTLM hashes stored in: %s\nPlaintext passwords stored in: %s" % (hash_filename, passwords_filename))

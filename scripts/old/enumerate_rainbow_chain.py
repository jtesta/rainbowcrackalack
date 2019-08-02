#!/usr/bin/python3
#
# Rainbow Crackalack: enumerate_rainbow_chain.py
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


# This program will print all the plaintexts & hashes for a particular rainbow chain.
# It currently only supports NTLM hashes in the ascii-32-95 charset.

import hashlib, sys


def table_index_to_reduction_offset(table_index):
  return table_index * 65536


def fill_plaintext_space_table(charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index):
  n = 1

  plaintext_space_up_to_index[0] = 0
  for i in range(1, plaintext_len_max + 1):
    n = n * charset_len
    if (i < plaintext_len_min):
      plaintext_space_up_to_index[i] = 0
    else:
      plaintext_space_up_to_index[i] = plaintext_space_up_to_index[i - 1] + n

  return plaintext_space_up_to_index[plaintext_len_max]


def index_to_plaintext(index, charset, charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index):

  plaintext = ''
  plaintext_len = 0


  if plaintext_len_min == 9:
    for i in range(0, 9):
      plaintext = plaintext + charset[ (index & 0xff) % charset_len ]
      index = index >> 7

    return plaintext


  for i in range(plaintext_len_max - 1, plaintext_len_min - 1 - 1, -1):
    if (index >= plaintext_space_up_to_index[i]):
      plaintext_len = i + 1
      break

  index_x = index - plaintext_space_up_to_index[plaintext_len - 1]
  for i in range(plaintext_len - 1, -1, -1):
    plaintext = charset[index_x % charset_len] + plaintext
    index_x = int(index_x / charset_len)

  return plaintext


def do_hash(plaintext):
  return hashlib.new('md4', plaintext.encode('utf-16le')).digest()


def hash_to_index(hash, reduction_offset, plaintext_space_total, pos):
    return (int.from_bytes(hash, byteorder='little') + reduction_offset + pos) % plaintext_space_total;


def print_plaintexts(start_index, end_index, charset, charset_len, plaintext_space_up_to_index, plaintext_len_min, plaintext_len_max, reduction_offset, chain_len):

  print("Position\tPlaintext\tHash\tHash Index")
  
  index = start_index
  for pos in range(0, chain_len - 1):
    plaintext = index_to_plaintext(index, charset, charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index)

    hash = do_hash(plaintext)

    print("%u\t%s\t%s\t" % (pos, plaintext, hash.hex()), end='')
    index = hash_to_index(hash[0:8], reduction_offset, plaintext_space_total, pos)
    print(index)

  if index != end_index:
    print("\nERROR: end index is expected to be %u, but computed %u.\n" % (end_index, index))

  return


def run_self_tests():
  plaintext_space_up_to_index = [None] * 16
  charset = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
  charset_len = len(charset)

  plaintext_space_total = fill_plaintext_space_table(charset_len, 8, 8, plaintext_space_up_to_index);

  # Self-test: index_to_plaintext()
  plaintext = index_to_plaintext(5222991064626285, charset, charset_len, 8, 8, plaintext_space_up_to_index)
  if plaintext != 'jk5(J-f\\':
    print("index_to_plaintext() failed self-test! Expected: %s. Got: %s." % ('jk5(J-f\\', plaintext))
    exit(-1)

    # Self-test: do_hash()
    hash = do_hash('C1t1z3n#')[0:8]
    if hash.hex() != 'ff0bc475edd85a6a':
      print("do_hash() failed self-test! Expected: %s. Got: %s." % ('ff0bc475edd85a6a', hash.hex()))
      exit(-1)

      # Self-test: hash_to_index()
      index = hash_to_index(b"\x12\x34\x56\x78\x9a\xbc\xde\xf0", 0, plaintext_space_total, 666)
      if (index != 1438903040496756):
        print("hash_to_index() failed self-test! Expected: %u. Got: %u.\n" % (1438903040496756, index))
        exit(-1)


run_self_tests()

if len(sys.argv) != 5:
  print("\nThis script takes a start index, end index, the plaintext min & max (which must both be the same), and a chain length, then outputs all the plaintexts.  It is assumed that the us-ascii-32-95 character set is in use, along with a table index of 0.")
  print()
  print("Usage:\n\n  %s start_index end_index plaintext_min_max chain_length\n" % sys.argv[0])
  exit(-1)

start_index = int(sys.argv[1])
end_index = int(sys.argv[2])
min_max = int(sys.argv[3])
chain_len = int(sys.argv[4])

table_index = 0
plaintext_len_min = plaintext_len_max = min_max

plaintext_space_up_to_index = [None] * 16
charset = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
charset_len = len(charset)


print("\nEnumerating chain (%u, %u) using the following parameters:\n\tcharset: ascii-32-95\n\tplaintext len min: %u\n\tplaintext len max: %u\n\ttable index: %u\n\tchain len: %u\n" % (start_index, end_index, plaintext_len_min, plaintext_len_max, table_index, chain_len))

plaintext_space_total = fill_plaintext_space_table(charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index);

print_plaintexts(start_index, end_index, charset, charset_len, plaintext_space_up_to_index, plaintext_len_min, plaintext_len_max, table_index_to_reduction_offset(table_index), chain_len)

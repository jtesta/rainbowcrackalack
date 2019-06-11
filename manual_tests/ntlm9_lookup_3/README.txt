This is a very short test that checks NTLM9 lookups succeed on a very basic level.

Start by generating a standard NTLM9 index 0 table:

  ./crackalack_gen ntlm ascii-32-95 9 9 0 1350000 67108864 0

Then sort this table.  The SHA256 hash of the sorted table should be:

  413d0dbe9c4630fde5c5a1c72051941a8316fe28083bb31a7c7dc3dc9d7d8a2f

Perform a lookup on the three hashes in 'three_ntlm9_hashes.txt' against this table.  All three hashes should be cracked.

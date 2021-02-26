This is a very short test that checks NTLM9 lookups succeed on a very basic level.

Either the first table in the NTLM9 set can be used for lookups, or the table can be generated.  To use the existing table in the NTLM9 set, copy 'ntlm_ascii-32-95#9-9_0_803000x67108864_0.rtc' into its own directory (then skip below to LOOKUP STEP).

Otherwise, to generate the table:

  # ./crackalack_gen ntlm ascii-32-95 9 9 0 803000 67108864 0

The SHA256 hash of the raw table should be:

  0781255260c82ed8832b2798466c58db4dd7921ea93f3e9f1f8c7f69dda65851

Then sort this table.  The hash of the sorted table should be:

  c513fc977099ab7de44f0eabe12e0c20b0eda836ac565d3b36d4a3b9b51dd076


LOOKUP STEP

The following command will look up the four NTLM9 hashes in the table:

  # ./crackalack_lookup /path/to/table manual_tests/ntlm9_lookup_4/four_ntlm9_hashes.txt

All four hashes should be cracked:

  aca387940078ec9e6586aae02a1e72d2 => )93Q"n2Ye
  ad28df3edfb20fd6b641345ddc567db9 => VfrOEpHsY
  775389d1da199b8068aafd5740d7f050 => _-%Z2)ZKE
  32ffbb2147248c7edd1af0da340b2747 => zd{uslu0i

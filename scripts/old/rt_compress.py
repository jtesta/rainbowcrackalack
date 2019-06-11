#!/usr/bin/python3
#
# Rainbow Crackalack: rt_compress.py
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


# This program converts raw rainbow tables into optimized, sorted, and compressed
# tables that are ready for production use.
#
# This program *only* works on the snap-packaged version of the rainbowcrack tools.
# Install them with:
#
#     # snap install --beta rainbowcrack


#
# Output of production run on raw 8-character NTLM tables.  This was run on an
# AMD Threadripper 1950X with 32 GB of memory:
#


# 
# RTC files will be stored in /home/pwn0r/rtc.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_0.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_31.rt
#   Merging completed in 151 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 4114 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 510 seconds.
#   Size of compressed blob is 16.85 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 128 seconds.
#   Size of uncompressed blob is 29.95 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_0.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_28.rt.
#   Short file found (537782448 bytes / 33611403 chains).  Processing next round.
#   Split 1 GB chunks in 133 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_0.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_28.rtc
#   Re-compressed 1 GB blocks in 172 seconds.
# 
# Fully processed 32 GB of tables in 5213 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 537782448 bytes / 33611403 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_32.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_63.rt
#   Merging completed in 152 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 8396 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 519 seconds.
#   Size of compressed blob is 19.00 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 142 seconds.
#   Size of uncompressed blob is 30.39 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_32.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_60.rt.
#   Short file found (1011327440 bytes / 63207965 chains).  Processing next round.
#   Split 1 GB chunks in 137 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_29.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_57.rtc
#   Re-compressed 1 GB blocks in 166 seconds.
# 
# Fully processed 32 GB of tables in 9517 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 1011327440 bytes / 63207965 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_64.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_95.rt
#   Merging completed in 154 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 12010 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 533 seconds.
#   Size of compressed blob is 21.17 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 147 seconds.
#   Size of uncompressed blob is 30.79 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_64.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_93.rt.
#   Short file found (340691024 bytes / 21293189 chains).  Processing next round.
#   Split 1 GB chunks in 137 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_58.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_87.rtc
#   Re-compressed 1 GB blocks in 173 seconds.
# 
# Fully processed 32 GB of tables in 13159 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 340691024 bytes / 21293189 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_96.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_127.rt
#   Merging completed in 154 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 1669 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 516 seconds.
#   Size of compressed blob is 18.90 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 142 seconds.
#   Size of uncompressed blob is 30.24 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_96.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_124.rt.
#   Short file found (839761408 bytes / 52485088 chains).  Processing next round.
#   Split 1 GB chunks in 137 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_88.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_116.rtc
#   Re-compressed 1 GB blocks in 163 seconds.
# 
# Fully processed 32 GB of tables in 2786 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 839761408 bytes / 52485088 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_128.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_159.rt
#   Merging completed in 151 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 1383 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 529 seconds.
#   Size of compressed blob is 21.07 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 147 seconds.
#   Size of uncompressed blob is 30.65 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_128.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_157.rt.
#   Short file found (190164752 bytes / 11885297 chains).  Processing next round.
#   Split 1 GB chunks in 135 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_117.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_146.rtc
#   Re-compressed 1 GB blocks in 175 seconds.
# 
# Fully processed 32 GB of tables in 2525 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 190164752 bytes / 11885297 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_160.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_191.rt
#   Merging completed in 155 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 10880 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 519 seconds.
#   Size of compressed blob is 18.82 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 137 seconds.
#   Size of uncompressed blob is 30.11 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_160.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_188.rt.
#   Short file found (707347744 bytes / 44209234 chains).  Processing next round.
#   Split 1 GB chunks in 139 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_147.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_175.rtc
#   Re-compressed 1 GB blocks in 164 seconds.
# 
# Fully processed 32 GB of tables in 11999 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 707347744 bytes / 44209234 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_192.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_223.rt
#   Merging completed in 156 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 1320 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 528 seconds.
#   Size of compressed blob is 20.99 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 147 seconds.
#   Size of uncompressed blob is 30.54 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_192.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_221.rt.
#   Short file found (72765600 bytes / 4547850 chains).  Processing next round.
#   Split 1 GB chunks in 139 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_176.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_205.rtc
#   Re-compressed 1 GB blocks in 174 seconds.
# 
# Fully processed 32 GB of tables in 2470 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 72765600 bytes / 4547850 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_224.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_255.rt
#   Merging completed in 159 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 1231 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 517 seconds.
#   Size of compressed blob is 18.76 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 136 seconds.
#   Size of uncompressed blob is 30.01 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_224.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_252.rt.
#   Short file found (603276144 bytes / 37704759 chains).  Processing next round.
#   Split 1 GB chunks in 137 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_206.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_234.rtc
#   Re-compressed 1 GB blocks in 163 seconds.
# 
# Fully processed 32 GB of tables in 2348 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 603276144 bytes / 37704759 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_256.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_287.rt
#   Merging completed in 154 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 5216 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 527 seconds.
#   Size of compressed blob is 20.93 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 147 seconds.
#   Size of uncompressed blob is 30.45 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_256.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_284.rt.
#   Short file found (1070153168 bytes / 66884573 chains).  Processing next round.
#   Split 1 GB chunks in 137 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_235.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_263.rtc
#   Re-compressed 1 GB blocks in 166 seconds.
# 
# Fully processed 32 GB of tables in 6353 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 1070153168 bytes / 66884573 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_288.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_319.rt
#   Merging completed in 154 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 1418 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 537 seconds.
#   Size of compressed blob is 21.20 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 144 seconds.
#   Size of uncompressed blob is 30.84 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_288.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_317.rt.
#   Short file found (393331488 bytes / 24583218 chains).  Processing next round.
#   Split 1 GB chunks in 140 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_264.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_293.rtc
#   Re-compressed 1 GB blocks in 175 seconds.
# 
# Fully processed 32 GB of tables in 2573 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 393331488 bytes / 24583218 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_320.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_351.rt
#   Merging completed in 158 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 9823 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 529 seconds.
#   Size of compressed blob is 20.82 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 141 seconds.
#   Size of uncompressed blob is 30.28 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_320.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_348.rt.
#   Short file found (887543344 bytes / 55471459 chains).  Processing next round.
#   Split 1 GB chunks in 135 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_294.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_322.rtc
#   Re-compressed 1 GB blocks in 167 seconds.
# 
# Fully processed 32 GB of tables in 10958 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 887543344 bytes / 55471459 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_352.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_383.rt
#   Merging completed in 162 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 1365 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 532 seconds.
#   Size of compressed blob is 21.10 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 148 seconds.
#   Size of uncompressed blob is 30.69 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_352.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_381.rt.
#   Short file found (233625696 bytes / 14601606 chains).  Processing next round.
#   Split 1 GB chunks in 136 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_323.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_352.rtc
#   Re-compressed 1 GB blocks in 170 seconds.
# 
# Fully processed 32 GB of tables in 2517 seconds.
# 
# 
# -------------------------------------
# 
# Merging RT files into 32 GB blob...
#   Added 233625696 bytes / 14601606 chains left over from last block.
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_384.rt
# [...]
#   Adding to 32 GB blob.rt: snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_399.rt
# 
# Source tables exhausted (snap/rainbowcrack/common/NTLM_8chars_raw/ntlm_ascii-32-95#8-8_0_422000x67108864_400.rt not found).
#   Merging completed in 77 seconds.
# 
# Sorting 32GB blob...
#   Sorting 32 GB completed in 471 seconds.
# 
# Compressing 32 GB blob...
#   Compressed 32 GB blob in 74 seconds.
#   Size of compressed blob is 10.78 GB.
# 
# Uncompressing 32 GB blob...
#   Uncompressed 32 GB blob in 25 seconds.
#   Size of uncompressed blob is 15.68 GB.
# 
# Splitting uncompressed, sorted, and pruned blob into 1 GB chunks...
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_384.rt.
# [...]
#   Wrote 1.00 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_398.rt.
#   Wrote 0.44 GB to snap/rainbowcrack/common/tmpjqu9i95k/ntlm_ascii-32-95#8-8_0_422000x67108864_399.rt.
#   Split 1 GB chunks in 32 seconds.
# 
# Re-compressing 1 GB blocks...
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_353.rtc
# [...]
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x67108864_367.rtc
#   Created final RTC: /home/pwn0r/rtc/ntlm_ascii-32-95#8-8_0_422000x29730227_0.rtc
#   Re-compressed 1 GB blocks in 70 seconds.
# 
# Fully processed 32 GB of tables in 756 seconds.
# 
# 
# -------------------------------------
# 
# Raw table size:        400.00 GB
# Compressed table size: 207.49 GB
# Compression rate:      48%
#


import os, shutil, subprocess, sys, tempfile, time


RT2RTC = 'rainbowcrack.rt2rtc'
RTC2RT = 'rainbowcrack.rtc2rt'
COMMON_DIR = 'snap/rainbowcrack/common'

TABLE_SIZE = 1024 * 1024 * 1024  #  1 GB
FILE_BUF_SIZE = 1024 * 1024 * 16 # 16 MB


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
    args = [RT2RTC, relative_temp_dir, '-s', str(start_bits), '-e', str(end_bits), '-c', '32768', '-p']
    #print("args: %s" % " ".join(args))

    proc = subprocess.run(args, stdout=subprocess.PIPE)
    output = proc.stdout.decode('ascii')
    #print(output)

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


if len(sys.argv) == 1:
  print("\nThis program runs rainbowcrack's rt2rtc program on a directory of RT files.\n\nBecause rt2rtc does not easily handle the case when two RT files produce the same RTC filename, and because it is not clear if it automatically uses the optimal start and end bit values, this program is preferred for compressing rainbow tables.\n\nIt depends on the 'rainbowcrack' snap package (as root, run \"snap install --beta rainbowcrack\").  Hence, it works on Linux only.\n\nPlace a directory of RT files in the ~/snap/rainbowcrack/common/ directory, and run with:\n\n  $ python3 %s relative_rt_directory absolute_rtc_directory\n\nNote that the RT input directory path must be relative to the 'common' directory, but the RTC output directory path is absolute.  i.e.: If the RT files are in ~/snap/rainbowcrack/common/rt_files/, and the output should go in ~/rtc, run the program with:\n\n  $ python3 %s rt_files ~/rtc\n" % (sys.argv[0], sys.argv[0]))
  exit(0)

rt_dir = sys.argv[1]  # Relative to COMMON_DIR
result_dir = sys.argv[2]  # Absolute path.
#original_cwd = os.getcwd()


# Ensure that the common directory exists.
if not os.path.exists(COMMON_DIR):
  print("Error: cannot find rainbowcrack\'s common directory: %s.  Did you install the rainbowcrack snap package? (Hint: snap install --beta rainbowcrack)\n\nOtherwise, ensure you are in the current user's base home directory.\n" % COMMON_DIR)
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
temp_dir = tempfile.mkdtemp(dir='snap/rainbowcrack/common')
temp_dir_single = tempfile.mkdtemp(dir='snap/rainbowcrack/common')

# Get the relative path (with respect to the common dir) of the temp dir.
relative_temp_dir = temp_dir[temp_dir.rfind('/') + 1:]
relative_temp_dir_single = temp_dir_single[temp_dir_single.rfind('/') + 1:]

# Create the result directory if it does not already exist.
if not os.path.isdir(result_dir):
  os.mkdir(result_dir)

print("\nRTC files will be stored in %s." % result_dir)
#print("Processing %s/*.rt...\n\n" % os.path.join(COMMON_DIR, rt_dir))
total_rt_bytes = 0
total_rtc_bytes = 0

# Get the basename of the first *.rt file ("ntlm_ascii-32-95#8-8_0_100x1024_").  With
# this, we can iterate from _0.rt, _1.rt, etc., in order (note that directory listings
# are sometimes unordered).
base_filename = None
for filename in os.listdir(os.path.join(COMMON_DIR, rt_dir)):
   if filename.endswith('.rt'):

     upos = filename.rfind('_')
     if upos != -1:
       base_filename = filename[0:upos + 1]
       break

if base_filename is None:
  print("Error: could not determine base filename!")
  exit(-1)

#print(base_filename)
current_file_num = 0

blob_filename = os.path.join(temp_dir, 'blob.rt')
blob_filename_relative = os.path.join(relative_temp_dir, 'blob.rt')

out_of_files = False
while not out_of_files:

  #
  # Merge the unsorted tables into one 32 GB blob.
  #

  print("\n\n-------------------------------------", flush=True)
  print("\nMerging RT files into 32 GB blob...", flush=True)
  block_start = time.time()
  merge_start = time.time()
  with open(blob_filename, 'wb') as blob:

    # If any bytes from the previous run were left over, add that to this blob first.
    if os.path.exists('leftover.rt'):
      leftover_len = 0
      with open('leftover.rt', 'rb') as lo:
        buf = lo.read(FILE_BUF_SIZE)
        while buf != b'':
          leftover_len += blob.write(buf)
          buf = lo.read(FILE_BUF_SIZE)
      os.unlink('leftover.rt')
      print("  Added %u bytes / %u chains left over from last block." % (leftover_len, leftover_len / 16))

    # Iterate over the next 32 unsorted tables and append them to the blob.
    for i in range(current_file_num, current_file_num + 32):
      rt_filename = os.path.join(COMMON_DIR, rt_dir, "%s%d.rt" % (base_filename, i))

      if os.path.exists(rt_filename):
        print("  Adding to 32 GB blob.rt: %s" % rt_filename, flush=True)

        # Read blocks out of the raw table file, and write them to the blob file.
        with open(rt_filename, 'rb') as rt:
          buf = rt.read(FILE_BUF_SIZE)
          while buf != b'':
            total_rt_bytes += blob.write(buf)
            buf = rt.read(FILE_BUF_SIZE)
      else:
        print("\nSource tables exhausted (%s not found)." % rt_filename, flush=True)
        out_of_files = True
        break

  print("  Merging completed in %u seconds." % int(time.time() - merge_start), flush=True)

  if out_of_files and os.path.getsize(blob_filename) == 0:
    print("DONE", flush=True)
    os.unlink(blob_filename)
    break

  # Rename from 'blob.rt' to something like 'ntlm_ascii-32-95#8-8_0_100x32768_0.rt',
  # since the compression tool doesn't like vague file names.
  xpos = base_filename.rfind('x')
  if xpos == -1:
    print("Error: failed to parse basename: %s" % base_filename, flush=True)
    exit(-1)

  new_blob_filename = "%s%d_0.rt" % (base_filename[0:xpos + 1], os.path.getsize(blob_filename) / 16)
  new_path = os.path.join(temp_dir, new_blob_filename)
  shutil.move(blob_filename, new_path)
  blob_filename = new_path
  blob_filename_relative = os.path.join(relative_temp_dir, new_blob_filename)
  #print("  Renamed unsorted blob from 'blob.rt' to: %s." % blob_filename)


  #
  # Now sort the blob.
  #

  print("\nSorting 32GB blob...", flush=True)
  sort_start = time.time()
  proc = subprocess.run(['rainbowcrack.rtsort', relative_temp_dir], stdout=subprocess.PIPE)
  if proc.stdout.decode('ascii').find('writing sorted data') == -1:
    print("\nSorting failed. :(", flush=True)
    exit(-1)
  print("  Sorting 32 GB completed in %u seconds." % int(time.time() - sort_start), flush=True)


  #
  # Compress the blob.
  #
  
  print("\nCompressing 32 GB blob...", flush=True)
  compress_start = time.time()
  rtc_filename_relative, unused1, unused2 = compress_rt(relative_temp_dir)
  print("  Compressed 32 GB blob in %u seconds." % int(time.time() - compress_start), flush=True)

  # Remove the uncompressed blob.
  os.unlink(blob_filename)

  # Move the RTC into the temp dir.
  shutil.move(os.path.join(COMMON_DIR, rtc_filename_relative), temp_dir)

  
  rtc_filename_relative = os.path.join(temp_dir, rtc_filename_relative)
  print("  Size of compressed blob is %.2f GB." % (os.path.getsize(rtc_filename_relative) / (1024 ** 3)), flush=True)


  #
  # Uncompress it again.
  #

  print("\nUncompressing 32 GB blob...", flush=True)
  uncompress_start = time.time()
  proc = subprocess.run(['rainbowcrack.rtc2rt', relative_temp_dir], stdout=subprocess.PIPE)
  if proc.stdout.decode('ascii').find('converting') == -1:
    print("\nUncompressing failed.  :(", flush=True)
    exit(-1)

  print("  Uncompressed 32 GB blob in %u seconds." % int(time.time() - uncompress_start), flush=True)

    
  # Remove the compressed & sorted blob.
  os.unlink(rtc_filename_relative)
  #print("  Deleted: %s" % rtc_filename_relative)

  # The filename may be different because chains were pruned...
  for f in os.listdir(temp_dir):
    if f.endswith('.rt'):
      blob_filename = os.path.join(temp_dir, f)

  print("  Size of uncompressed blob is %.2f GB." % (os.path.getsize(blob_filename) / (1024 ** 3)), flush=True)


  #
  # Split 1 GB chunks from 32 GB uncompressed sorted blob.
  #

  print("\nSplitting uncompressed, sorted, and pruned blob into 1 GB chunks...", flush=True)
  split_start = time.time()
  with open(blob_filename, 'rb') as blob:
    for i in range(current_file_num, current_file_num + 32):
      rt_filename = os.path.join(temp_dir, "%s%d.rt" % (base_filename, i))
      with open(rt_filename, 'wb') as rt:

        # While debugging with small tables, just read the chunk entirely at once.
        if FILE_BUF_SIZE > TABLE_SIZE:
          rt.write(blob.read(TABLE_SIZE))
        else: # Production mode with really big files: read in 16 MB blocks.
          buf = blob.read(FILE_BUF_SIZE)
          len_written = 0
          while (buf != b'') and (len_written < TABLE_SIZE):
            len_written += rt.write(buf)
            buf = blob.read(FILE_BUF_SIZE)

      rt_size = os.path.getsize(rt_filename)
      if rt_size == 0:
        os.unlink(rt_filename)

      # If a partial table was written, rename it to 'leftover.rt' so its handled in
      # the next loop.
      elif (rt_size != TABLE_SIZE) and not out_of_files:
        print("  Short file found (%u bytes / %u chains).  Processing next round." % (rt_size, rt_size / 16))
        shutil.move(rt_filename, 'leftover.rt')
      else:
        print("  Wrote %.2f GB to %s." % (os.path.getsize(rt_filename) / (1024 ** 3), rt_filename), flush=True)

    # Write (or append) what's left in the blob to 'leftover.rt' for the next loop to
    # handle.
    leftover = blob.read(FILE_BUF_SIZE)
    leftover_len = 0
    f = None
    if len(leftover) > 0:
      if os.path.exists('leftover.rt'):
        f = open('leftover.rt', 'r+b')
      else:
        f = open('leftover.rt', 'wb')

      while leftover != b'':
        leftover_len += f.write(leftover)
        leftover = blob.read(FILE_BUF_SIZE)
      f.close()
      print("  Left-over %u bytes / %u chains will be processed next round." % (leftover_len, leftover_len / 16), flush=True)
    leftover = None

  print("  Split 1 GB chunks in %u seconds." % int(time.time() - split_start), flush=True)

  # Delete the uncompressed sorted blob.
  os.unlink(blob_filename)


  #
  # Compress the individual 1 GB tables.
  #

  print("\nRe-compressing 1 GB blocks...", flush=True)

  recompress_start = time.time()
  for i in range(current_file_num, current_file_num + 32):
    rt_filename = os.path.join(temp_dir, "%s%d.rt" % (base_filename, i))
    if os.path.exists(rt_filename):
      shutil.move(rt_filename, temp_dir_single)

      rtc_filename_relative, unused1, unused2 = compress_rt(relative_temp_dir_single)
      rtc_path = move_rtc(rtc_filename_relative, result_dir)
      print("  Created final RTC: %s" % rtc_path, flush=True)
      total_rtc_bytes += os.path.getsize(rtc_path)

      for f in os.listdir(temp_dir_single):
        if f.endswith('.rt'):
          os.unlink(os.path.join(temp_dir_single, f))

  print("  Re-compressed 1 GB blocks in %u seconds." % int(time.time() - recompress_start), flush=True)

  print("\nFully processed 32 GB of tables in %u seconds." % int(time.time() - block_start), flush=True)
  current_file_num += 32

os.rmdir(temp_dir_single)
os.rmdir(temp_dir)

print("\n\n-------------------------------------\n")
print("Raw table size:        %.2f GB" % (total_rt_bytes / (1024 ** 3)))
print("Compressed table size: %.2f GB" % (total_rtc_bytes / (1024 ** 3)))
print("Compression rate:      %.0f%%" % (((total_rt_bytes - total_rtc_bytes) / total_rt_bytes) * 100))
exit(0)

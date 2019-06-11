#!/usr/bin/python3
#
# Rainbow Crackalack: make_rts.py
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


# This will create 96 rainbow tables with 1K chains each.  Useful for
# testing the rt_compress.py script.

import subprocess

for i in range(0, 96):
    proc = subprocess.run(['rainbowcrack.rtgen', 'ntlm', 'ascii-32-95', '8', '8', '0', '100', '1024', str(i)])

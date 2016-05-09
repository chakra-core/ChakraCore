#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Python 2.7 and 3.x compatibility for easier testing regardless of python installation
from __future__ import print_function

import sys
import os.path
import re

copyright_lines = [
    r'-------------------------------------------------------------------------------------------------------',
    r' Copyright \(C\) Microsoft( Corporation and contributors)?\. All rights reserved\.',
    r' Licensed under the MIT license\. See LICENSE\.txt file in the project root for full license information\.',
    r'.*' # the above should always be followed by at least one other line, so make sure that line is present
]

regexes = []
for line in copyright_lines:
    pattern = '^.{1,5}%s$' % line
    regexes.append(re.compile(pattern))

if len(sys.argv) < 2:
    print("Requires passing a filename as an argument.")
    exit(1)

file_name = sys.argv[1]
if not os.path.isfile(file_name):
    print("File does not exist:", file_name, "(not necessarily an error)")
    exit(0)

def report_incorrect(file_name, pairs):
    # found a problem so report the problem to the caller and exit
    print(file_name, "... does not contain a correct Microsoft copyright notice.\n")
    # print the relevant lines to help the reader find the problem
    for (_, line) in pairs:
        print("    ", line, end="")
    print()

linecount = 0
pairs = []
with open(file_name, 'r') as sourcefile:
    hashbang = sourcefile.readline()
    if not hashbang.startswith("#!"):
        sourcefile.seek(0)
    pairs += zip(regexes, sourcefile)

for (regex, line) in pairs:
    linecount += 1
    line = line.rstrip()
    matches = regex.match(line)
    if not matches:
        report_incorrect(file_name, pairs)
        exit(1)

if linecount == 0:
    # the file was empty (e.g. dummy.js) so no problem
    exit(0)
elif linecount != len(regexes):
    report_incorrect(file_name, pairs)
    exit(1)

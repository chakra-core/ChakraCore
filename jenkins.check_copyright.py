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
    r' Copyright \(C\) Microsoft\. All rights reserved\.',
    r' Licensed under the MIT license\. See LICENSE\.txt file in the project root for full license information\.'
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

linecount = 0
with open(file_name, 'r') as sourcefile:
    for x in range(0,len(copyright_lines)):
        try:
            line = next(sourcefile)
            linecount += 1
        except StopIteration:
            if linecount == 0:
                # the file was empty (e.g. dummy.js) so no problem
                exit(0)
            else:
                # file didn't have enough lines to have a complete copyright notice
                print(file_name, "... does not contain a correct Microsoft copyright notice.")
                exit(1)

        line = line.rstrip()
        matches = regexes[x].match(line)
        if not matches:
            print(file_name, "... does not contain a correct Microsoft copyright notice.")
            # found a problem so exit and report the problem to the caller
            exit(1)

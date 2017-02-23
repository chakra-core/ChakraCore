#!/usr/bin/env python
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

from __future__ import print_function, with_statement # py 2.5
import sys
import os

def print_usage():
    print("jstoc tool")
    print("creates a (const char array) from a javascript file.")
    print("")
    print("usage: jstoc.py <js file path> <variable name>")
    sys.exit(1)

def convert():
    if len(sys.argv) < 3:
        print_usage()

    js_file_name = sys.argv[1]
    if os.path.isfile(js_file_name) == False:
        print_usage()

    h_file_name = js_file_name + '.h'

    js_file_time = os.path.getmtime(js_file_name)
    h_file_time = 0
    if os.path.isfile(h_file_name):
        h_file_time = os.path.getmtime(h_file_name)

    str_header = "static const char " + sys.argv[2] + "[] = {\n"

    # if header file is up to date and no update to jstoc.py since, skip..
    if h_file_time < js_file_time or h_file_time < os.path.getmtime(sys.argv[0]):
        with open(js_file_name, "rb") as js_file:
            last_break = len(str_header) # skip first line
            byte = js_file.read(1)
            while byte:
                str_header += str(ord(byte))
                str_header += ","
                # beautify a bit. limit column to ~80
                column_check = (len(str_header) + 1) - last_break
                if column_check > 5 and column_check % 80 < 5:
                    last_break = len(str_header) + 1
                    str_header += "\n"
                else:
                    str_header += " "
                byte = js_file.read(1)
            str_header += "0\n};"

        h_file = open(h_file_name, "w")
        h_file.write(str_header)
        h_file.close()
        print("-- " + h_file_name + " is created")
    else:
        print("-- " + h_file_name + " is up to date. skipping.")

if __name__ == '__main__':
    sys.exit(convert())
else:
    print("try without python")

#!/usr/bin/env python
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

import sys
import os
import subprocess as SP

test_all = True
test_root = os.path.dirname(os.path.realpath(__file__))

# ugly trick
ch_path = os.path.join(os.path.dirname(test_root), "BuildLinux/ch")

if not os.path.isfile(ch_path):
    print "BuildLinux/ch not found. Did you run ./build.sh already?"
    sys.exit(1)

if len(sys.argv) > 1:
    if sys.argv[1] in ['-?', '--help']:
        print "ChakraCore *nix Test Script\n"

        print "Usage:"
        print "test.py <optional test path>\n"

        print "-?, --help    : Show help\n"

        print "Samples:"
        print "test only Array:"
        print "\t./test.py Array\n"
        
        print "test a single file:"
        print "\t./test.py Basics/hello.js\n"

        print "test all folders:"
        print "\t./test.py"
        sys.exit(0)
    test_all = None

test_dirs=['']
if test_all:
    test_dirs = os.listdir(test_root)
else:
    test_dirs[0] = sys.argv[1]

def show_failed(filename, output, exit_code, expected_output):
    print "\nFailed ->", filename
    if expected_output == None:
        print "\nOutput:"
        print "----------------------------"
        print output
        print "----------------------------"
    else:
        lst_output = output.split('\n')
        lst_expected = expected_output.split('\n')
        ln = min(len(lst_output), len(lst_expected))
        for i in range(0, ln):
            if lst_output[i] != lst_expected[i]:
                print "Output: (at line " + str(i) + ")" 
                print "----------------------------"
                print lst_output[i]
                print "----------------------------"
                print "Expected Output:"
                print "----------------------------"
                print lst_expected[i]
                print "----------------------------"
                break

    print "exit code:", exit_code
    print "\nFailed!"
    sys.exit(exit_code)

def test_path(folder, is_file):
    files=['']
    if is_file == False:
        print "Testing ->", os.path.basename(folder)
        files = os.listdir(folder)
    else:
        files[0] = folder
    
    for js_file in files:
        if is_file or os.path.splitext(js_file)[1] == '.js':
            js_file = os.path.join(folder, js_file)
            js_output = ""

            if not os.path.isfile(js_file):
                print "Javascript file doesn't exist (" + js_file + ")"
                sys.exit(1)

            p = SP.Popen([ch_path, js_file], stdout=SP.PIPE, stderr=SP.STDOUT, close_fds=True)
            js_output = p.communicate()[0].replace('\r','')
            exit_code = p.wait()

            if exit_code != 0:
                show_failed(js_file, js_output, exit_code, None)
            else: #compare outputs
                baseline = os.path.splitext(js_file)[0] + '.baseline'
                baseline = os.path.join(folder, baseline)
                if os.path.isfile(baseline):
                    expected_output = None
                    with open(baseline, 'r') as bs_file:
                        expected_output = bs_file.read().replace('\r', '')
                    # todo: compare line by line and use/implement wild cards support
                    # todo: by default we discard line endings (xplat), make this optional
                    if expected_output.replace('\n', '') != js_output.replace('\n', ''):
                        show_failed(js_file, js_output, exit_code, expected_output)

            if not is_file:
                print "\tPassed ->", os.path.basename(js_file)

is_file = len(test_dirs) == 1 and os.path.splitext(test_dirs[0])[1] == '.js'

for folder in test_dirs:
    full_path = os.path.join(test_root, folder)
    if os.path.isdir(full_path) or is_file:
        test_path(full_path, is_file)

print 'Success!'

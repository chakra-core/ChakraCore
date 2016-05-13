#!/usr/bin/env python
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

import sys
import os
import subprocess as SP
import argparse
import xml.etree.ElementTree as ET

# handle command line args
parser = argparse.ArgumentParser(
    description='ChakraCore *nix Test Script',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog='''\
Samples:

test all folders:
    {0}

test only Array:
    {0} Array

test a single file:
    {0} Basics/hello.js
'''.format(sys.argv[0]))

parser.add_argument('folders', metavar='folder', nargs='*',
                    help='folder subset to run tests')
parser.add_argument('-b', '--binary', metavar='bin', help='ch full path')
parser.add_argument('-d', '--debug', action='store_true',
                    help='use debug build');
parser.add_argument('-t', '--test', action='store_true', help='use test build')
parser.add_argument('--x86', action='store_true', help='use x86 build')
parser.add_argument('--x64', action='store_true', help='use x64 build')
args = parser.parse_args()


test_root = os.path.dirname(os.path.realpath(__file__))
repo_root = os.path.dirname(test_root)

# arch: x86, x64
arch = 'x86' if args.x86 else ('x64' if args.x64 else None)
if arch == None:
    arch = os.environ.get('_BuildArch', 'x86')

# flavor: debug, test, release
type_flavor = {'chk':'debug', 'test':'test', 'fre':'release'}
flavor = 'debug' if args.debug else ('test' if args.test else None)
if flavor == None:
    flavor = type_flavor[os.environ.get('_BuildType', 'fre')]

# binary: full ch path
binary = args.binary
if binary == None:
    if sys.platform == 'win32':
        binary = 'Build/VcBuild/bin/{}_{}/ch.exe'.format(arch, flavor)
    else:
        binary = 'BuildLinux/ch'
    binary = os.path.join(repo_root, binary)
if not os.path.isfile(binary):
    print('{} not found. Did you run ./build.sh already?'.format(binary))
    sys.exit(1)


# records pass_count/fail_count
class PassFailCount(object):
    def __init__(self):
        self.pass_count = 0
        self.fail_count = 0

    def __str__(self):
        return 'passed {}, failed {}'.format(self.pass_count, self.fail_count)

# records total and individual folder's pass_count/fail_count
class TestResult(PassFailCount):
    def __init__(self):
        super(self.__class__, self).__init__()
        self.folders = {}

    def _get_folder_result(self, folder):
        r = self.folders.get(folder)
        if not r:
            r = PassFailCount()
            self.folders[folder] = r
        return r

    def log(self, filename=None, folder=None, fail=False):
        if not folder:
            folder = os.path.basename(os.path.dirname(filename))
        r = self._get_folder_result(folder)
        if fail:
            r.fail_count += 1
            self.fail_count += 1
        else:
            r.pass_count += 1
            self.pass_count += 1

test_result = TestResult()


def show_failed(filename, output, exit_code, expected_output):
    print("\nFailed -> {}".format(filename))
    if expected_output == None:
        print("\nOutput:")
        print("----------------------------")
        print(output)
        print("----------------------------")
    else:
        lst_output = output.split('\n')
        lst_expected = expected_output.split('\n')
        ln = min(len(lst_output), len(lst_expected))
        for i in range(0, ln):
            if lst_output[i] != lst_expected[i]:
                print("Output: (at line " + str(i) + ")")
                print("----------------------------")
                print(lst_output[i])
                print("----------------------------")
                print("Expected Output:")
                print("----------------------------")
                print(lst_expected[i])
                print("----------------------------")
                break

    print("exit code: {}".format(exit_code))
    test_result.log(filename, fail=True)

def test_path(path):
    if os.path.isfile(path):
        folder, file = os.path.dirname(path), os.path.basename(path)
    else:
        folder, file = path, None

    tests = load_tests(folder, file)
    if len(tests) == 0:
        return

    print("Testing -> " + os.path.basename(folder))
    for test in tests:
        test_one(folder, test)

def test_one(folder, test):
    js_file = os.path.join(folder, test['files'])
    js_output = ""

    flags = test.get('compile-flags')
    cmd = [binary, '-WERExceptionSupport', '-ExtendedErrorStackForTestHost'] \
          + (flags.split() if flags else []) \
          + [js_file]
    p = SP.Popen(cmd, stdout=SP.PIPE, stderr=SP.STDOUT)
    js_output = p.communicate()[0].replace('\r','')
    exit_code = p.wait()

    if exit_code != 0:
        return show_failed(js_file, js_output, exit_code, None)
    else: #compare outputs
        baseline = os.path.splitext(js_file)[0] + '.baseline'
        if os.path.isfile(baseline):
            expected_output = None
            with open(baseline, 'r') as bs_file:
                expected_output = bs_file.read().replace('\r', '')
            # todo: compare line by line and use/implement wild cards support
            # todo: by default we discard line endings (xplat), make this optional
            if expected_output.replace('\n', '') != js_output.replace('\n', ''):
                return show_failed(js_file, js_output, exit_code, expected_output)

    print("\tPassed -> " + os.path.basename(js_file))
    test_result.log(folder=folder)

def load_tests(folder, file):
    try:
        xmlpath = os.path.join(folder, 'rlexe.xml')
        xml = ET.parse(xmlpath).getroot()
    except IOError:
        return []

    tests = [load_test(x) for x in xml]
    if file != None:
        tests = [x for x in tests if x['files'] == file]
        if len(tests) == 0 and is_jsfile(file):
            tests = [{'files':file}]
    return tests

def load_test(testXml):
    test = dict()
    for c in testXml.find('default'):
        test[c.tag] = c.text
    return test

def is_jsfile(path):
    return os.path.splitext(path)[1] == '.js'

def main():
    # By default run all tests
    if len(args.folders) == 0:
        args.folders = [os.path.join(test_root, x)
                        for x in sorted(os.listdir(test_root))]

    for folder in args.folders:
        test_path(folder)

    print('\n')
    print('============================')
    for folder, result in sorted(test_result.folders.items()):
        print('{}: {}'.format(folder, result))
    print('============================')
    print('Total: {}'.format(test_result))
    print('Success!' if test_result.fail_count == 0 else 'Failed!')
    return 0

if __name__ == '__main__':
    sys.exit(main())

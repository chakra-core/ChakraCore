#!/usr/bin/env python
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

from __future__ import print_function
from datetime import datetime
from multiprocessing import Pool, Manager
from threading import Timer
import sys
import os
import subprocess as SP
import argparse
import xml.etree.ElementTree as ET
import re

# handle command line args
parser = argparse.ArgumentParser(
    description='ChakraCore *nix Test Script',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog='''\
Samples:

test all folders:
    runtests.py

test only Array:
    runtests.py Array

test a single file:
    runtests.py Basics/hello.js
''')

parser.add_argument('folders', metavar='folder', nargs='*',
                    help='folder subset to run tests')
parser.add_argument('-b', '--binary', metavar='bin', help='ch full path')
parser.add_argument('-d', '--debug', action='store_true',
                    help='use debug build');
parser.add_argument('-t', '--test', action='store_true', help='use test build')
parser.add_argument('--include-slow', action='store_true',
                    help='include slow tests')
parser.add_argument('--only-slow', action='store_true',
                    help='run only slow tests')
parser.add_argument('--nightly', action='store_true',
                    help='run as nightly tests')
parser.add_argument('--tag', nargs='*',
                    help='select tests with given tags')
parser.add_argument('--not-tag', nargs='*',
                    help='exclude tests with given tags')
parser.add_argument('--timeout', type=int, default=60,
                    help='test timeout (default 60 seconds)')
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

# global tags/not_tags
tags = set(args.tag or [])
not_tags = set(args.not_tag or []).union(['fail'])
if args.only_slow:
    tags.add('Slow')
elif not args.include_slow:
    not_tags.add('Slow')

not_tags.add('exclude_nightly' if args.nightly else 'nightly')

# xplat: temp hard coded to exclude tag 'require_backend'
if sys.platform != 'win32':
    not_tags.add('require_backend')

# records pass_count/fail_count
class PassFailCount(object):
    def __init__(self):
        self.pass_count = 0
        self.fail_count = 0

    def __str__(self):
        return 'passed {}, failed {}'.format(self.pass_count, self.fail_count)

    def total_count(self):
        return self.pass_count + self.fail_count

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

    def log(self, filename, fail=False):
        folder = os.path.basename(os.path.dirname(filename))
        r = self._get_folder_result(folder)
        if fail:
            r.fail_count += 1
            self.fail_count += 1
        else:
            r.pass_count += 1
            self.pass_count += 1

# test variants:
#   interpreted: -maxInterpretCount:1 -maxSimpleJitRunCount:1 -bgjit-
#   dynapogo: -forceNative -off:simpleJit -bgJitDelay:0
class TestVariant(object):
    MSG_PRINT = 0
    MSG_TEST_RESULT = 1
    _empty_set = set()

    def __init__(self, name, compile_flags=[]):
        self.name = name
        self.compile_flags = \
            ['-WERExceptionSupport', '-ExtendedErrorStackForTestHost'] + compile_flags
        self.tags = tags.copy()
        self.not_tags = not_tags.union(
            ['{}_{}'.format(x, name) for x in 'fails','excludes'])

        self.msg_queue = Manager().Queue() # messages from multi processes
        self.test_result = TestResult()

    # check if this test variant should run a given test
    def _should_test(self, test):
        tags = test.get('tags')
        tags = set(tags.split(',')) if tags else self._empty_set
        if not tags.isdisjoint(self.not_tags):
            return False
        if self.tags and not self.tags.issubset(tags):
            return False
        return True

    # queue a test result from multi-process runs
    def _log_result(self, filename, fail=False):
        self.msg_queue.put((self.MSG_TEST_RESULT, filename, fail))

    # queue output from multiprocessing runs
    def _print(self, line):
        self.msg_queue.put((self.MSG_PRINT, line))

    # (on main process) process one queued message
    def _process_msg(self, msg):
        if msg[0] == self.MSG_TEST_RESULT:
            tmp,filename,fail = msg
            self.test_result.log(filename, fail=fail)
        elif msg[0] == self.MSG_PRINT:
            print(msg[1])
        else:
            print('ERROR: invalid msg! {}'.format(msg))
            sys.exit(-1)

    # (on main process) wait and process one queued message
    def _process_one_msg(self):
        self._process_msg(self.msg_queue.get())

    # log a failed test with details
    def _show_failed(self, flags, filename,
                    output, exit_code, elapsed_time,
                    expected_output=None, timedout=False):
        self._print('[{}] Failed -> {}'.format(elapsed_time, filename))
        if timedout:
            self._print('ERROR: Test timed out!')
        self._print('{} {} {}'.format(binary, ' '.join(flags), filename));
        if expected_output == None or timedout:
            self._print("\nOutput:")
            self._print("----------------------------")
            self._print(output)
            self._print("----------------------------")
        else:
            lst_output = output.split('\n')
            lst_expected = expected_output.split('\n')
            ln = min(len(lst_output), len(lst_expected))
            for i in range(0, ln):
                if lst_output[i] != lst_expected[i]:
                    self._print("Output: (at line " + str(i) + ")")
                    self._print("----------------------------")
                    self._print(lst_output[i])
                    self._print("----------------------------")
                    self._print("Expected Output:")
                    self._print("----------------------------")
                    self._print(lst_expected[i])
                    self._print("----------------------------")
                    break

        self._print("exit code: {}".format(exit_code))
        self._log_result(filename, fail=True)

    # temp: try find real file name on hard drive if case mismatch
    def _check_file(self, folder, filename):
        path = os.path.join(folder, filename)
        if os.path.isfile(path):
            return path     # file exists on disk

        filename_lower = filename.lower()
        files = os.listdir(folder)
        for i in range(len(files)):
            if files[i].lower() == filename_lower:
                self._print('\nWARNING: {} should be {}\n'.format(
                    path, files[i]))
                return os.path.join(folder, files[i])

        # cann't find the file, just return the path and let it error out
        return path

    # run one test under this variant
    def test_one(self, folder, test):
        js_file = self._check_file(folder, test['files'])
        js_output = ""

        working_path = os.path.dirname(js_file)
        file_name = os.path.basename(js_file)

        flags = test.get('compile-flags')
        flags = self.compile_flags + (flags.split() if flags else [])
        cmd = [binary] + flags + [file_name]

        proc = SP.Popen(cmd, stdout=SP.PIPE, stderr=SP.STDOUT, cwd=working_path)
        timeout_data = [proc, False]
        def timeout_func(timeout_data):
            timeout_data[0].kill()
            timeout_data[1] = True
        timeout = args.timeout # or specific test timeout override
        timer = Timer(timeout, timeout_func, [timeout_data])
        start_time = datetime.now()
        try:
            timer.start()
            js_output = proc.communicate()[0].replace('\r','')
            exit_code = proc.wait()
        finally:
            timer.cancel()
        elapsed_time = (datetime.now() - start_time).total_seconds()

        # shared _show_failed args
        fail_args = { 'flags': flags, 'filename': js_file,
                    'output': js_output, 'exit_code': exit_code,
                    'elapsed_time': elapsed_time };

        # check timed out
        if (timeout_data[1]):
            return self._show_failed(timedout=True, **fail_args)

        # check ch failed
        if exit_code != 0:
            return self._show_failed(**fail_args)

        # check output
        if 'baseline' not in test:
            # output lines must be 'pass' or 'passed' or empty
            lines = (line.lower() for line in js_output.split('\n'))
            if any(line != '' and line != 'pass' and line != 'passed'
                    for line in lines):
                return self._show_failed(**fail_args)
        else:
            baseline = test.get('baseline')
            if baseline:
                # perform baseline comparison
                baseline = self._check_file(working_path, baseline)
                expected_output = None
                with open(baseline, 'r') as bs_file:
                    baseline_output = bs_file.read()

                # Cleanup carriage return
                # todo: remove carriage return at the end of the line
                #       or better fix ch to output same on all platforms
                expected_output = re.sub('[\r]+\n', '\n', baseline_output)

                # todo: implement wild cards support
                if expected_output != js_output:
                    return self._show_failed(
                        expected_output=expected_output, **fail_args)

        # passed
        self._print('[{}] Passed -> {}'.format(elapsed_time, js_file))
        self._log_result(js_file)

    # run tests under this variant, using given multiprocessing Pool
    def run(self, tests, pool):
        print('\n############# Starting {} variant #############'.format(
            self.name))
        if self.tags:
            print('  tags: {}'.format(self.tags))
        for x in self.not_tags:
            print('  exclude: {}'.format(x))
        print()

        # filter tests to run
        tests = [x for x in tests if self._should_test(x[1])]

        # run tests in parallel
        result = pool.map_async(run_one,
                    [(self,folder,test) for folder,test in tests])
        while self.test_result.total_count() != len(tests):
            self._process_one_msg()

    # print test result summary
    def print_summary(self):
        print('\n######## Logs for {} variant ########'.format(self.name))
        for folder, result in sorted(self.test_result.folders.items()):
            print('{}: {}'.format(folder, result))
        print("----------------------------")
        print('Total: {}'.format(self.test_result))

# global run one test function for multiprocessing, used by TestVariant
def run_one(data):
    variant, folder, test = data
    variant.test_one(folder, test)


# record folder/tags info from test_root/rlexedirs.xml
class FolderTags(object):
    _empty_set = set()

    def __init__(self):
        xmlpath = os.path.join(test_root, 'rlexedirs.xml')
        try:
            xml = ET.parse(xmlpath).getroot()
        except IOError:
            print('ERROR: failed to read {}'.format(xmlpath))
            exit(-1)

        self._folder_tags = {}
        for x in xml:
            d = x.find('default')
            key = d.find('files').text.lower() # avoid case mismatch
            tags = d.find('tags')
            self._folder_tags[key] = \
                set(tags.text.split(',')) if tags != None else self._empty_set

    # check if should test a given folder
    def should_test(self, folder):
        key = os.path.basename(os.path.normpath(folder)).lower()
        ftags = self._folder_tags.get(key)

        # folder listed in rlexedirs.xml and not exlucded by global not_tags
        return ftags != None and ftags.isdisjoint(not_tags)


# load all tests in folder using rlexe.xml file
def load_tests(folder, file):
    try:
        xmlpath = os.path.join(folder, 'rlexe.xml')
        xml = ET.parse(xmlpath).getroot()
    except IOError:
        return []

    def load_test(testXml):
        test = dict()
        for c in testXml.find('default'):
            test[c.tag] = c.text
        return test

    tests = [load_test(x) for x in xml]
    if file != None:
        tests = [x for x in tests if x.get('files') == file]
        if len(tests) == 0 and is_jsfile(file):
            tests = [{'files':file, 'baseline':''}]
    return tests

def is_jsfile(path):
    return os.path.splitext(path)[1] == '.js'

def main():
    # By default run all tests
    if len(args.folders) == 0:
        files = (os.path.join(test_root, x) for x in os.listdir(test_root))
        args.folders = [f for f in sorted(files) if not os.path.isfile(f)]

    # load all tests
    tests = []
    folder_tags = FolderTags()
    for path in args.folders:
        if os.path.isfile(path):
            folder, file = os.path.dirname(path), os.path.basename(path)
        else:
            folder, file = path, None
        if folder_tags.should_test(folder):
            tests += ((folder,test) for test in load_tests(folder, file))

    # test variants
    variants = [
        TestVariant('interpreted', [
            '-maxInterpretCount:1', '-maxSimpleJitRunCount:1', '-bgjit-'])
    ]

    # run each variant
    pool = Pool() # Use a multiprocessing process Pool
    start_time = datetime.now()
    for variant in variants:
        variant.run(tests, pool)
    elapsed_time = datetime.now() - start_time

    # print summary
    for variant in variants:
        variant.print_summary()

    print()
    failed = any(variant.test_result.fail_count > 0 for variant in variants)
    print('[{}] {}'.format(
        str(elapsed_time), 'Success!' if not failed else 'Failed!'))
    return 0

if __name__ == '__main__':
    sys.exit(main())

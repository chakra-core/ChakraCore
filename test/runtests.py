#!/usr/bin/env python
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Copyright (c) ChakraCore Project Contributors. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

from __future__ import print_function
from datetime import datetime
from multiprocessing import Pool, Manager, cpu_count
from threading import Timer
import sys
import os
import glob
import subprocess as SP
import traceback
import argparse
import xml.etree.ElementTree as ET
import re
import time

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

DEFAULT_TIMEOUT = 60
SLOW_TIMEOUT = 180

parser.add_argument('folders', metavar='folder', nargs='*',
                    help='folder subset to run tests')
parser.add_argument('-b', '--binary', metavar='bin',
                    help='ch full path')
parser.add_argument('-v', '--verbose', action='store_true',
                    help='increase verbosity of output')
parser.add_argument('--sanitize', metavar='sanitizers',
                    help='ignore tests known to be broken with these sanitizers')
parser.add_argument('-d', '--debug', action='store_true',
                    help='use debug build')
parser.add_argument('-t', '--test', '--test-build', action='store_true',
                    help='use test build')
parser.add_argument('--variants', metavar='variant', nargs='+',
                    help='run specified test variants')
parser.add_argument('--include-slow', action='store_true',
                    help='include slow tests (timeout ' + str(SLOW_TIMEOUT) + ' seconds)')
parser.add_argument('--only-slow', action='store_true',
                    help='run only slow tests')
parser.add_argument('--tag', nargs='*',
                    help='select tests with given tags')
parser.add_argument('--not-tag', action='append',
                    help='exclude tests with given tags')
parser.add_argument('--flags', default='',
                    help='global test flags to ch')
parser.add_argument('--timeout', type=int, default=DEFAULT_TIMEOUT,
                    help='test timeout (default ' + str(DEFAULT_TIMEOUT) + ' seconds)')
parser.add_argument('--swb', action='store_true',
                    help='use binary from VcBuild.SWB to run the test')
parser.add_argument('--lldb', default=None,
                    help='run test suit with lldb batch mode to get call stack for crashing processes (ignores baseline matching)', action='store_true')
parser.add_argument('--x86', action='store_true',
                    help='use x86 build')
parser.add_argument('--x64', action='store_true',
                    help='use x64 build')
parser.add_argument('--arm', action='store_true',
                    help='use arm build')
parser.add_argument('--arm64', action='store_true',
                    help='use arm64 build')
parser.add_argument('-j', '--processcount', metavar='processcount', type=int,
                    help='number of parallel threads to use')
parser.add_argument('--warn-on-timeout', action='store_true',
                    help='warn when a test times out instead of labelling it as an error immediately')
parser.add_argument('--override-test-root', type=str,
                    help='change the base directory for the tests (where rlexedirs will be sought)')
parser.add_argument('--extra-flags', type=str,
                    help='add extra flags to all executed tests')
parser.add_argument('--orc','--only-return-code', action='store_true',
                    help='only consider test return 0/non-0 for pass-fail (no baseline checks)')
parser.add_argument('--show-passes', action='store_true',
                    help='display passed tests, when false only failures and the result summary are shown')
args = parser.parse_args()

test_root = os.path.dirname(os.path.realpath(__file__))
repo_root = os.path.dirname(test_root)

# new test root
if args.override_test_root:
    test_root = os.path.realpath(args.override_test_root)

# arch: x86, x64, arm, arm64
arch = None
if args.x86:
    arch = 'x86'
elif args.x64:
    arch = 'x64'
elif args.arm:
    arch = 'arm'
elif args.arm64:
    arch = 'arm64'

if arch == None:
    arch = os.environ.get('_BuildArch', 'x86')
if sys.platform != 'win32':
    arch = 'x64'    # xplat: hard code arch == x64
arch_alias = 'amd64' if arch == 'x64' else None

# flavor: debug, test, release
flavor = 'Debug' if args.debug else ('Test' if args.test else None)
if flavor == None:
    print("ERROR: Test build target wasn't defined.")
    print("Try '-t' (test build) or '-d' (debug build).")
    sys.exit(1)

# handling for extra flags
extra_flags = ['-WERExceptionSupport']
if args.extra_flags:
    extra_flags = args.extra_flags.split()

# test variants
if not args.variants:
    args.variants = ['interpreted', 'dynapogo']

# append exe to the binary name on windows
binary_name = "ch"
if sys.platform == 'win32':
    binary_name = "ch.exe"

# binary: full ch path
binary = args.binary
if binary == None:
    if sys.platform == 'win32':
        build = "VcBuild.SWB" if args.swb else "VcBuild"
        binary = os.path.join(repo_root, 'Build', build, 'bin', '{}_{}'.format(arch, flavor), binary_name)
    else:
        binary = os.path.join(repo_root, 'out', flavor, binary_name)

if not os.path.isfile(binary):
    print('{} not found. Aborting.'.format(binary))
    sys.exit(1)

# global tags/not_tags
tags = set(args.tag or [])
not_tags = set(args.not_tag or []).union(['fail', 'exclude_' + arch, 'exclude_' + flavor])

if arch_alias:
    not_tags.add('exclude_' + arch_alias)
if args.only_slow:
    tags.add('Slow')
elif not args.include_slow:
    not_tags.add('Slow')
elif args.include_slow and args.timeout == DEFAULT_TIMEOUT:
    args.timeout = SLOW_TIMEOUT

# verbosity
verbose = False
show_passes = False
if args.verbose:
    verbose = True
    show_passes = True
    print("Emitting verbose output...")
elif args.show_passes:
    show_passes = True

# xplat: temp hard coded to exclude unsupported tests
if sys.platform != 'win32':
    not_tags.add('exclude_xplat')
    not_tags.add('require_winglob')
    not_tags.add('require_simd')
else:
    not_tags.add('exclude_windows')

# exclude tests that depend on features not supported on a platform
if arch == 'arm' or arch == 'arm64':
    not_tags.add('require_asmjs')

# exclude tests known to fail under certain sanitizers
if args.sanitize != None:
    not_tags.add('exclude_sanitize_'+args.sanitize)

if sys.platform == 'darwin':
    not_tags.add('exclude_mac')

if 'require_icu' in not_tags or 'exclude_noicu' in not_tags:
    not_tags.add('Intl')

not_compile_flags = None

# use -j flag to specify number of parallel processes
processcount = cpu_count()
if args.processcount != None:
    processcount = int(args.processcount)

# handle warn on timeout
warn_on_timeout = False
if args.warn_on_timeout == True:
    warn_on_timeout = True

# handle limiting test result analysis to return codes
return_code_only = False
if args.orc == True:
    return_code_only = True

# use tags/not_tags/not_compile_flags as case-insensitive
def lower_set(s):
    return set([x.lower() for x in s] if s else [])

tags = lower_set(tags)
not_tags = lower_set(not_tags)
not_compile_flags = lower_set(not_compile_flags)

# split tags text into tags set
_empty_set = set()
def split_tags(text):
    return set(x.strip() for x in text.lower().split(',')) if text \
            else _empty_set

# remove carriage returns at end of line to avoid platform difference
def normalize_new_line(text):
    return re.sub(b'[\r]+\n', b'\n', text)

# A test simply contains a collection of test attributes.
# Misc attributes added by test run:
#   id              unique counter to identify a test
#   filename        full path of test file
#   elapsed_time    elapsed time when running the test
#
class Test(dict):
    __setattr__ = dict.__setitem__
    __delattr__ = dict.__delitem__

    # support dot syntax for normal attribute access
    def __getattr__(self, key):
        return super(Test, self).__getattr__(key) if key.startswith('__') \
                else self.get(key)

    # mark start of this test run, to compute elapsed_time
    def start(self):
        self.start_time = datetime.now()

    # mark end of this test run, compute elapsed_time
    def done(self):
        if not self.elapsed_time:
            self.elapsed_time = (datetime.now() - self.start_time)\
                                .total_seconds()

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
    def __init__(self, name, compile_flags=[], variant_not_tags=[]):
        self.name = name
        self.compile_flags = \
            ['-ExtendedErrorStackForTestHost',
             '-BaselineMode'] + compile_flags
        self._compile_flags_has_expansion = self._has_expansion(compile_flags)
        self.tags = tags.copy()
        self.not_tags = not_tags.union(variant_not_tags).union(
            ['{}_{}'.format(x, name) for x in ('fails','exclude')])

        self.msg_queue = Manager().Queue() # messages from multi processes
        self.test_result = TestResult()
        self.test_count = 0
        self._print_lines = [] # _print lines buffer
        self._last_len = 0
        if verbose:
            print("Added variant {0}:".format(name))
            print("Flags: " + ", ".join(self.compile_flags))
            print("Tags: " + ", ".join(self.tags))
            print("NotTags: " + ", ".join(self.not_tags))

    @staticmethod
    def _has_expansion(flags):
        return any(re.match(r'.*\${.*}', f) for f in flags)

    @staticmethod
    def _expand(flag, test):
        return re.sub(r'\${id}', str(test.id), flag)

    def _expand_compile_flags(self, test):
        if self._compile_flags_has_expansion:
            return [self._expand(flag, test) for flag in self.compile_flags]
        return self.compile_flags

    # check if this test variant should run a given test
    def _should_test(self, test):
        tags = split_tags(test.get('tags'))
        if not tags.isdisjoint(self.not_tags):
            return False
        if self.tags and not self.tags.issubset(tags):
            return False
        if not_compile_flags: # exclude unsupported compile-flags if any
            flags = test.get('compile-flags')
            if flags and \
                    not not_compile_flags.isdisjoint(flags.lower().split()):
                return False
        return True

    # print output from multi-process run, to be sent with result message
    def _print(self, line):
        self._print_lines.append(line)

    # queue a test result from multi-process runs
    def _log_result(self, test, fail):
        if fail or show_passes:
            output = ''
            for line in self._print_lines:
                output = output + line + '\n' # collect buffered _print output
        else:
            output = ''
        self._print_lines = []
        self.msg_queue.put((test.filename, fail, test.elapsed_time, output))

    # (on main process) process one queued message
    def _process_msg(self, msg):
        filename, fail, elapsed_time, output = msg
        self.test_result.log(filename, fail=fail)
        if fail or show_passes:
            line = '[{}/{} {:4.2f}] {} -> {}'.format(
                self.test_result.total_count(),
                self.test_count,
                elapsed_time,
                'Failed' if fail else 'Passed',
                self._short_name(filename))
            padding = self._last_len - len(line)
            print(line + ' ' * padding, end='\n' if fail else '\r')
            self._last_len = len(line) if not fail else 0
            if len(output) > 0:
                print(output)

    # get a shorter test file path for display only
    def _short_name(self, filename):
        folder = os.path.basename(os.path.dirname(filename))
        return os.path.join(folder, os.path.basename(filename))

    # (on main process) wait and process one queued message
    def _process_one_msg(self):
        self._process_msg(self.msg_queue.get())

    # log a failed test with details
    def _show_failed(self, test, flags, exit_code, output,
                    expected_output=None, timedout=False):
        if timedout:
            if warn_on_timeout:
                self._print('WARNING: Test timed out!')
            else:
                self._print('ERROR: Test timed out!')
        self._print('{} {} {}'.format(binary, ' '.join(flags), test.filename))

        if not return_code_only:
            if expected_output == None or timedout:
                self._print("\nOutput:")
                self._print("----------------------------")
                self._print(output.decode('utf-8'))
                self._print("----------------------------")
            else:
                lst_output = output.split(b'\n')
                lst_expected = expected_output.split(b'\n')
                ln = min(len(lst_output), len(lst_expected))
                for i in range(0, ln):
                    if lst_output[i] != lst_expected[i]:
                        self._print("Output: (at line " + str(i+1) + ")")
                        self._print("----------------------------")
                        self._print(lst_output[i].decode('utf-8'))
                        self._print("----------------------------")
                        self._print("Expected Output:")
                        self._print("----------------------------")
                        self._print(lst_expected[i].decode('utf-8'))
                        self._print("----------------------------")
                        break

        self._print("exit code: {}".format(exit_code))
        if warn_on_timeout and timedout:
            self._log_result(test, fail=False)
        else:
            self._log_result(test, fail=True)

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

        # can't find the file, just return the path and let it error out
        return path

    # run one test under this variant
    def test_one(self, test):
        try:
            test.start()
            self._run_one_test(test)
        except Exception:
            test.done()
            self._print(traceback.format_exc())
            self._log_result(test, fail=True)

    # internally perform one test run
    def _run_one_test(self, test):
        folder = test.folder
        js_file = test.filename = self._check_file(folder, test.files)
        js_output = b''

        working_path = os.path.dirname(js_file)

        flags = test.get('compile-flags') or ''
        flags = self._expand_compile_flags(test) + \
                    args.flags.split() + \
                    flags.split()

        if test.get('custom-config-file') != None:
            flags = ['-CustomConfigFile:' + test.get('custom-config-file')]

        if args.lldb == None:
            cmd = [binary] + flags + [os.path.basename(js_file)]
        else:
            lldb_file = open(working_path + '/' + os.path.basename(js_file) + '.lldb.cmd', 'w')
            lldb_command = ['run'] + flags + [os.path.basename(js_file)]
            lldb_file.write(' '.join([str(s) for s in lldb_command]));
            lldb_file.close()
            cmd = ['lldb'] + [binary] + ['-s'] + [os.path.basename(js_file) + '.lldb.cmd'] + ['-o bt'] + ['-b']

        test.start()
        proc = SP.Popen(cmd, stdout=SP.PIPE, stderr=SP.STDOUT, cwd=working_path)
        timeout_data = [proc, False]
        def timeout_func(timeout_data):
            timeout_data[0].kill()
            timeout_data[1] = True
        timeout = test.get('timeout', args.timeout) # test override or default
        timer = Timer(timeout, timeout_func, [timeout_data])
        skip_baseline_match = False
        try:
            timer.start()
            js_output = normalize_new_line(proc.communicate()[0])
            exit_code = proc.wait()
            # if -lldb was set; check if test was crashed before corrupting the output
            search_for = " exited with status = 0 (0x00000000)"
            if args.lldb != None and exit_code == 0 and js_output.index(search_for) > 0:
                js_output = js_output[0:js_output.index(search_for)]
                exit_pos = js_output.rfind('\nProcess ')
                if exit_pos > len(js_output) - 20: # if [Process ????? <seach for>]
                    if 'baseline' not in test:
                        js_output = "pass"
                    else:
                        skip_baseline_match = True
        finally:
            timer.cancel()
        test.done()

        # shared _show_failed args
        fail_args = { 'test': test, 'flags': flags,
                      'exit_code': exit_code, 'output': js_output }

        # check timed out
        if (timeout_data[1]):
            return self._show_failed(timedout=True, **fail_args)

        # check ch failed
        if exit_code != 0:
            return self._show_failed(**fail_args)

        if not return_code_only:
            # check output
            if 'baseline' not in test:
                # output lines must be 'pass' or 'passed' or empty with at least 1 not empty
                passes = 0
                for line in js_output.split(b'\n'):
                    if line !=b'':
                        low = line.lower()
                        if low == b'pass' or low == b'passed':
                            passes = 1
                        else:
                            return self._show_failed(**fail_args)
                if passes == 0:
                    return self._show_failed(**fail_args)
            else:
                baseline = test.get('baseline')
                if not skip_baseline_match and baseline:
                    # perform baseline comparison
                    baseline = self._check_file(folder, baseline)
                    with open(baseline, 'rb') as bs_file:
                        baseline_output = bs_file.read()

                    # Clean up carriage return
                    # todo: remove carriage return at the end of the line
                    #       or better fix ch to output same on all platforms
                    expected_output = normalize_new_line(baseline_output)

                    if expected_output != js_output:
                        return self._show_failed(
                            expected_output=expected_output, **fail_args)

        # passed
        if verbose:
            self._print('{} {} {}'.format(binary, ' '.join(flags), test.filename))
        self._log_result(test, fail=False)

    # run tests under this variant, using given multiprocessing Pool
    def _run(self, tests, pool):
        # filter tests to run
        tests = [x for x in tests if self._should_test(x)]
        self.test_count += len(tests)

        # run tests in parallel
        pool.map_async(run_one, [(self,test) for test in tests])
        while self.test_result.total_count() != self.test_count:
            self._process_one_msg()

    # print test result summary
    def print_summary(self, time):
        print('\n############ Results for {} tests ###########'\
                        .format(self.name))
        for folder, result in sorted(self.test_result.folders.items()):
            print('{}: {}'.format(folder, result))
        print("----------------------------")
        print('Total: {}'.format(self.test_result))
        print('Time taken for {} tests: {} seconds\n'.format(self.name, round(time.total_seconds(),2)))
        sys.stdout.flush()

    # run all tests from testLoader
    def run(self, testLoader, pool, sequential_pool):
        print('\n############# Starting {} tests #############'\
                        .format(self.name))
        if self.tags:
            print('  tags: {}'.format(self.tags))
        for x in self.not_tags:
            print('  exclude: {}'.format(x))
        sys.stdout.flush()

        start_time = datetime.now()        
        tests, sequential_tests = [], []
        for folder in testLoader.folders():
            if folder.tags.isdisjoint(self.not_tags):
                dest = tests if not folder.is_sequential else sequential_tests
                dest += folder.tests
        if tests:
            self._run(tests, pool)
        if sequential_tests:
            self._run(sequential_tests, sequential_pool)

        self.print_summary(datetime.now() - start_time)

# global run one test function for multiprocessing, used by TestVariant
def run_one(data):
    try:
        variant, test = data
        variant.test_one(test)
    except Exception:
        print('ERROR: Unhandled exception!!!')
        traceback.print_exc()


# A test folder contains a list of tests and maybe some tags.
class TestFolder(object):
    def __init__(self, tests, tags=_empty_set):
        self.tests = tests
        self.tags = tags
        self.is_sequential = 'sequential' in tags

# TestLoader loads all tests
class TestLoader(object):
    def __init__(self, paths):
        self._folder_tags = self._load_folder_tags()
        self._test_id = 0
        self._folders = []

        for path in paths:
            if os.path.isfile(path):
                folder, file = os.path.dirname(path), os.path.basename(path)
            else:
                folder, file = path, None

            ftags = self._get_folder_tags(folder)
            if ftags != None:  # Only honor entries listed in rlexedirs.xml
                tests = self._load_tests(folder, file)
                self._folders.append(TestFolder(tests, ftags))

    def folders(self):
        return self._folders

    # load folder/tags info from test_root/rlexedirs.xml
    @staticmethod
    def _load_folder_tags():
        xmlpath = os.path.join(test_root, 'rlexedirs.xml')
        try:
            xml = ET.parse(xmlpath).getroot()
        except IOError:
            print('ERROR: failed to read {}'.format(xmlpath))
            exit(-1)

        folder_tags = {}
        for x in xml:
            d = x.find('default')
            key = d.find('files').text.lower() # avoid case mismatch
            tags = d.find('tags')
            folder_tags[key] = \
                split_tags(tags.text) if tags != None else _empty_set
        return folder_tags

    # get folder tags if any
    def _get_folder_tags(self, folder):
        key = os.path.basename(os.path.normpath(folder)).lower()
        return self._folder_tags.get(key)

    def _next_test_id(self):
        self._test_id += 1
        return self._test_id

    # load all tests in folder using rlexe.xml file
    def _load_tests(self, folder, file):
        try:
            xmlpath = os.path.join(folder, 'rlexe.xml')
            xml = ET.parse(xmlpath).getroot()
        except IOError:
            return []

        def test_override(condition, check_tag, check_value, test):
            target = condition.find(check_tag)
            if target != None and target.text == check_value:
                for override in condition.find('override'):
                    test[override.tag] = override.text

        def load_test(testXml):
            test = Test(folder=folder)
            for c in testXml.find('default'):
                if c.tag == 'timeout':                       # timeout seconds
                    test[c.tag] = int(c.text)
                elif c.tag == 'tags' and c.tag in test:      # merge multiple <tags>
                    test[c.tag] = test[c.tag] + ',' + c.text
                else:
                    test[c.tag] = c.text

            condition = testXml.find('condition')
            if condition != None:
                test_override(condition, 'target', arch_alias, test)

            return test

        tests = [load_test(x) for x in xml]
        if file != None:
            tests = [x for x in tests if x.files == file]
            if len(tests) == 0 and self.is_jsfile(file):
                tests = [Test(folder=folder, files=file, baseline='')]

        for test in tests:  # assign unique test.id
            test.id = self._next_test_id()

        return tests

    @staticmethod
    def is_jsfile(path):
        return os.path.splitext(path)[1] == '.js'

def main():
    # Set the right timezone, the tests need Pacific Standard Time
    # TODO: Windows. time.tzset only supports Unix
    if hasattr(time, 'tzset'):
        os.environ['TZ'] = 'US/Pacific'
        time.tzset()
    elif sys.platform == 'win32':
        os.system('tzutil /s "Pacific Standard time"')

    # By default run all tests
    if len(args.folders) == 0:
        files = (os.path.join(test_root, x) for x in os.listdir(test_root))
        args.folders = [f for f in sorted(files) if not os.path.isfile(f)]

    # load all tests
    testLoader = TestLoader(args.folders)

    # test variants
    variants = [x for x in [
        TestVariant('interpreted', extra_flags + [
                '-maxInterpretCount:1', '-maxSimpleJitRunCount:1', '-bgjit-',
                '-dynamicprofilecache:profile.dpl.${id}'
            ], [
                'require_disable_jit'
            ]),
        TestVariant('dynapogo', extra_flags + [
                '-forceNative', '-off:simpleJit', '-bgJitDelay:0',
                '-dynamicprofileinput:profile.dpl.${id}'
            ], [
                'require_disable_jit'
            ]),
        TestVariant('disable_jit', extra_flags + [
                '-nonative'
            ], [
                'exclude_interpreted', 'fails_interpreted', 'require_backend'
            ])
    ] if x.name in args.variants]

    print('############# ChakraCore Test Suite #############')
    print('Testing {} build'.format('Test' if flavor == 'Test' else 'Debug'))
    print('Using {} threads'.format(processcount))
    # run each variant
    pool, sequential_pool = Pool(processcount), Pool(1)
    start_time = datetime.now()
    for variant in variants:
        variant.run(testLoader, pool, sequential_pool)
    elapsed_time = datetime.now() - start_time

    failed = any(variant.test_result.fail_count > 0 for variant in variants)
    print('[{} seconds] {}'.format(
        round(elapsed_time.total_seconds(),2), 'Success!' if not failed else 'Failed!'))

    # rm profile.dpl.*
    for f in glob.glob(test_root + '/*/profile.dpl.*'):
        os.remove(f)

    return 1 if failed else 0

if __name__ == '__main__':
    sys.exit(main())

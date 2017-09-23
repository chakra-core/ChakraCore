#!/usr/bin/env python3

import argparse
import sys
import os
import glob
import subprocess
import shutil
import multiprocessing as mp

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
INTERPRETER_DIR = os.path.join(SCRIPT_DIR, '..', 'interpreter')
WASM_EXEC = os.path.join(INTERPRETER_DIR, 'wasm')

WAST_TESTS_DIR = os.path.join(SCRIPT_DIR, 'core')
JS_TESTS_DIR = os.path.join(SCRIPT_DIR, 'js-api')
HTML_TESTS_DIR = os.path.join(SCRIPT_DIR, 'html')
HARNESS_DIR = os.path.join(SCRIPT_DIR, 'harness')

HARNESS_FILES = ['testharness.js', 'testharnessreport.js', 'testharness.css']
WPT_URL_PREFIX = '/resources'

# Helpers.
def run(*cmd):
    return subprocess.run(cmd,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT,
                          universal_newlines=True)

# Preconditions.
def ensure_remove_dir(path):
    if os.path.exists(path):
        shutil.rmtree(path)

def ensure_empty_dir(path):
    ensure_remove_dir(path)
    os.mkdir(path)

def compile_wasm_interpreter():
    print("Recompiling the wasm interpreter...")
    result = run('make', '-C', INTERPRETER_DIR)
    if result.returncode != 0:
        print("Couldn't recompile wasm spec interpreter")
        sys.exit(1)
    print("Done!")

def ensure_wasm_executable(path_to_wasm):
    """
    Ensure we have built the wasm spec interpreter.
    """
    result = run(path_to_wasm, '-v', '-e', '')
    if result.returncode != 0:
        print('Unable to run the wasm executable')
        sys.exit(1)

# JS harness.
def convert_one_wast_file(inputs):
    wast_file, js_file = inputs
    print('Compiling {} to JS...'.format(wast_file))
    return run(WASM_EXEC, wast_file, '-h', '-o', js_file)

def convert_wast_to_js(out_js_dir):
    """Compile all the wast files to JS and store the results in the JS dir."""

    inputs = []

    for wast_file in glob.glob(os.path.join(WAST_TESTS_DIR, '*.wast')):
        # Don't try to compile tests that are supposed to fail.
        if '.fail.' in wast_file:
            continue

        js_filename = os.path.basename(wast_file) + '.js'
        js_file = os.path.join(out_js_dir, js_filename)
        inputs.append((wast_file, js_file))

    pool = mp.Pool(processes=8)
    for result in pool.imap_unordered(convert_one_wast_file, inputs):
        if result.returncode != 0:
            print('Error when compiling {} to JS: {}', wast_file, result.stdout)

def build_js(out_js_dir, include_harness=False):
    print('Building JS...')
    convert_wast_to_js(out_js_dir)

    print('Copying JS tests to the JS out dir...')
    for js_file in glob.glob(os.path.join(JS_TESTS_DIR, '*.js')):
        shutil.copy(js_file, out_js_dir)

    harness_dir = os.path.join(out_js_dir, 'harness')
    ensure_empty_dir(harness_dir)

    print('Copying JS test harness to the JS out dir...')
    for js_file in glob.glob(os.path.join(HARNESS_DIR, '*')):
        if os.path.basename(js_file) in HARNESS_FILES and not include_harness:
            continue
        shutil.copy(js_file, harness_dir)

    print('Done building JS.')

# HTML harness.
HTML_HEADER = """<!doctype html>
<html>
    <head>
        <meta charset="UTF-8">
        <title>WebAssembly Web Platform Test</title>
    </head>
    <body>

        <script src={WPT_PREFIX}/testharness.js></script>
        <script src={WPT_PREFIX}/testharnessreport.js></script>
        <script src={PREFIX}/index.js></script>
        <script src={PREFIX}/wasm-constants.js></script>
        <script src={PREFIX}/wasm-module-builder.js></script>

        <div id=log></div>
"""

HTML_BOTTOM = """
    </body>
</html>
"""

def build_html_js(out_dir, js_dir, include_harness=False):
    if js_dir is None:
        ensure_empty_dir(out_dir)
        build_js(out_dir, include_harness)
    else:
        print('Copying JS files into the HTML dir...')
        ensure_remove_dir(out_dir)
        def ignore(_src, names):
            if include_harness:
                return []
            return [name for name in names if os.path.basename(name) in HARNESS_FILES]
        shutil.copytree(js_dir, out_dir, ignore=ignore)
        print('Done copying JS files into the HTML dir.')

    for js_file in glob.glob(os.path.join(HTML_TESTS_DIR, '*.js')):
        shutil.copy(js_file, out_dir)

def build_html_from_js(js_html_dir, html_dir):
    for js_file in glob.glob(os.path.join(js_html_dir, '*.js')):
        js_filename = os.path.basename(js_file)
        html_filename = js_filename + '.html'
        html_file = os.path.join(html_dir, html_filename)
        with open(html_file, 'w+') as f:
            content = HTML_HEADER.replace('{PREFIX}', './js/harness') \
                                 .replace('{WPT_PREFIX}', WPT_URL_PREFIX)
            content += "        <script src=./js/{SCRIPT}></script>".replace('{SCRIPT}', js_filename)
            content += HTML_BOTTOM
            f.write(content)

def build_html(html_dir, js_dir):
    print("Building HTML tests...")

    js_html_dir = os.path.join(html_dir, 'js')

    build_html_js(js_html_dir, js_dir)

    print('Building WPT tests from JS tests...')
    build_html_from_js(js_html_dir, html_dir)

    print("Done building HTML tests.")


# Front page harness.
def wrap_single_test(js_file):
    test_func_name = os.path.basename(js_file).replace('.', '_').replace('-', '_')

    content = ["(function {}() {{".format(test_func_name)]
    with open(js_file, 'r') as f:
        content += f.readlines()
    content.append('reinitializeRegistry();')
    content.append('})();')

    with open(js_file, 'w') as f:
        f.write('\n'.join(content))

def build_front_page(out_dir, js_dir):
    print('Building front page containing all the HTML tests...')

    js_out_dir = os.path.join(out_dir, 'js')

    build_html_js(js_out_dir, js_dir, include_harness=True)
    for js_file in glob.glob(os.path.join(js_out_dir, '*.js')):
        wrap_single_test(js_file)

    front_page = os.path.join(out_dir, 'index.html')
    with open(front_page, 'w+') as f:
        content = HTML_HEADER.replace('{PREFIX}', './js/harness') \
                             .replace('{WPT_PREFIX}', './js/harness')
        for js_file in glob.glob(os.path.join(js_out_dir, '*.js')):
            filename = os.path.basename(js_file)
            content += "        <script src=./js/{SCRIPT}></script>\n".replace('{SCRIPT}', filename)
        content += HTML_BOTTOM
        f.write(content)

    print('Done building front page!')

# Main program.
def process_args():
    parser = argparse.ArgumentParser(description="Helper tool to build the\
            multi-stage cross-browser test suite for WebAssembly.")

    parser.add_argument('--js',
                        dest="js_dir",
                        help="Relative path to the output directory for the pure JS tests.",
                        type=str)

    parser.add_argument('--html',
                        dest="html_dir",
                        help="Relative path to the output directory for the Web Platform tests.",
                        type=str)

    parser.add_argument('--front',
                        dest="front_dir",
                        help="Relative path to the output directory for the front page.",
                        type=str)

    parser.add_argument('--dont-recompile',
                        action="store_const",
                        dest="compile",
                        help="Don't recompile the wasm spec interpreter (by default, it is)",
                        const=False,
                        default=True)

    return parser.parse_args(), parser

if __name__ == '__main__':
    args, parser = process_args()

    js_dir = args.js_dir
    html_dir = args.html_dir
    front_dir = args.front_dir

    if front_dir is None and js_dir is None and html_dir is None:
        print('At least one mode must be selected.\n')
        parser.print_help()
        sys.exit(1)

    if args.compile:
        compile_wasm_interpreter()

    ensure_wasm_executable(WASM_EXEC)

    if js_dir is not None:
        ensure_empty_dir(js_dir)
        build_js(js_dir)

    if html_dir is not None:
        ensure_empty_dir(html_dir)
        build_html(html_dir, js_dir)

    if front_dir is not None:
        ensure_empty_dir(front_dir)
        build_front_page(front_dir, js_dir)

    print('Done!')

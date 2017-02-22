#!/usr/bin/env python3

import sys
import os
import glob
import subprocess
import shutil

CWD = os.getcwd()
WASM_EXEC = os.path.join(CWD, '..', 'interpreter', 'wasm')

WAST_DIR = os.path.join(CWD, 'core')
JS_DIR = os.path.join(CWD, 'js-api')
HTML_DIR = os.path.join(CWD, 'html')

OUT_DIR = os.path.join(CWD, 'out')
OUT_JS_DIR = os.path.join(OUT_DIR, 'js')
OUT_HTML_DIR = os.path.join(OUT_DIR, 'html')

# Helpers.
def run(*cmd):
    return subprocess.run(cmd,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT,
                          universal_newlines=True)

# Preconditions.
def ensure_dir(path):
    if not os.path.exists(path) or not os.path.isdir(path):
        os.mkdir(path)

def ensure_wasm_executable(path_to_wasm):
    """
    Ensure we have built the wasm spec interpreter.
    """
    result = run(path_to_wasm, '-v', '-e', '')
    if result.returncode != 0:
        print('Unable to run the wasm executable')
        sys.exit(1)


def ensure_clean_initial_state():
    ensure_wasm_executable(WASM_EXEC)

    if os.path.exists(OUT_DIR):
        shutil.rmtree(OUT_DIR)

    ensure_dir(OUT_DIR)
    ensure_dir(OUT_JS_DIR)
    ensure_dir(OUT_HTML_DIR)

# JS harness.
def replace_js_harness(js_file):
    test_func_name = os.path.basename(js_file).replace('.', '_').replace('-', '_')

    content = ["(function {}() {{".format(test_func_name)]
    with open(js_file, 'r') as f:
        content += f.readlines()
    content.append('})();')

    with open(js_file, 'w') as f:
        f.write('\n'.join(content))

def convert_wast_to_js():
    """Compile all the wast files to JS and store the results in the JS dir."""
    for wast_file in glob.glob(os.path.join(WAST_DIR, '*.wast')):
        # Don't try to compile tests that are supposed to fail.
        if 'fail.wast' in wast_file:
            continue

        print('Compiling {} to JS...'.format(wast_file))
        js_filename = os.path.basename(wast_file) + '.js'
        js_file = os.path.join(OUT_JS_DIR, js_filename)
        result = run(WASM_EXEC, wast_file, '-h', '-o', js_file)
        if result.returncode != 0:
            print('Error when compiling {} to JS: {}', wast_file, result.stdout)

        replace_js_harness(js_file)

def build_js():
    print('Building JS...')
    convert_wast_to_js()

    print('Copying JS tests to the JS out dir...')
    for js_file in glob.glob(os.path.join(JS_DIR, '*.js')):
        shutil.copy(js_file, OUT_JS_DIR)

HTML_HEADER = """<!doctype html>
<html>
    <head>
        <meta charset="UTF-8">
        <title>WebAssembly Web Platform Test</title>
        <link rel=author href=mailto:bbouvier@mozilla.com title=bnjbvr>
    </head>
    <body>

        <script src={PREFIX}harness/testharness.js></script>
        <script src={PREFIX}harness/testharnessreport.js></script>
        <script src={PREFIX}harness/index.js></script>
        <script src={PREFIX}harness/wasm-constants.js></script>
        <script src={PREFIX}harness/wasm-module-builder.js></script>

        <div id=log></div>
"""

HTML_BOTTOM = """
    </body>
</html>
"""

def build_html_from_js(src_dir):
    files = []
    for js_file in glob.glob(os.path.join(src_dir, '*.js')):
        if src_dir != OUT_JS_DIR:
            shutil.copy(js_file, OUT_JS_DIR)

        js_filename = os.path.basename(js_file)
        files.append(js_filename)
        html_filename = js_filename + '.html'
        html_file = os.path.join(OUT_HTML_DIR, html_filename)
        with open(html_file, 'w+') as f:
            content = HTML_HEADER.replace('{PREFIX}', '../../')
            content += "        <script src=../js/{SCRIPT}></script>".replace('{SCRIPT}', js_filename)
            content += HTML_BOTTOM
            f.write(content)
    return files

def build_html():
    print("Building HTML tests...")

    print('Building WPT tests from pure JS tests...')
    js_files = build_html_from_js(OUT_JS_DIR)

    print('Building WPT tests from HTML JS tests...')
    js_files += build_html_from_js(HTML_DIR)

    print('Building front page containing all the HTML tests...')
    front_page = os.path.join(OUT_DIR, 'index.html')
    with open(front_page, 'w+') as f:
        content = HTML_HEADER.replace('{PREFIX}', '../')
        for filename in js_files:
            content += "        <script src=./js/{SCRIPT}></script>\n".replace('{SCRIPT}', filename)
            content += "        <script>reinitializeRegistry();</script>\n"
        content += HTML_BOTTOM
        f.write(content)

if __name__ == '__main__':
    ensure_clean_initial_state()
    build_js()
    build_html()
    print('Done!')

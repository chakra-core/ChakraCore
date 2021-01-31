#!/usr/bin/env python
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Regenerate embedded bytecode headers.
# NOTEs:
# 1. this script is for linux and macOS only, on windows please use RegenAllByteCode.cmd AND update_bytecode_version.ps1
# 2. this script uses paths relative to the tools directory it's in so cd to it before running
# 3. this script relies on forcing 64bit CC builds to produce 32bit bytecode - this could break due to future changes to CC.
#    If this facility breaks the CI will fail AND either this will need fixing or bytecode will need to be regenerated using
#    32 bit builds on windows
# 4. Run with flag '--skip-build' if the necessary versions of ChakraCore are already built to skip the compilation step
#    (this is useful if editing the .js files that the bytecode is produced from)
# Two versions of CC and ch must be compiled for bytecode generation 
# ch is used to generate bytecode.
# ch (NoJIT variety) is used to generate NoJIT bytecodes.

import subprocess
import sys
import uuid

# Compile ChakraCore both noJit and Jit variants
def run_sub(message, commands, error):
    print(message)
    sub = subprocess.Popen(commands)
    sub.wait()
    if sub.returncode != 0:
        sys.exit(error)

if len(sys.argv) == 1 or sys.argv[1] != '--skip-build':
    run_sub('Compiling ChakraCore with no Jit',
        ['../build.sh', '--no-jit', '--test-build', '--target-path=../out/noJit', '-j=2'], 
        'No Jit build failed - aborting bytecode generation')
    run_sub('Compiling ChakraCore with Jit',
        ['../build.sh', '--test-build', '--target-path=../out/Jit', '-j=2'], 
        'Jit build failed - aborting bytecode generation')

# Regenerate the bytecode
def bytecode_job(outPath, command, error):
    header = open(outPath, 'w')
    job = subprocess.Popen(command, stdout=header)
    job.wait()
    if job.returncode != 0:
        sys.exit(error)

# INTL
print('Generating INTL bytecode')
bytecode_job('../lib/Runtime/Library/InJavascript/Intl.js.nojit.bc.64b.h',
    ['../out/noJit/test/ch', '-GenerateLibraryByteCodeHeader', '-Intl', '../lib/Runtime/Library/InJavascript/Intl.js'],
    'Failed to generate INTL 64bit noJit bytecode')

bytecode_job('../lib/Runtime/Library/InJavascript/Intl.js.nojit.bc.32b.h',
    ['../out/noJit/test/ch', '-GenerateLibraryByteCodeHeader', '-Intl', '-Force32BitByteCode','../lib/Runtime/Library/InJavascript/Intl.js'],
    'Failed to generate INTL 32bit noJit bytecode')

bytecode_job('../lib/Runtime/Library/InJavascript/Intl.js.bc.64b.h',
    ['../out/Jit/test/ch', '-GenerateLibraryByteCodeHeader', '-Intl', '../lib/Runtime/Library/InJavascript/Intl.js'],
    'Failed to generate INTL 64bit bytecode')

bytecode_job('../lib/Runtime/Library/InJavascript/Intl.js.bc.32b.h',
    ['../out/Jit/test/ch', '-GenerateLibraryByteCodeHeader', '-Intl', '-Force32BitByteCode','../lib/Runtime/Library/InJavascript/Intl.js'],
    'Failed to generate INTL 32bit bytecode')

# JsBuiltin
print('Generating JsBuiltin Bytecode')
bytecode_job('../lib/Runtime/Library/JsBuiltin/JsBuiltin.js.nojit.bc.64b.h',
    ['../out/noJit/test/ch', '-GenerateLibraryByteCodeHeader', '-JsBuiltIn', '-LdChakraLib', '../lib/Runtime/Library/JsBuiltin/JsBuiltin.js'],
    'Failed to generate noJit 64bit JsBuiltin Bytecode')

bytecode_job('../lib/Runtime/Library/JsBuiltin/JsBuiltin.js.nojit.bc.32b.h',
    ['../out/noJit/test/ch', '-GenerateLibraryByteCodeHeader', '-JsBuiltIn', '-LdChakraLib', '-Force32BitByteCode', '../lib/Runtime/Library/JsBuiltin/JsBuiltin.js'],
    'Failed to generate noJit 32bit JsBuiltin Bytecode')

bytecode_job('../lib/Runtime/Library/JsBuiltin/JsBuiltin.js.bc.64b.h',
    ['../out/Jit/test/ch', '-GenerateLibraryByteCodeHeader', '-JsBuiltIn', '-LdChakraLib', '../lib/Runtime/Library/JsBuiltin/JsBuiltin.js'],
    'Failed to generate 64bit JsBuiltin Bytecode')

bytecode_job('../lib/Runtime/Library/JsBuiltin/JsBuiltin.js.bc.32b.h',
    ['../out/Jit/test/ch', '-GenerateLibraryByteCodeHeader', '-JsBuiltIn', '-LdChakraLib', '-Force32BitByteCode', '../lib/Runtime/Library/JsBuiltin/JsBuiltin.js'],
    'Failed to generate 32bit JsBuiltin Bytecode')

# Bytecode regeneration complete - create a new GUID for it
print('Generating new GUID for new bytecode')
guid_header = open('../lib/Runtime/Bytecode/ByteCodeCacheReleaseFileVersion.h', 'w')
guid = str(uuid.uuid4())

output_str = '''//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// NOTE: If there is a merge conflict the correct fix is to make a new GUID.
// This file was generated with tools/xplatRegenByteCode.py

// {%s}
const GUID byteCodeCacheReleaseFileVersion =
{ 0x%s, 0x%s, 0x%s, {0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s } };
''' % (guid,
    guid[:8], guid[9:13], guid[14:18], guid[19:21], guid[21:23], guid[24:26],
    guid[26:28], guid[28:30], guid[30:32], guid[32:34], guid[-2:])

guid_header.write(output_str)

print('Bytecode successfully regenerated. Please rebuild ChakraCore to incorporate it.')

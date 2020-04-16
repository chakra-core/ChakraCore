#!/usr/bin/env python
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Regenerate embedded bytecode headers.
# NOTE this script is for linux and macOS only, on windows please use RegenAllByteCode.cmd AND update_bytecode_version.ps1
# Also note - this script uses paths relative to the tools directory it's in so cd to it before running
# ch is used to generate bytecode.
# ch (NoJIT variety) is used to generate NoJIT bytecodes.


import subprocess
import sys
import uuid

# Compile ChakraCore both noJit and Jit variants
print("Compiling ChakraCore with no Jit")
noJitBuild = subprocess.Popen(['../build.sh', '--no-jit', '--test-build', '--target-path=../out/noJit', '-j=2'])
noJitBuild.wait()

if noJitBuild.returncode!=0:
    sys.exit("No Jit build failed - aborting bytecode generation")

print("Compiling ChakraCore with Jit")
jitBuild = subprocess.Popen(['../build.sh', '--test-build', '--target-path=../out', '-j=2'])
jitBuild.wait()

if jitBuild.returncode!=0:
    sys.exit("Jit build failed - aborting bytecode generation")

#Regenerate the bytecode

#INTL
print("Generating INTL bytecode")
noJitIntlHeader = open('../lib/Runtime/Library/InJavascript/Intl.js.nojit.bc.h','w')
noJitIntl = subprocess.Popen(['../out/noJit/test/ch', '-GenerateLibraryByteCodeHeader', '-Intl', '../lib/Runtime/Library/InJavascript/Intl.js'],stdout=noJitIntlHeader)

jitIntlHeader = open('../lib/Runtime/Library/InJavascript/Intl.js.bc.h','w')
jitIntl = subprocess.Popen(['../out/test/ch', '-GenerateLibraryByteCodeHeader', '-Intl', '../lib/Runtime/Library/InJavascript/Intl.js'],stdout=jitIntlHeader)

noJitIntl.wait()
if noJitIntl.returncode!=0:
    sys.exit("Failed to regenerate INTL noJit Bytecode")

jitIntl.wait()
if jitIntl.returncode!=0:
    sys.exit("Failed to regenerate INTL Jit Bytecode")

#JsBuiltin
print("Generating JsBuiltin Bytecode")
noJitJsBuiltinHeader = open('../lib/Runtime/Library/JsBuiltin/JsBuiltin.js.nojit.bc.h','w')
noJitJsBuiltin = subprocess.Popen(['../out/noJit/test/ch', '-GenerateLibraryByteCodeHeader', '-JsBuiltIn', '-LdChakraLib', '../lib/Runtime/Library/JsBuiltin/JsBuiltin.js'],stdout=noJitJsBuiltinHeader)

jitJsBuiltinHeader = open('../lib/Runtime/Library/JsBuiltin/JsBuiltin.js.bc.h','w')
jitJsBuiltin = subprocess.Popen(['../out/test/ch', '-GenerateLibraryByteCodeHeader', '-JsBuiltIn', '-LdChakraLib', '../lib/Runtime/Library/JsBuiltin/JsBuiltin.js'],stdout=jitJsBuiltinHeader)


noJitJsBuiltin.wait()
if noJitJsBuiltin.returncode!=0:
    sys.exit("Failed to regenerate JsBuiltin noJit Bytecode")

jitJsBuiltin.wait()
if jitJsBuiltin.returncode!=0:
    sys.exit("Failed to regenerate JsBuiltin Jit Bytecode")

#Bytecode regeneration complete - create a new GUID for it

guidHeader = open('../lib/Runtime/Bytecode/ByteCodeCacheReleaseFileVersion.h', 'w')
guid = str(uuid.uuid1())

outputStr = '''//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// NOTE: If there is a merge conflict the correct fix is to make a new GUID.
// This file was generated with tools/xplatRegenBytecode.py

// {%s}
const GUID byteCodeCacheReleaseFileVersion =
{ 0x%s, 0x%s, 0x%s, {0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s } };
''' % (guid,
    guid[:8], guid[9:13], guid[14:18], guid[19:21], guid[21:23], guid[24:26],
    guid[26:28], guid[28:30], guid[30:32], guid[32:34], guid[-2:])

guidHeader.write(outputStr)

print("Bytecode successfully regenerated, please rebuild ChakraCore to incorporate it.")




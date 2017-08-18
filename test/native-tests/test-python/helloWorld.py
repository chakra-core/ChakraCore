#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

import sys
import os.path
from ctypes import *

if sys.platform == "darwin":
    platform = "dylib"
else:
    platform = "so"

build_type = sys.argv[1]
if sys.argv.__len__() > 2 and sys.argv[2] != None:
    so_path = sys.argv[2]
else:
    so_path = "../../../out/" + build_type + "/libChakraCore." + platform

if os.path.isfile(so_path):
    chakraCore = CDLL(so_path)
else:
    print platform + " file not found. It must be a static library"
    sys.exit(0) # static build

script = create_string_buffer("(()=>{return \'Hello world!\';})()")
fileName = "sample.js"

runtime = c_void_p()
# Create Javascript Runtime.
chakraCore.JsCreateRuntime(0, 0, byref(runtime));

context = c_void_p()
# Create an execution context.
chakraCore.JsCreateContext(runtime, byref(context));

# Now set the current execution context.
chakraCore.JsSetCurrentContext(context);

fname = c_void_p();
# create JsValueRef from filename
chakraCore.JsCreateString(fileName, len(fileName), byref(fname));

scriptSource = c_void_p();
# Create ArrayBuffer from script source
chakraCore.JsCreateExternalArrayBuffer(script, len(script), 0, 0, byref(scriptSource));

jsResult = c_void_p();
# Run the script.
chakraCore.JsRun(scriptSource, 0, fname, 0, byref(jsResult));

# Convert script result to String in JavaScript; redundant if script returns a String
resultJSString = c_void_p()
chakraCore.JsConvertValueToString(jsResult, byref(resultJSString));

stringLength = c_size_t();
# Get buffer size needed for the result string
chakraCore.JsCopyString(resultJSString, 0, 0, 0, byref(stringLength));

resultSTR = create_string_buffer(stringLength.value + 1); # buffer is big enough to store the result

# Get String from JsValueRef
chakraCore.JsCopyString(resultJSString, byref(resultSTR), stringLength.value + 1, 0, 0);

# Set `null-ending` to the end
resultSTRLastByte = (c_char * stringLength.value).from_address(addressof(resultSTR))
resultSTRLastByte = '\0';

print("Result from ChakraCore: ", resultSTR.value);

# Dispose runtime
chakraCore.JsDisposeRuntime(runtime);

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ChakraCore.h"
#include <string>
#include <iostream>

using namespace std;

int main()
{
    JsRuntimeHandle runtime;
    JsContextRef context;
    JsValueRef result;
    unsigned currentSourceContext = 0;

    const char* script = "(()=>{return \'SUCCESS\';})()";

    // Create a runtime.
    JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime);

    // Create an execution context.
    JsCreateContext(runtime, &context);

    // Now set the current execution context.
    JsSetCurrentContext(context);

    // Run the script.
    JsRunScriptUtf8(script, currentSourceContext++, "", &result);

    // Convert your script result to String in JavaScript; redundant if your script returns a String
    JsValueRef resultJSString;
    JsConvertValueToString(result, &resultJSString);

    // Project script result back to C++.
    char *resultSTR;
    size_t stringLength;
    JsStringToPointerUtf8Copy(resultJSString, &resultSTR, &stringLength);

    printf("Result -> %s \n", resultSTR);
    JsStringFree(resultSTR);

    // Dispose runtime
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(runtime);

    return 0;
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ChakraCore.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>

#define FAIL_CHECK(cmd)                          \
    do                                           \
    {                                            \
        JsErrorCode errCode = (JsErrorCode) cmd; \
        if (errCode != JsNoError)                \
        {                                        \
            printf("Error %d at '%s'\n",         \
                errCode, #cmd);                  \
            return 1;                            \
        }                                        \
    } while(0)

using namespace std;

unsigned int AddMethodProperty(JsValueRef target, const char* str,
                               JsNativeFunction func)
{
    JsPropertyIdRef Id;
    FAIL_CHECK(JsCreatePropertyId(str, strlen(str), &Id));

    JsValueRef nameVar;
    FAIL_CHECK(JsCreateString(str, strlen(str), &nameVar));

    JsValueRef functionVar;
    FAIL_CHECK(JsCreateNamedFunction(nameVar, func, nullptr, &functionVar));

    FAIL_CHECK(JsSetProperty(target, Id, functionVar, true));

    return JsNoError;
}

JsValueRef CHECK(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    void * data;
    if (JsGetExternalData(arguments[1], &data) == JsNoError &&
        ((size_t*)data)[122] - ((size_t*)data)[0] == 41976)
    {
        fprintf(stdout, "SU"); // CCESS will be printed during finalizer call
    }
    else
    {
        fprintf(stderr, "External data corrupt?");
    }

    return nullptr;
}

JsValueRef createWithProto(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    size_t *data = new size_t[123];
    data[0] = 12345;
    data[122] = 54321;

    JsValueRef obj;

    JsErrorCode code = JsCreateExternalObjectWithPrototype(data, [](void* buffer)
    {
        size_t * mem = (size_t*)buffer;
        if(!(mem[122] - mem[0] == 41976)) abort();
        delete mem;

        // test tool needs to see `SUCCESS` to determine if script was
        // successful. So far the test printed `SU`,
        // this should print the last part `CCESS`
        fprintf(stdout, "CCESS\n");
    }, arguments[1], &obj);

    if (code != JsNoError)
    {
        printf("FAILED to create external object %d \n", code);
    }

    return obj;
}

int main()
{
    JsRuntimeHandle runtime;
    JsContextRef context;
    JsValueRef result;
    unsigned currentSourceContext = 0;

    const char* script = "\
    (function(){\
        var proto = { sub: 3 }; \
        var ext = CreateWithProto(proto); \
        if (ext.sub === 3) CHECK(ext); \
        return \'\'; \
    })();\
    ";

    // Create a runtime.
    JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime);

    // Create an execution context.
    JsCreateContext(runtime, &context);

    // Now set the current execution context.
    JsSetCurrentContext(context);

    JsValueRef fname;
    FAIL_CHECK(JsCreateString("sample", strlen("sample"), &fname));

    JsValueRef scriptSource;
    FAIL_CHECK(JsCreateExternalArrayBuffer((void*)script,
                                           (unsigned)strlen(script),
                                           nullptr, nullptr, &scriptSource));

    JsValueRef global;
    FAIL_CHECK(JsGetGlobalObject(&global));

    FAIL_CHECK(AddMethodProperty(global, "CreateWithProto", &createWithProto));
    FAIL_CHECK(AddMethodProperty(global, "CHECK", &CHECK));

    // Run the script.
    FAIL_CHECK(JsRun(scriptSource, currentSourceContext++, fname,
                     JsParseScriptAttributeNone, &result));

    // Convert your script result to String in JavaScript;
    // redundant if your script returns a String
    JsValueRef resultJSString;
    FAIL_CHECK(JsConvertValueToString(result, &resultJSString));

    // Project script result back to C++.
    char *resultSTR = nullptr;
    size_t stringLength;
    FAIL_CHECK(JsCopyString(resultJSString, nullptr, 0, &stringLength));
    resultSTR = (char*)malloc(stringLength + 1);
    FAIL_CHECK(JsCopyString(resultJSString, resultSTR, stringLength, nullptr));
    resultSTR[stringLength] = 0;

    fprintf(stdout, "%s", resultSTR); // prints an empty string ``
    free(resultSTR);

    // Dispose runtime
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(runtime);

    return 0;
}

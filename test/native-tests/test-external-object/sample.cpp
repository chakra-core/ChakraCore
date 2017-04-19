//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "../CommonDefinitions.h"

using namespace std;

inline JsErrorCode AddMethodProperty(JsValueRef target, const char* str, JsNativeFunction func)
{
    JsPropertyIdRef Id;
    JsErrorCode err = JsCreatePropertyId(str, strlen(str), &Id);
    if (err != JsNoError) return err;

    JsValueRef nameVar;
    err = JsCreateString(str, strlen(str), &nameVar);
    if (err != JsNoError) return err;

    JsValueRef functionVar;
    err = JsCreateNamedFunction(nameVar, func, nullptr, &functionVar);
    if (err != JsNoError) return err;

    err = JsSetProperty(target, Id, functionVar, true);

    return err;
}

JsValueRef addExternalDataId(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    size_t *org = new size_t[123];
    org[0] = 12345;
    org[122] = 54321;
    JsErrorCode code = JsSetExternalDataWithCallback(arguments[1], org, [](void* data)
    {
        size_t *buffer = (size_t*) data;
        if (buffer[0] != 12345 || buffer[122] != 54321) abort();
        delete buffer;

        // Test checker searches for `SUCCESS` to determine if script was successful
        // If everything goes well, this should print last
        printf("ESS!\n");
    });
    if (code != JsNoError) printf("FAILED to set external data %d \n", code);
    return nullptr;
}

JsValueRef hasExternalDataId(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
  bool hasExternal = false;
  JsErrorCode code = JsHasExternalData(arguments[1], &hasExternal);
  if (code != JsNoError) printf("FAILED to check external data %d \n", code);

  JsValueRef boolValue;
  JsBoolToBoolean(hasExternal, &boolValue);

  return boolValue;
}

JsValueRef getExternalDataId(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    void *data;
    JsErrorCode code = JsGetExternalData(arguments[1], &data);
    if (code != JsNoError) printf("FAILED to get external data %d \n", code);

    if (!data) return nullptr;

    size_t *buffer = (size_t*)data;
    JsValueRef dblPtr;
    JsDoubleToNumber(buffer[0], &dblPtr);

    return dblPtr;
}

int main()
{
    JsRuntimeHandle runtime;
    JsContextRef context;
    JsValueRef result;
    unsigned currentSourceContext = 0;

    const char* script = "\
    (function(){\
        do {\
            var obj = {};\
            AddExternalData(obj);\
            if (!HasExternalData(obj)) throw new Error('HasExternalData Failed');\
            if (GetExternalData(obj) != 12345) throw new Error('GetExternalData Failed');\
        } while(0);\
        return \"SUCC\";\
    })();\
    "; // this script will print `SUCC` and callback defined under `addExternalDataId`
       // will print `ESS!` afterwards. In combination, test checker will see `SUCCESS!`

    // Create a runtime.
    JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime);

    // Create an execution context.
    JsCreateContext(runtime, &context);

    // Now set the current execution context.
    JsSetCurrentContext(context);

    JsValueRef fname;
    FAIL_CHECK(JsCreateString("sample", strlen("sample"), &fname));

    JsValueRef scriptSource;
    FAIL_CHECK(JsCreateExternalArrayBuffer((void*)script, (unsigned int)strlen(script),
        nullptr, nullptr, &scriptSource));

    JsValueRef global;
    FAIL_CHECK(JsGetGlobalObject(&global));

    FAIL_CHECK(AddMethodProperty(global, "AddExternalData", &addExternalDataId));

    FAIL_CHECK(AddMethodProperty(global, "HasExternalData", &hasExternalDataId));

    FAIL_CHECK(AddMethodProperty(global, "GetExternalData", &getExternalDataId));

    // Run the script.
    FAIL_CHECK(JsRun(scriptSource, currentSourceContext++, fname, JsParseScriptAttributeNone, &result));

    // Convert your script result to String in JavaScript; redundant if your script returns a String
    JsValueRef resultJSString;
    FAIL_CHECK(JsConvertValueToString(result, &resultJSString));

    // Project script result back to C++.
    char *resultSTR = nullptr;
    size_t stringLength;
    FAIL_CHECK(JsCopyString(resultJSString, nullptr, 0, &stringLength));
    resultSTR = (char*) malloc(stringLength + 1);
    FAIL_CHECK(JsCopyString(resultJSString, resultSTR, stringLength + 1, nullptr));
    resultSTR[stringLength] = 0;

    printf("%s", resultSTR); // prints `SUCC`
    free(resultSTR);

    // Dispose runtime
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(runtime);

    return 0;
}

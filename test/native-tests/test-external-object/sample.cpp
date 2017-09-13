//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "../CommonDefinitions.h"

using namespace std;

const char letters [5] = { 'E', 'S', 'S', '!', '\n' };
static int hitCount = 0;

JsValueRef addExternalDataId(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    size_t *org = new size_t[123];
    org[0] = 12345;
    org[122] = 54321;
    JsErrorCode code = JsObjectSetExternalDataWithCallback(arguments[1], org, [](void* data)
    {
        size_t *buffer = (size_t*) data;
        if (buffer[0] != 12345 || buffer[122] != 54321) abort();
        delete buffer;

        // Test checker searches for `SUCCESS` to determine if script was successful
        // If everything goes well, this should print last
        printf("%c", letters[hitCount++]);
    });
    if (code != JsNoError) printf("FAILED to set external data %d \n", code);
    return nullptr;
}

JsValueRef sameTypeHandler(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    bool hasExternal = false;
    JsValueRef obj1 = arguments[1];
    JsValueRef obj2 = arguments[2];

    char * fmem1 = (char*) obj1;
    char * fmem2 = (char*) obj2;

    union MemBlock
    {
      void* addr;
      char  mem[8];
    };

    MemBlock first;
    memcpy(first.mem, fmem1, 8); // copy the address of Type

    MemBlock second;
    memcpy(second.mem, fmem2, 8); // copy the address of Type

    fprintf(stderr, "First: %p Second: %p \n", first.addr, second.addr);

    JsValueRef boolValue;
    JsBoolToBoolean(second.addr == second.addr, &boolValue);

    return boolValue;
}

JsValueRef hasExternalDataId(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    bool hasExternal = false;
    JsValueRef obj = arguments[1];

    JsErrorCode code = JsObjectHasExternalData(obj, &hasExternal);
    if (code != JsNoError) printf("FAILED to check external data %d \n", code);

    JsValueRef boolValue;
    JsBoolToBoolean(hasExternal, &boolValue);

    return boolValue;
}

JsValueRef getExternalDataId(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    void *data;
    JsErrorCode code = JsObjectGetExternalData(arguments[1], &data);
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

    // this script will print `S` + `UCC` and callback defined under `addExternalDataId`
    // will print `ESS!` afterwards. In combination, `SUCCESS!`

    // Each array item triggers a letter from ESS!\n on their  ExternalData finalization.

    // First part tests type sharing among objects with externalData

    // obj[.....] parts (Date.now() + a / b) are to push type fork. (so we test type handler converters)
    // prior to setting unique property, we set CommonPropertyName to force shared type handling

    // the loop for function is to trigger further optimizations

    // object property loop is to test auxSlot expansion
    const char* script = "\
    (function(){\
        var fncBase = function(nn) { this.xyz = nn; };\
        AddExternalData(fncBase);\
        var obj1 = new fncBase(1), obj2 = new fncBase(2);\
        obj1.y = 1;\
        obj2.y = 2;\
        AddExternalData(obj1);\
        AddExternalData(obj2);\
        obj1.a = 1;\
        obj2.b = 2;\
        if (!HasExternalData(obj1)) throw new Error('HasExternalData TypeHandler transfer failed (a)');\
        if (!HasExternalData(obj2)) throw new Error('HasExternalData TypeHandler transfer failed (b)');\
        if (!SameTypeHandler(obj1, obj2)) throw new Error('TypeHandler sharing failed');\
        var retVal = \"UCC\";\
        var arr = [ {}, [], function(r) { if (!r) return \"S\"; else this.X=r; }, new Date(), new Object() ];\
        for (var i = 0; i < arr.length; i++) {\
            var obj = arr[i];\
            AddExternalData(obj);\
            obj.CommonPropertyName = new arr[2](i);\
            if (!HasExternalData(obj)) throw new Error('HasExternalData 1st Failed');\
            obj[Date.now() + 'a'] = new Date();\
            if (!HasExternalData(obj)) throw new Error('HasExternalData 2nd Failed');\
            obj[Date.now() + 'b'] = new Date();\
            if (GetExternalData(obj) != 12345) throw new Error('GetExternalData Failed');\
        }\
        var m = \"\";\
        for (var j = 0; j < 1e6; j++) m = arr[2](j);\
        if (!HasExternalData(arr[2])) throw new Error('HasExternalData 3rd Failed');\
        for (var j = 0; j < 1e4; j++) \
        { \
            arr[0]['a' + j] = 4; \
        } \
        if (!HasExternalData(arr[0])) throw new Error('HasExternalData 4th Failed');\
        return arr[2]() + retVal;\
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
    FAIL_CHECK(JsCreateExternalArrayBuffer((void*)script, (unsigned int)strlen(script),
        nullptr, nullptr, &scriptSource));

    JsValueRef global;
    FAIL_CHECK(JsGetGlobalObject(&global));

    FAIL_CHECK(AddMethodProperty(global, "AddExternalData", &addExternalDataId));

    FAIL_CHECK(AddMethodProperty(global, "HasExternalData", &hasExternalDataId));

    FAIL_CHECK(AddMethodProperty(global, "GetExternalData", &getExternalDataId));

    FAIL_CHECK(AddMethodProperty(global, "SameTypeHandler", &sameTypeHandler));

    // Run the script.
    FAIL_CHECK(JsRun(scriptSource, currentSourceContext++, fname, JsParseScriptAttributeNone, &result));

    // Convert your script result to String in JavaScript; redundant if your script returns a String
    JsValueRef resultJSString;
    FAIL_CHECK(JsConvertValueToString(result, &resultJSString));

    // Project script result back to C++.
    char *resultSTR = nullptr;
    size_t stringLength;
    FAIL_CHECK(JsCopyString(resultJSString, nullptr, 0, &stringLength));
    resultSTR = (char*)malloc(stringLength + 1);
    FAIL_CHECK(JsCopyString(resultJSString, resultSTR, stringLength, nullptr));
    resultSTR[stringLength] = 0;

    printf("%s", resultSTR); // prints `SUCC`
    free(resultSTR);

    // Dispose runtime
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(runtime);

    return 0;
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ChakraCore.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>

#define FAIL_CHECK(cmd)                     \
    do                                      \
    {                                       \
        JsErrorCode errCode = cmd;          \
        if (errCode != JsNoError)           \
        {                                   \
            printf("Error %d at '%s'\n",    \
                errCode, #cmd);             \
            return 1;                       \
        }                                   \
    } while(0)

using namespace std;

int main()
{
    JsRuntimeHandle runtime;
    JsContextRef context;
    unsigned currentSourceContext = 0;

    // Create a runtime.
    JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime);

    // Create an execution context.
    JsCreateContext(runtime, &context);

    // Now set the current execution context.
    JsSetCurrentContext(context);

    JsValueRef valueRef;
    FAIL_CHECK(JsCreateString("sample", strlen("sample"), &valueRef));

    JsWeakRef weakRef;
    FAIL_CHECK(JsCreateWeakReference(valueRef, &weakRef));

    FAIL_CHECK(JsGetWeakReferenceValue(weakRef, &valueRef));
    if (valueRef == JS_INVALID_REFERENCE) {
        printf("JsGetWeakReference() returned invalid reference.\n");
        return 1;
    }

    // Clear the reference on the stack, so that the value will be GC'd.
    valueRef = JS_INVALID_REFERENCE;

    FAIL_CHECK(JsCollectGarbage(runtime));

    FAIL_CHECK(JsGetWeakReferenceValue(weakRef, &valueRef));
    if (valueRef != JS_INVALID_REFERENCE) {
        printf("JsGetWeakReference() should have returned invalid reference after GC.\n");
        return 1;
    }

    printf("Result -> SUCCESS \n");

    // Dispose runtime
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(runtime);

    return 0;
}

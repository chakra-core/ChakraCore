//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ChakraCore.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>

#define ERROR_INSPECT                                                   \
    do {                                                                \
        JsValueRef exception = nullptr, exceptionString;                \
        JsGetAndClearException(&exception);                             \
        if (exception) {                                                \
            JsConvertValueToString(exception, &exceptionString);        \
            char errorString[8192];                                     \
            JsCopyString(exceptionString, errorString, 8192, nullptr);  \
            printf("Exception:\n%s\n", errorString);                    \
        }                                                               \
    } while(0);

#define FAIL_CHECK(cmd)                     \
    do                                      \
    {                                       \
        JsErrorCode errCode = cmd;          \
        if (errCode != JsNoError)           \
        {                                   \
            ERROR_INSPECT                   \
            printf("Error %d at '%s'\n",    \
                errCode, #cmd);             \
            return errCode;                 \
        }                                   \
    } while(0)

JsErrorCode AddMethodProperty(JsValueRef target, const char* str, JsNativeFunction func)
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

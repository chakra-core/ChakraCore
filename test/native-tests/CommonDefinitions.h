//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ChakraCore.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>

#define FAIL_CHECK(cmd)                                                 \
do                                                                      \
{                                                                       \
    JsErrorCode errCode = cmd;                                          \
    if (errCode != JsNoError)                                           \
    {                                                                   \
        bool hasException = false;                                      \
        JsValueRef exception = nullptr, exceptionString;                \
        JsGetAndClearException(&exception);                             \
        if (exception) {                                                \
            JsConvertValueToString(exception, &exceptionString);        \
            char errorString[8192];                                     \
            size_t written = 0;                                         \
            JsCopyString(exceptionString, errorString, 8192, &written); \
            printf("Exception:\n%s\n", errorString);                    \
        }                                                               \
        printf("Error %d at '%s'\n", errCode, #cmd);                    \
        return 1;                                                       \
    }                                                                   \
} while(0)

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "Common.h"
#include "ChakraPlatform.h"

namespace PlatformAgnostic
{
    namespace EventTrace
    {
        void FireGenericEventTrace(const void* traceData)
        {
            JS_ETW(EventWriteJSCRIPT_INTERNAL_GENERIC_EVENT(static_cast<const char16*>(traceData)));
        }
    }
}
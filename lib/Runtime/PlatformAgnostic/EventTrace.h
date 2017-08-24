//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Core/CommonTypedefs.h"

namespace PlatformAgnostic
{
    namespace EventTrace
    {
        void FireGenericEventTrace(const void* traceData);
    }
}
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

typedef unsigned __int32 charcount_t;

namespace ChPlatformAgnostic
{
    class Module
    {
    public:
        static bool GetBinaryLocation(char* const path, const charcount_t size, charcount_t* const resultStrLength);
    };
} // namespace ChPlatformAgnostic

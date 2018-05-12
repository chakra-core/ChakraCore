//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace PlatformAgnostic
{
namespace Arrays
{
    class Utility
    {
#ifndef ENABLE_GLOBALIZATION
        class ArraysLocale
        {
            WCHAR commaSeparator;
            WCHAR semicolonSeparator;
            WCHAR localeSeparator;

        public:

            ArraysLocale();

            inline WCHAR GetLocaleSeparator() { return localeSeparator; }
        };

        static ArraysLocale arraysLocale;
#endif
    public:

        static size_t GetLocaleSeparator(WCHAR* szSeparator);
    };

} // namespace Arrays
} // namespace PlatformAgnostic

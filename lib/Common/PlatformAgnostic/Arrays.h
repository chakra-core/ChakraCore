//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace PlatformAgnostic
{
namespace Arrays
{
#ifndef ENABLE_GLOBALIZATION

    class ArrayLocalization
    {
        char16 localeSeparator;

    public:

        ArrayLocalization();

        inline char16 GetLocaleSeparator() { return localeSeparator; }
    };

#endif   

    size_t GetLocaleSeparator(WCHAR* szSeparator);

} // namespace Arrays
} // namespace PlatformAgnostic

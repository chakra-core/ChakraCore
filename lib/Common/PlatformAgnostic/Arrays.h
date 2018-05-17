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

    // According to MSDN the maximum number of characters for the list separator (LOCALE_SLIST) is four, including a terminating null character.
    bool GetLocaleSeparator(char16* szSeparator, uint32* sepOutSize, uint32 sepBufferSize = 0);

} // namespace Arrays
} // namespace PlatformAgnostic

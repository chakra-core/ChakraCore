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

        static const uint16 BufSize = 256;

    public:

        ArrayLocalization();

        inline char16 GetLocaleSeparator() { return localeSeparator; }
    };

#endif   

    static const uint32 SeparatorBufferSize = 6;

    // According to MSDN the maximum number of characters for the list separator (LOCALE_SLIST) is four, including a terminating null character.
    bool GetLocaleSeparator(char16* szSeparator, uint32* sepOutLength, uint32 sepBufferSize);

    template <uint32 sepBufferSize>
    inline bool GetLocaleSeparator(char16(&szSepatator)[sepBufferSize], uint32 *sepOutLength)
    {
        return GetLocaleSeparator(szSeparator, sepOutLength, sepBufferSize);
    }

} // namespace Arrays
} // namespace PlatformAgnostic

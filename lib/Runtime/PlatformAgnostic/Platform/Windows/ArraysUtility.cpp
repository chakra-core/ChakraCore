//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "Common.h"
#include "ChakraPlatform.h"

namespace PlatformAgnostic
{
namespace Arrays
{
    // Potential candidate for optimization 
    bool GetLocaleSeparator(char16* szSeparator, uint32* sepOutSize, uint32 sepBufSize)
    {
        char16 localeName[LOCALE_NAME_MAX_LENGTH] = { 0 };

        int ret = GetUserDefaultLocaleName(localeName, _countof(localeName));

        if (ret == 0)
        {
            AssertMsg(FALSE, "GetUserDefaultLocaleName failed");
            return false;
        }

        *sepOutSize = GetLocaleInfoEx(localeName, LOCALE_SLIST, szSeparator, sepBufSize);

        if (*sepOutSize == 0)
        {
            AssertMsg(FALSE, "GetLocaleInfo failed");
            return false;
        }
        else
        {
            // Append ' ' if necessary
            if (*sepOutSize < 2 || szSeparator[*sepOutSize - 2] != ' ')
            {
                szSeparator[*sepOutSize - 1] = ' ';
                szSeparator[*sepOutSize] = '\0';
            }
        }

        return true;
    }

} // namespace Arrays
} // namespace PlatformAgnostic

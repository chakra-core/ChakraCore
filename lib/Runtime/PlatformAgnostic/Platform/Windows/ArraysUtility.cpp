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

    size_t GetLocaleSeparator(WCHAR* szSeparator)
    {

        LCID lcid = GetUserDefaultLCID();
        size_t count = 0;

        count = GetLocaleInfoW(lcid, LOCALE_SLIST, szSeparator, 5);
        if (!count)
        {
            AssertMsg(FALSE, "GetLocaleInfo failed");
            return count;
        }
        else
        {
            // Append ' '  if necessary
            if (count < 2 || szSeparator[count - 2] != ' ')
            {
                szSeparator[count - 1] = ' ';
                szSeparator[count] = '\0';
            }
        }

        return count;
    }

} // namespace Arrays
} // namespace PlatformAgnostic

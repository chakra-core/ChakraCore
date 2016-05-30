//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "Common.h"
#include "ChakraPlatform.h"

namespace PlatformAgnostic
{
namespace Numbers
{
    size_t Utility::NumberToDefaultLocaleString(const WCHAR *number_string,
                                                const size_t length,
                                                WCHAR *buffer,
                                                const size_t pre_allocated_buffer_size)
    {
        size_t count = GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0,
                                          number_string, NULL, NULL, 0);

        AssertMsg(count <= INT_MAX, "required buffer shouldn't be bigger than INTMAX");
        if (count != 0 && count <= pre_allocated_buffer_size)
        {
            AssertMsg(pre_allocated_buffer_size <= INT_MAX,
                     "pre_allocated_buffer_size shouldn't be this big!");
            return GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, number_string,
                                     NULL, buffer, (int)pre_allocated_buffer_size);
        }

        return count;
    }

} // namespace Numbers
} // namespace PlatformAgnostic

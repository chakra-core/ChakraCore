//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Common.h"
#include "ChakraPlatform.h"
#include <locale.h>

namespace PlatformAgnostic
{
namespace Arrays
{
    #define BufSize 256

    static char16 commaSeparator = _u(',');
    static char16 semicolonSeparator = _u(';');

    ArrayLocalization::ArrayLocalization()
    {
        char buffer[BufSize];
        char *old_locale = setlocale(LC_NUMERIC, NULL);

        if (old_locale != NULL)
        {
            memcpy(buffer, old_locale, BufSize);

            setlocale(LC_NUMERIC, "");
        }

        struct lconv *locale_format = localeconv();
        if (locale_format != NULL)
        {
            // No specific list separator on Linux/macOS
            // Use this pattern to determine the list separator
            if (*locale_format->decimal_point == commaSeparator)
            {
                localeSeparator = semicolonSeparator;
            }
            else
            {
                localeSeparator = commaSeparator;
            }
        }

        if (old_locale != NULL)
        {
            setlocale(LC_NUMERIC, buffer);
        }
    }

    size_t GetLocaleSeparator(WCHAR *szSeparator)
    {

        ArrayLocalization arrayLocalization;

        size_t count = 0;

        //Add ' ' after separator
        szSeparator[count] = arrayLocalization.GetLocaleSeparator();
        szSeparator[++count] = ' ';
        szSeparator[++count] = '\0';

        return count;
    }

} // namespace Arrays
} // namespace PlatformAgnostic

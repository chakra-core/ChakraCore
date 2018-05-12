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

    template<class T, class N = T>
    void ByteCopy(const T *from, N *to, const size_t max_length)
    {
        size_t length = 0;
        for (; length < max_length && (to[length] = static_cast<N>(from[length])) != 0;
            length++);
        if (length == max_length)
        {
            to[length - 1] = N(0);
        }
    }

    Utility::ArraysLocale::ArraysLocale()
    {

        commaSeparator = WCHAR(',');
        semicolonSeparator = WCHAR(';');

        char buffer[BufSize];
        char *old_locale = setlocale(LC_NUMERIC, NULL);

        if (old_locale != NULL)
        {
            ByteCopy<char>(old_locale, buffer, BufSize);

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

    Utility::ArraysLocale Utility::arraysLocale;

    size_t Utility::GetLocaleSeparator(WCHAR *szSeparator)
    {

        size_t count = 0;

        //Add ' ' after separator
        szSeparator[count] = arraysLocale.GetLocaleSeparator();
        szSeparator[++count] = ' ';
        szSeparator[++count] = '\0';

        return count;
    }

} // namespace Arrays
} // namespace PlatformAgnostic

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

    bool GetLocaleSeparator(char16* szSeparator, uint32* sepOutSize, uint32 sepBufSize)
    {
        ArrayLocalization arrayLocalization;
        //Append ' ' after separator
        szSeparator[*sepOutSize] = arrayLocalization.GetLocaleSeparator();
        szSeparator[++(*sepOutSize)] = ' ';
        szSeparator[++(*sepOutSize)] = '\0';

        return true;
    }

} // namespace Arrays
} // namespace PlatformAgnostic

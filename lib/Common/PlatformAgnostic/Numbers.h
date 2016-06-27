//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef RUNTIME_PLATFORM_AGNOSTIC_COMMON_NUMBERS
#define RUNTIME_PLATFORM_AGNOSTIC_COMMON_NUMBERS

namespace PlatformAgnostic
{
namespace Numbers
{
    class Utility
    {
#ifndef ENABLE_GLOBALIZATION
        class NumbersLocale
        {
            WCHAR localeThousands;
            WCHAR localeDecimal;
            WCHAR localeNegativeSign;

            int maxDigitsAfterDecimals;
            WCHAR defaultDecimalDot;
            WCHAR defaultDecimalComma;

        public:

            NumbersLocale();

            inline int   GetMaxDigitsAfterDecimals() { return maxDigitsAfterDecimals; }
            inline WCHAR GetLocaleThousands()        { return localeThousands; }
            inline bool  HasLocaleThousands()        { return localeThousands != 0; }
            inline WCHAR GetLocaleDecimal()          { return localeDecimal; }
            inline WCHAR GetLocaleNegativeSign()     { return localeNegativeSign; }

            inline bool  IsDecimalPoint(const WCHAR wc)
                                                     { return wc == defaultDecimalDot
                                                           || wc == defaultDecimalComma; }
        };

        // non-ICU implementation keeps user locale intact process wide
        // xplat-todo: While implementing ICU option, make both per context.
        static NumbersLocale numbersLocale;
#endif
    public:

        static size_t NumberToDefaultLocaleString(const WCHAR *number_string,
                                                  const size_t length,
                                                  WCHAR *buffer,
                                                  const size_t pre_allocated_buffer_size);
    };
} // namespace Numbers
} // namespace PlatformAgnostic

#endif // RUNTIME_PLATFORM_AGNOSTIC_COMMON_NUMBERS

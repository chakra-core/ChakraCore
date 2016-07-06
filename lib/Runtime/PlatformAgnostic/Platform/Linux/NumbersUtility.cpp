//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Common.h"
#include "ChakraPlatform.h"
#include <locale.h>

namespace PlatformAgnostic
{
namespace Numbers
{
    #define BufSize 256

    template<class T, class N=T>
    void ByteCopy(const T *from, N *to, const size_t max_length)
    {
        size_t length = 0;
        for(;length < max_length && (to[length] = static_cast<N>(from[length])) != 0;
            length++);
        if (length == max_length)
        {
            to[length-1] = N(0);
        }
    }

    static const WCHAR number_zero        = WCHAR('0');
    static const WCHAR number_five        = WCHAR('5');
    static const WCHAR number_nine        = WCHAR('9');

    Utility::NumbersLocale::NumbersLocale()
    {
        maxDigitsAfterDecimals = 3;
        defaultDecimalDot = WCHAR('.');
        defaultDecimalComma = WCHAR(',');
        localeNegativeSign = WCHAR('-');
        
        char buffer[BufSize];
        char *old_locale = setlocale(LC_NUMERIC, NULL);

        if(old_locale != NULL)
        {
            ByteCopy<char>(old_locale, buffer, BufSize);

            // get user's LC_NUMERIC from OS
            setlocale(LC_NUMERIC, "");
        }

        struct lconv *locale_format = localeconv();
        if (locale_format != NULL)
        {
            localeThousands = (WCHAR) *(locale_format->thousands_sep);
            localeDecimal = (WCHAR) *(locale_format->decimal_point);
        }
        else // default to en_US
        {
            localeThousands = (WCHAR) ',';
            localeDecimal = (WCHAR) '.';
        }

        if (old_locale != NULL)
        {
            // set process/LC_NUMERIC back
            setlocale(LC_NUMERIC, buffer);
        }
    }

    Utility::NumbersLocale Utility::numbersLocale;

    size_t Utility::NumberToDefaultLocaleString(const WCHAR *number_string,
                                                const size_t length,
                                                WCHAR *buffer,
                                                const size_t pre_allocated_buffer_size)
    {
        if (number_string == nullptr || length == 0 || length >= INT_MAX)
        {
            return 0;
        }

        size_t decimal_pos = length; // by default, assume there is no decimal pointer
        bool is_negative = number_string[0] == numbersLocale.GetLocaleNegativeSign();
        int minimum_pos = is_negative ? 1 : 0;

        // find decimal position
        for(int pos = (int)length; pos >= minimum_pos; pos--)
        {
            WCHAR num = number_string[pos];

            if(numbersLocale.IsDecimalPoint(num))
            {
                decimal_pos = pos; // number is no longer an integer
                break;
            }
        }

        size_t int_length = decimal_pos - minimum_pos;
        size_t dec_length = length > decimal_pos ? length - decimal_pos : 0;
        size_t end_pos = length - 1;
        bool rollup_last_digit = false;

        if (dec_length) // non-integer
        {
            const size_t maxDigits = numbersLocale.GetMaxDigitsAfterDecimals();
            if (dec_length > maxDigits)
            {
                WCHAR num = number_string[decimal_pos + maxDigits + 1];

                if (num >= number_five)
                {
                    rollup_last_digit = true;
                }
                end_pos -= (dec_length - maxDigits) - 1;
                dec_length = maxDigits;
            }

            bool passed_zero = false;
            AssertMsg(decimal_pos != 0, "Decimal pointer shouldn't be at index 0");
            for(size_t pos = end_pos; pos > decimal_pos; pos--)
            {
                WCHAR num = number_string[pos];

                if (rollup_last_digit && num == number_nine)
                {
                    if ( passed_zero ) break;
                }
                else if (num == number_zero)
                {
                    passed_zero = true;
                    if ( rollup_last_digit ) break;
                }
                else
                {
                    break;
                }

                end_pos--;
                dec_length--;
            }

            // It looks like we will format number to an integer 0.9999 -> 1
            // So, skip decimal point too
            if (dec_length == 0)
            {
                end_pos--;
            }
        }

        bool add_extra_one = false;
        // see if we are going to add an extra digit to integer side
        // i.e. 999.9999 -> (1)000
        if (rollup_last_digit && dec_length == 0)
        {
            add_extra_one = true;
            for(size_t pos = minimum_pos; pos < decimal_pos; pos++)
            {
                WCHAR num = number_string[pos];
                
                if ( num != number_nine )
                {
                    add_extra_one = false;
                    break;
                }
            }

            if (add_extra_one)
            {
                int_length++;
            }
        }

        size_t expected_length = end_pos + 1 + (add_extra_one ? 1 : 0);

        if (numbersLocale.HasLocaleThousands() && int_length > 3) // if locale has a sign for thousands
        {
            expected_length += int_length / 3;
            if (int_length % 3 == 0)
            {
                expected_length--;
            }
        }

        AssertMsg(expected_length < INT_MAX,
             "Length of the formatted string shouldn't be >= INT_MAX");

        // check if we have enough buffer before formatting
        if (expected_length + /* \0 */ 1 > pre_allocated_buffer_size)
        {
            return expected_length + 1;
        }

        size_t bpos = expected_length - 1;
        if (dec_length > 0) // non-integer
        {
            for(size_t pos = end_pos; pos > decimal_pos; pos--)
            {
                WCHAR num = number_string[pos];
                buffer[bpos] = num;
                
                if (rollup_last_digit)
                {
                    if (num == number_nine)
                    {
                        buffer[bpos] = number_zero;
                    }
                    else
                    {
                        buffer[bpos] += 1;
                        rollup_last_digit = false;
                    }
                }
                bpos--;
            }
            buffer[bpos--] = numbersLocale.GetLocaleDecimal();
        }

        int thousand_counter = 0;
        const bool hasLocaleThousands = numbersLocale.HasLocaleThousands();
        for (int pos = (int)decimal_pos - 1; pos >= minimum_pos; pos--)
        {
            WCHAR num = number_string[pos];

            if (hasLocaleThousands)
            {
                if (thousand_counter && thousand_counter % 3  == 0 )
                {
                    buffer[bpos--] = numbersLocale.GetLocaleThousands();
                }
                thousand_counter++;
            }

            buffer[bpos] = num;
            if (rollup_last_digit)
            {
                if (num == number_nine)
                {
                    buffer[bpos] = number_zero;
                }
                else
                {
                    buffer[bpos] = num + 1;
                    rollup_last_digit = false;
                }
            }
            bpos--;
        }

        if (add_extra_one) // 999.9999 -> (1)000
        {
            if (thousand_counter && thousand_counter % 3 == 0)
            {
                buffer[bpos--] = numbersLocale.GetLocaleThousands();
            }
            buffer[bpos--] = number_zero + 1;
        }

        buffer[expected_length] = WCHAR(0);

        if (is_negative)
        {
            buffer[0] = numbersLocale.GetLocaleNegativeSign();
        }

        return expected_length + 1 /* \0 */;
    }
} // namespace Numbers
} // namespace PlatformAgnostic

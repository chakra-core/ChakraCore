//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "UnicodeText.h"
#include "UnicodeTextInternal.h"

#include <cctype>
#include <string.h>
#define IS_CHAR(ch) \
        (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')

#define IS_NUMBER(ch) \
        (ch >= '0' && ch <= '9')

#define minm(a,b) ((a < b) ? a : b)

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
        // We actually don't care about the legacy behavior on xplat since no one
        // has a dependency on it. So this actually is different between
        // Windows and xplat
        CharacterClassificationType GetLegacyCharacterClassificationType(char16 character)
        {
            return CharacterClassificationType::Invalid;
        }

        int32 NormalizeString(NormalizationForm normalizationForm, const char16* sourceString, uint32 sourceLength, char16* destString, int32 destLength, ApiError* pErrorOut)
        {
            *pErrorOut = ApiError::NoError;
            if (destString == nullptr)
            {
                return sourceLength;
            }

            int32 len = (int32)minm(minm(destLength, sourceLength), INT_MAX);
            memcpy(destString, sourceString, len * sizeof(char16));

            return len;
        }

        bool IsNormalizedString(NormalizationForm normalizationForm, const char16* testString, int32 testStringLength) {
            // TODO: implement this
            return true;
        }

        template<bool toUpper, bool useInvariant>
        charcount_t ChangeStringLinguisticCase(const char16* sourceString, charcount_t sourceLength, char16* destString, charcount_t destLength, ApiError* pErrorOut)
        {
            typedef WCHAR(*CaseConversionFunc)(WCHAR);
            *pErrorOut = ApiError::NoError;
            if (destString == nullptr)
            {
                return sourceLength;
            }

            charcount_t len = static_cast<charcount_t>(minm(minm(destLength, sourceLength), MaxCharCount));
            CaseConversionFunc fnc = toUpper ? PAL_towupper : PAL_towlower;
            for (charcount_t i = 0; i < len; i++)
            {
                destString[i] = fnc(sourceString[i]);
            }

            return static_cast<charcount_t>(len);
        }

        bool IsWhitespace(codepoint_t ch)
        {
            if (ch > 127)
            {
                return ch == 0xA0   || // NO-BREAK SPACE
                       ch == 0x3000 || // IDEOGRAPHIC SPACE
                       ch == 0xFEFF;   // ZERO WIDTH NO-BREAK SPACE (BOM)
            }

            char asc = (char)ch;
            return asc == ' ' || asc == '\n' || asc == '\t' || asc == '\r';
        }

        bool IsIdStart(codepoint_t ch)
        {
            // Following codepoints are treated as part of ID_Start
            // for backwards compatibility as per section 2.5 of the Unicode 8 spec
            // See http://www.unicode.org/reports/tr31/tr31-23.html#Backward_Compatibility
            // The exact list is in PropList.txt in the Unicode database
            switch (ch)
            {
            case 0x2118: // SCRIPT CAPITAL P
            case 0x212E: // ESTIMATED SYMBOL
            case 0x309B: // KATAKANA-HIRAGANA VOICED SOUND MARK
            case 0x309C: // KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
                return true;
            default:
                return false;
            }
        }

        bool IsIdContinue(codepoint_t ch)
        {
            // Following codepoints are treated as part of ID_Continue
            // for backwards compatibility as per section 2.5 of the Unicode 8 spec
            // See http://www.unicode.org/reports/tr31/tr31-23.html#Backward_Compatibility
            // The exact list is in PropList.txt in the Unicode database
            switch (ch)
            {
            case 0x00B7: return true; // MIDDLE DOT
            case 0x0387: return true; // GREEK ANO TELEIA
            case 0x1369: return true; // ETHIOPIC DIGIT ONE
            case 0x136A: return true; // ETHIOPIC DIGIT TWO
            case 0x136B: return true; // ETHIOPIC DIGIT THREE
            case 0x136C: return true; // ETHIOPIC DIGIT FOUR
            case 0x136D: return true; // ETHIOPIC DIGIT FIVE
            case 0x136E: return true; // ETHIOPIC DIGIT SIX
            case 0x136F: return true; // ETHIOPIC DIGIT SEVEN
            case 0x1370: return true; // ETHIOPIC DIGIT EIGHT
            case 0x1371: return true; // ETHIOPIC DIGIT NINE
            case 0x19DA: return true; // NEW TAI LUE THAM DIGIT ONE
            default: return false;
            }
        }

        int LogicalStringCompare(const char16* string1, int str1size, const char16* string2, int str2size)
        {
            return PlatformAgnostic::UnicodeText::Internal::LogicalStringCompareImpl(string1, str1size, string2, str2size);
        }

        bool IsExternalUnicodeLibraryAvailable()
        {
            return false;
        }

        UnicodeGeneralCategoryClass GetGeneralCategoryClass(codepoint_t ch)
        {
            if (IS_CHAR(ch))
            {
                return UnicodeGeneralCategoryClass::CategoryClassLetter;
            }

            if (IS_NUMBER(ch))
            {
                return UnicodeGeneralCategoryClass::CategoryClassDigit;
            }

            if (ch == 8232) // U+2028
            {
                return UnicodeGeneralCategoryClass::CategoryClassLineSeparator;
            }

            if (ch == 8233) // U+2029
            {
                return UnicodeGeneralCategoryClass::CategoryClassParagraphSeparator;
            }

            if (IsWhitespace(ch))
            {
                return UnicodeGeneralCategoryClass::CategoryClassSpaceSeparator;
            }

            if (ispunct((int)ch))
            {
                return UnicodeGeneralCategoryClass::CategoryClassConnectorPunctuation;
            }

            // for the rest of the ``combining chars``, return CategoryClassSpaceSeparator
            // consider this approach as a fallback mechnanism.
            // since, this function returns CategoryClassLetter for everything else.
            if ((ch >= 836 && ch <= 846) || ch == 810) // U+32A || U+346 <= ch <= U+34E
            {
                return UnicodeGeneralCategoryClass::CategoryClassSpaceSeparator;
            }

            if (ch >= 768 && ch <= 879) // U+300 <= ch <= U+36F (version 1.0)
            {
                return UnicodeGeneralCategoryClass::CategoryClassSpaceSeparator;
            }

            if (ch >= 6832 && ch <= 6911) // U+1AB0 <= ch <= U+1AFF (version 7.0)
            {
                return UnicodeGeneralCategoryClass::CategoryClassSpaceSeparator;
            }

            // default to letter since we don't identify unicode letters
            // xplat-todo: can we do this better?
            return UnicodeGeneralCategoryClass::CategoryClassLetter;
        }
    }
}

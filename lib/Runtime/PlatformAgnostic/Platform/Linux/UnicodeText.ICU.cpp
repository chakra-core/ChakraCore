//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "UnicodeText.h"
#ifdef HAS_REAL_ICU
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/normalizer2.h>
#else
#include <cctype>
#define UErrorCode int
#define U_ZERO_ERROR 0
#define UChar char16
#include <string.h>
#define IS_CHAR(ch) \
        (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')

#define IS_NUMBER(ch) \
        (ch >= '0' && ch <= '9')
#endif

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
#if DBG
        static UErrorCode g_lastErrorCode = U_ZERO_ERROR;
#endif

        static_assert(sizeof(char16) == sizeof(UChar),
            "This implementation depends on ICU char size matching char16's size");

#ifdef HAS_REAL_ICU
        // Helper ICU conversion facilities
        static const Normalizer2* TranslateToICUNormalizer(NormalizationForm normalizationForm)
        {
            UErrorCode errorCode = U_ZERO_ERROR;
            const Normalizer2* normalizer;

            switch (normalizationForm)
            {
                case NormalizationForm::C:
                    normalizer = Normalizer2::getNFCInstance(errorCode);
                    break;
                case NormalizationForm::D:
                    normalizer = Normalizer2::getNFDInstance(errorCode);
                    break;
                case NormalizationForm::KC:
                    normalizer = Normalizer2::getNFKCInstance(errorCode);
                    break;
                case NormalizationForm::KD:
                    normalizer = Normalizer2::getNFKDInstance(errorCode);
                    break;
                default:
                    AssertMsg(false, "Unsupported normalization form");
            };

            AssertMsg(U_SUCCESS(errorCode), (char*) u_errorName(errorCode));

            return normalizer;
        }

        //
        // Check if a UTF16 string is valid according to the UTF16 standard
        // Specifically, check that we don't have any invalid surrogate pairs
        // If the string is valid, we return true.
        // If not, we set invalidIndex to the index of the first invalid char index
        // and return false
        // If the invalid char is a lead surrogate pair, we return its index
        // Otherwise, we treat the char before as the invalid one and return index - 1
        // This function has defined behavior only for null-terminated strings.
        // If the string is not null terminated, the behavior is undefined (likely hang)
        //
        static bool IsUtf16StringValid(const UChar* str, size_t length, size_t* invalidIndex)
        {
            Assert(invalidIndex != nullptr);
            *invalidIndex = -1;

            size_t i = 0;

            for (;;)
            {
                // Iterate through the UTF16-LE string
                // If we are at the end of the null terminated string, return true
                // since the string is valid
                // If not, check if the codepoint we have is a surrogate code unit.
                // If it is, the string is malformed since U16_NEXT would have returned
                // is the full codepoint if both code units in the surrogate pair were present
                UChar32 c;
                U16_NEXT(str, i, length, c);
                if (c == 0)
                {
                    return true;
                }
                if (U_IS_SURROGATE(c))
                {
                    if (U16_IS_LEAD(c))
                    {
                        *invalidIndex = i;
                    }
                    else
                    {
                        Assert(i > 0);
                        *invalidIndex = i - 1;
                    }

                    return false;
                }

                if (i >= length)
                {
                    return true;
                }
            }
        }

        static ApiError TranslateUErrorCode(UErrorCode icuError)
        {
#if DBG
            g_lastErrorCode = icuError;
#endif

            switch (icuError)
            {
                case U_BUFFER_OVERFLOW_ERROR:
                    return ApiError::InsufficientBuffer;
                case U_ILLEGAL_ARGUMENT_ERROR:
                case U_UNSUPPORTED_ERROR:
                    return ApiError::InvalidParameter;
                case U_INVALID_CHAR_FOUND:
                case U_TRUNCATED_CHAR_FOUND:
                case U_ILLEGAL_CHAR_FOUND:
                    return ApiError::InvalidUnicodeText;
                default:
                    return ApiError::UntranslatedError;
            }
        }

        // Linux ICU implementation of platform-agnostic Unicode interface
        int32 NormalizeString(NormalizationForm normalizationForm, const char16* sourceString, uint32 sourceLength, char16* destString, int32 destLength, ApiError* pErrorOut)
        {
            // Assert pointers
            Assert(sourceString != nullptr);
            Assert(destString != nullptr || destLength == 0);

            // This is semantically different than the Win32 NormalizeString API
            // For our usage, we always pass in the length rather than letting Windows
            // calculate the length for us
            Assert(sourceLength > 0);
            Assert(destLength >= 0);

            *pErrorOut = NoError;

            // On Windows, IsNormalizedString returns failure if the string
            // is a malformed utf16 string. Maintain the same behavior here.
            // Note that Windows returns this failure only if the dest buffer
            // is passed in, not in the estimation case
            size_t invalidIndex = 0;
            if (destString != nullptr && !IsUtf16StringValid((const UChar*) sourceString, sourceLength, &invalidIndex))
            {
                *pErrorOut = InvalidUnicodeText;
                return -1 * invalidIndex; // mimicking the behavior of Win32 NormalizeString
            }

            const Normalizer2* normalizer = TranslateToICUNormalizer(normalizationForm);
            Assert(normalizer != nullptr);

            const UnicodeString sourceUniStr((const UChar*) sourceString, sourceLength);

            UErrorCode errorCode = U_ZERO_ERROR;
            const UnicodeString destUniStr = normalizer->normalize(sourceUniStr, errorCode);

            if (U_FAILURE(errorCode))
            {
                *pErrorOut = TranslateUErrorCode(errorCode);
            }

            // xplat-todo: we could possibly make this more efficient and save the buffer copy
            // that destUniStr causes
            int32_t normalizedStringLength = destUniStr.length();
            if (destLength == 0 && normalizedStringLength >= 0)
            {
                *pErrorOut = ApiError::InsufficientBuffer;
                return normalizedStringLength;
            }

            destUniStr.extract((UChar*) destString, destLength, errorCode);
            if (U_FAILURE(errorCode))
            {
                *pErrorOut = TranslateUErrorCode(errorCode);
                return -1;
            }

            return normalizedStringLength;
        }

        bool IsNormalizedString(NormalizationForm normalizationForm, const char16* testString, int32 testStringLength)
        {
            Assert(testString != nullptr);
            UErrorCode errorCode = U_ZERO_ERROR;

            size_t length = static_cast<size_t>(testStringLength);
            if (testStringLength < 0)
            {
                length = u_strlen((const UChar*) testString);
            }

            // On Windows, IsNormalizedString returns failure if the string
            // is a malformed utf16 string. Maintain the same behavior here.
            size_t invalidIndex = 0;
            if (!IsUtf16StringValid((const UChar*) testString, length, &invalidIndex))
            {
                return false;
            }

            const Normalizer2* normalizer = TranslateToICUNormalizer(normalizationForm);
            Assert(normalizer != nullptr);

            const UnicodeString testUniStr((const UChar*) testString, length);
            bool isNormalized = normalizer->isNormalized(testUniStr, errorCode);

            Assert(U_SUCCESS(errorCode));
            return isNormalized;
        }

        bool IsWhitespace(codepoint_t ch)
        {
            return u_isUWhiteSpace(ch) == 1;
        }

        int32 ChangeStringLinguisticCase(CaseFlags caseFlags, const char16* sourceString, uint32 sourceLength, char16* destString, uint32 destLength, ApiError* pErrorOut)
        {
            int32_t resultStringLength = 0;
            UErrorCode errorCode = U_ZERO_ERROR;

            static_assert(sizeof(UChar) == sizeof(char16), "Unexpected char type from ICU, function might have to be updated");
            if (caseFlags == CaseFlagsUpper)
            {
                resultStringLength = u_strToUpper((UChar*) destString, destLength,
                    (UChar*) sourceString, sourceLength, NULL, &errorCode);
            }
            else if (caseFlags == CaseFlagsLower)
            {
                resultStringLength = u_strToLower((UChar*) destString, destLength,
                    (UChar*) sourceString, sourceLength, NULL, &errorCode);
            }
            else
            {
                Assert(false);
            }

            if (U_FAILURE(errorCode) &&
                !(destLength == 0 && errorCode == U_BUFFER_OVERFLOW_ERROR))
            {
                *pErrorOut = TranslateUErrorCode(errorCode);
                return -1;
            }

            // Todo: check for resultStringLength > destLength
            // Return insufficient buffer in that case
            return resultStringLength;
        }
#else

        bool IsWhitespace(codepoint_t ch)
        {
            if (ch > 127)
            {
                return (ch == 160)   || // 0xA0
                       (ch == 12288) || // IDEOGRAPHIC_SPACE
                       (ch == 65279);   // 0xFEFF
            }

            char asc = (char)ch;
            return asc == ' ' || asc == '\n' || asc == '\t' || asc == '\r';
        }

        bool IsNormalizedString(NormalizationForm normalizationForm, const char16* testString, int32 testStringLength) {
            // TODO: implement this
            return true;
        }

#define EMPTY_COPY \
    const int len = (destLength <= sourceLength) ? destLength - 1 : sourceLength; \
    memcpy(destString, sourceString, len * sizeof(char16)); \
    destString[len] = char16(0); \
    *pErrorOut = NoError; \
    return len;

        int32 NormalizeString(NormalizationForm normalizationForm, const char16* sourceString, uint32 sourceLength, char16* destString, int32 destLength, ApiError* pErrorOut)
        {
            // TODO: implement this
            EMPTY_COPY
        }

        int32 ChangeStringLinguisticCase(CaseFlags caseFlags, const char16* sourceString, uint32 sourceLength, char16* destString, uint32 destLength, ApiError* pErrorOut)
        {
            // TODO: implement this
            EMPTY_COPY
        }
#endif

        bool IsIdStart(codepoint_t ch)
        {
#ifdef HAS_REAL_ICU
            if (u_isIDStart(ch))
            {
                return true;
            }
#endif
            // Following codepoints are treated as part of ID_Start
            // for backwards compatibility as per section 2.5 of the Unicode 8 spec
            // See http://www.unicode.org/reports/tr31/tr31-23.html#Backward_Compatibility
            // The exact list is in PropList.txt in the Unicode database
            switch (ch)
            {
            case 0x2118: return true; // SCRIPT CAPITAL P
            case 0x212E: return true; // ESTIMATED SYMBOL
            case 0x309B: return true; // KATAKANA-HIRAGANA VOICED SOUND MARK
            case 0x309C: return true; // KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
            default: return false;
            }
        }

        bool IsIdContinue(codepoint_t ch)
        {
#ifdef HAS_REAL_ICU
            if (u_hasBinaryProperty(ch, UCHAR_ID_CONTINUE) == 1)
            {
                return true;
            }
#endif
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

        uint32 ChangeStringCaseInPlace(CaseFlags caseFlags, char16* stringToChange, uint32 bufferLength)
        {
#ifndef HAS_REAL_ICU
            // ASCII only
            typedef int (*CaseFlipper)(int);
            CaseFlipper flipper;
            if (caseFlags == CaseFlagsUpper)
            {
                flipper = toupper;
            }
            else
            {
                flipper = tolower;
            }
            for(uint32 i = 0; i < bufferLength; i++)
            {
                if (stringToChange[i] > 0 && stringToChange[i] < 127)
                {
                    char ch = (char)stringToChange[i];
                    stringToChange[i] = flipper(ch);
                }
            }
            return bufferLength;
#else
            // Assert pointers
            Assert(stringToChange != nullptr);
            ApiError error = NoError;

            if (bufferLength == 0 || stringToChange == nullptr)
            {
                return 0;
            }

            int32 ret = ChangeStringLinguisticCase(caseFlags, stringToChange, bufferLength, stringToChange, bufferLength, &error);

            // Callers to this function don't expect any errors
            Assert(error == ApiError::NoError);
            Assert(ret > 0);
            return (uint32) ret;
#endif
        }

        int LogicalStringCompare(const char16* string1, const char16* string2)
        {
            return PlatformAgnostic::UnicodeText::Internal::LogicalStringCompareImpl<char16>(string1, string2);
        }

        bool IsExternalUnicodeLibraryAvailable()
        {
#if defined(HAS_REAL_ICU)
            // Since we're using the ICU here, this is trivially true
            return true;
#else
            return false;
#endif
        }

        UnicodeGeneralCategoryClass GetGeneralCategoryClass(codepoint_t ch)
        {
#ifndef HAS_REAL_ICU
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
            // todo-xplat: implement the others ?
#else
            int8_t charType = u_charType(ch);

            if (charType == U_LOWERCASE_LETTER ||
                charType == U_UPPERCASE_LETTER ||
                charType == U_TITLECASE_LETTER ||
                charType == U_MODIFIER_LETTER ||
                charType == U_OTHER_LETTER ||
                charType == U_LETTER_NUMBER)
            {
                return UnicodeGeneralCategoryClass::CategoryClassLetter;
            }

            if (charType == U_DECIMAL_DIGIT_NUMBER)
            {
                return UnicodeGeneralCategoryClass::CategoryClassDigit;
            }

            if (charType == U_LINE_SEPARATOR)
            {
                return UnicodeGeneralCategoryClass::CategoryClassLineSeparator;
            }

            if (charType == U_PARAGRAPH_SEPARATOR)
            {
                return UnicodeGeneralCategoryClass::CategoryClassParagraphSeparator;
            }

            if (charType == U_SPACE_SEPARATOR)
            {
                return UnicodeGeneralCategoryClass::CategoryClassSpaceSeparator;
            }

            if (charType == U_COMBINING_SPACING_MARK)
            {
                return UnicodeGeneralCategoryClass::CategoryClassSpacingCombiningMark;
            }

            if (charType == U_NON_SPACING_MARK)
            {
                return UnicodeGeneralCategoryClass::CategoryClassNonSpacingMark;
            }

            if (charType == U_CONNECTOR_PUNCTUATION)
            {
                return UnicodeGeneralCategoryClass::CategoryClassConnectorPunctuation;
            }
#endif
            return UnicodeGeneralCategoryClass::CategoryClassOther;
        }

        // We actually don't care about the legacy behavior on Linux since no one
        // has a dependency on it. So this actually is different between
        // Windows and Linux
        CharacterClassificationType GetLegacyCharacterClassificationType(char16 character)
        {
#ifdef HAS_REAL_ICU
            auto charTypeMask = U_GET_GC_MASK(character);

            if ((charTypeMask & U_GC_L_MASK) != 0)
            {
                return CharacterClassificationType::Letter;
            }

            if ((charTypeMask & (U_GC_ND_MASK | U_GC_P_MASK)) != 0)
            {
                return CharacterClassificationType::DigitOrPunct;
            }

            // As per http://archives.miloush.net/michkap/archive/2007/06/11/3230072.html
            //  * C1_SPACE corresponds to the Unicode Zs category.
            //  * C1_BLANK corresponds to a hardcoded list thats ill-defined.
            // We'll skip that compatibility here and just check for Zs.
            // We explicitly check for 0xFEFF to satisfy the unit test in es5/Lex_u3.js
            if ((charTypeMask & U_GC_ZS_MASK) != 0 ||
                character == 0xFEFF ||
                character == 0xFFFE)
            {
                return CharacterClassificationType::Whitespace;
            }
#endif
            return CharacterClassificationType::Invalid;
        }
    }
};

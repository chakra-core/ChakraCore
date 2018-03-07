//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "UnicodeText.h"
#include "ChakraICU.h"

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
        static_assert(
            sizeof(char16) == sizeof(UChar),
            "This implementation depends on ICU char size matching char16's size"
        );

        // Returns a UNormalizer2 for a given NormalizationForm
        // IMPORTANT: unorm2_getInstance is used, which *explicitly* states to not call
        // unorm2_close on the returned value!
        static const UNormalizer2 *StaticUNormalizerFactory(NormalizationForm nf)
        {
            static_assert(
                NormalizationForm::C == UNORM2_COMPOSE &&
                NormalizationForm::D == UNORM2_DECOMPOSE &&
                // Deciding between ICU_NORMALIZATION_NFC and ICU_NORMALIZATION_NFKC
                // Depends on ::KC and ::KD being INT_MIN + their non-K forms
                // We can't just make them negative of their non-K forms because UNORM2_COMPOSE == 0
                NormalizationForm::KC == INT_MIN + UNORM2_COMPOSE &&
                NormalizationForm::KD == INT_MIN + UNORM2_DECOMPOSE &&
                (
                    NormalizationForm::Other != NormalizationForm::C &&
                    NormalizationForm::Other != NormalizationForm::D &&
                    NormalizationForm::Other != NormalizationForm::KC &&
                    NormalizationForm::Other != NormalizationForm::KD
                ),
                "Invalid NormalizationForm enum configuration"
            );
            UNormalization2Mode mode = static_cast<UNormalization2Mode>(nf);
            const char *name = ICU_NORMALIZATION_NFC;
            if (nf < 0)
            {
                mode = static_cast<UNormalization2Mode>(nf - INT_MIN);
                name = ICU_NORMALIZATION_NFKC;
            }
            AssertMsg(mode == UNORM2_COMPOSE || mode == UNORM2_DECOMPOSE, "Unsupported normalization mode");

            UErrorCode status = U_ZERO_ERROR;
            const UNormalizer2 *normalizer = unorm2_getInstance(nullptr, name, mode, &status);
            AssertMsg(U_SUCCESS(status), ICU_ERRORMESSAGE(status));
            return normalizer;
        }

        // Check if a UTF16 string is valid according to the UTF16 standard
        // Specifically, check that we don't have any invalid surrogate pairs
        // If the string is valid, we return true.
        // If not, we set invalidIndex to the index of the first invalid char index
        // and return false
        // If the invalid char is a lead surrogate pair, we return its index
        // Otherwise, we treat the char before as the invalid one and return index - 1
        // This function has defined behavior only for null-terminated strings.
        // If the string is not null terminated, the behavior is undefined (likely hang)
        static bool IsUtf16StringValid(const UChar* str, size_t length, int32* invalidIndex)
        {
            Assert(invalidIndex != nullptr);
            *invalidIndex = -1;

            int32 i = 0;
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

// Platform\Windows\UnicodeText.cpp has a more accurate version of this function for Windows
#if !_WIN32
        CharacterClassificationType GetLegacyCharacterClassificationType(char16 character)
        {
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

            return CharacterClassificationType::Invalid;
        }
#endif

        // ICU implementation of platform-agnostic Unicode interface
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
            int32 invalidIndex = 0;
            if (destString != nullptr && !IsUtf16StringValid((const UChar*) sourceString, sourceLength, &invalidIndex))
            {
                *pErrorOut = InvalidUnicodeText;
                return -1 * invalidIndex; // mimicking the behavior of Win32 NormalizeString
            }

            const UNormalizer2 *normalizer = StaticUNormalizerFactory(normalizationForm);
            Assert(normalizer != nullptr);

            UErrorCode status = U_ZERO_ERROR;
            int required = unorm2_normalize(normalizer, reinterpret_cast<const UChar *>(sourceString), sourceLength, reinterpret_cast<UChar *>(destString), destLength, &status);

            if (U_FAILURE(status))
            {
                *pErrorOut = ApiError::InsufficientBuffer;
            }

            return required;
        }

        bool IsNormalizedString(NormalizationForm normalizationForm, const char16* testString, int32 testStringLength)
        {
            Assert(testString != nullptr);
            if (testStringLength < 0)
            {
                testStringLength = u_strlen((const UChar*) testString);
            }

            // On Windows, IsNormalizedString returns failure if the string
            // is a malformed utf16 string. Maintain the same behavior here.
            int32 invalidIndex = 0;
            if (!IsUtf16StringValid((const UChar*) testString, testStringLength, &invalidIndex))
            {
                return false;
            }

            const UNormalizer2 *normalizer = StaticUNormalizerFactory(normalizationForm);
            Assert(normalizer != nullptr);

            UErrorCode status = U_ZERO_ERROR;
            bool isNormalized = unorm2_isNormalized(normalizer, reinterpret_cast<const UChar *>(testString), testStringLength, &status);
            AssertMsg(U_SUCCESS(status), ICU_ERRORMESSAGE(status));

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

        uint32 ChangeStringCaseInPlace(CaseFlags caseFlags, char16* stringToChange, uint32 bufferLength)
        {
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
        }

        bool IsIdStart(codepoint_t ch)
        {
            return u_isIDStart(ch);
        }

        bool IsIdContinue(codepoint_t ch)
        {
            return u_isIDPart(ch);
        }

        int LogicalStringCompare(const char16* string1, const char16* string2)
        {
            return PlatformAgnostic::UnicodeText::Internal::LogicalStringCompareImpl(string1, string2);
        }

        bool IsExternalUnicodeLibraryAvailable()
        {
            return true;
        }

        UnicodeGeneralCategoryClass GetGeneralCategoryClass(codepoint_t ch)
        {
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

            return UnicodeGeneralCategoryClass::CategoryClassOther;
        }
    }
};

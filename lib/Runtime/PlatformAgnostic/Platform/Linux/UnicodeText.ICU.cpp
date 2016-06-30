//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"

#ifndef _WIN32
// Non-Windows Implementation
// TODO: Probably should check for something specific to Linux instead of checking for not _WIN32
#include "UnicodeText.h"

#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/normalizer2.h>

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
#if DBG
        static UErrorCode g_lastErrorCode = U_ZERO_ERROR;
#endif

        // Helper ICU conversion facilities
        static const Normalizer2* TranslateToICUNormalizer(NormalizationForm normalizationForm)
        {
            UErrorCode errorCode = U_ZERO_ERROR;
            const Normalizer2* normalizer;

            switch (normalizationForm)
            {
                case NormalizationForm::C:
                    normalizer = Normalizer2::getNFCInstance(errorCode);
                case NormalizationForm::D:
                    normalizer = Normalizer2::getNFDInstance(errorCode);
                case NormalizationForm::KC:
                    normalizer = Normalizer2::getNFKCInstance(errorCode);
                case NormalizationForm::KD:
                    normalizer = Normalizer2::getNFKDInstance(errorCode);
                default:
                    AssertMsg(false, "Unsupported normalization form");
            };

            AssertMsg(U_SUCCESS(errorCode), (char*) u_errorName(errorCode));

            return normalizer;
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

            const Normalizer2* normalizer = TranslateToICUNormalizer(normalizationForm);
            Assert(normalizer != nullptr);

            const UnicodeString sourceUniStr((const UChar*) sourceString, sourceLength);

            UErrorCode errorCode = U_ZERO_ERROR;
            const UnicodeString destUniStr = normalizer->normalize(sourceUniStr, errorCode);

            if (U_FAILURE(errorCode))
            {
                *pErrorOut = TranslateUErrorCode(errorCode);
            }

            int32_t normalizedStringLength = destUniStr.length();
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

            const Normalizer2* normalizer = TranslateToICUNormalizer(normalizationForm);
            Assert(normalizer != nullptr);

            const UnicodeString testUniStr((const UChar*) testString, testStringLength);
            bool isNormalized = normalizer->isNormalized(testUniStr, errorCode);

            Assert(U_SUCCESS(errorCode));
            return isNormalized;
        }

        // Since we're using the ICU here, this is trivially true
        bool IsExternalUnicodeLibraryAvailable()
        {
            return true;
        }

        bool IsWhitespace(codepoint_t ch)
        {
            return u_isUWhiteSpace(ch) == 1;
        }

        bool IsIdStart(codepoint_t ch)
        {
            return u_isIDStart(ch);
        }
        
        bool IsIdContinue(codepoint_t ch)
        {
            return u_hasBinaryProperty(ch, UCHAR_ID_CONTINUE) == 1;
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

        // We actually don't care about the legacy behavior on Linux since no one
        // has a dependency on it. So this actually is different between
        // Windows and Linux
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
            if ((charTypeMask & U_GC_ZS_MASK) != 0)
            {
                return CharacterClassificationType::Whitespace;
            }
            
            return CharacterClassificationType::Invalid;
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

            if (U_FAILURE(errorCode))
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

        int LogicalStringCompare(const char16* string1, const char16* string2)
        {
            return PlatformAgnostic::UnicodeText::Internal::LogicalStringCompareImpl<char16>(string1, string2);
        }
    };
};
#endif

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"

#ifndef _WIN32
// Non-Windows Implementation
// TODO: Probably should check for something specific to Linux instead of checking for not _WIN32
#include "UnicodeText.h"
#include <unicode/utypes.h>
#include <unicode/unorm2.h>

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
#if DBG
        static UErrorCode g_lastErrorCode = U_ZERO_ERROR;
#endif

        // Helper ICU conversion facilities
        static const UNormalizer* TranslateToICUNormalizer(NormalizationForm normalizationForm)
        {
            UErrorCode errorCode = U_ZERO_ERROR;
            UNormalizer* normalizer = nullptr;

            switch (normalizationForm)
            {
                case NormalizationForm::C:
                    normalizer = unorm2_getNFCInstance(errorCode);
                case NormalizationForm::D:
                    normalizer = unorm2_getNFDInstance(errorCode);
                case NormalizationForm::KC:
                    normalizer = unorm2_getNFKCInstance(errorCode);
                case NormalizationForm::KD:
                    normalizer = unorm2_getNFKDInstance(errorCode);
                default:
                    AssertMsg(false, "Unsupported normalization form")
            };

            AssertMsg(U_SUCCESS(errorCode), u_errorName(errorCode));

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

            const UNormalizer* normalizer = TranslateToICUNormalizer(normalizationForm);
            Assert(normalizer != nullptr);

            UErrorCode errorCode = U_ZERO_ERROR;
            int32 normalizedStringLength = unorm2_normalize(normalizer, (const UChar*)sourceString, sourceLength, (UChar*)destString, destLength, &errorCode);

            if (U_FAILURE(errorCode))
            {
                *pErrorOut = TranslateUErrorCode(errorCode);
            }

            return normalizedStringLength;
        }

        bool IsNormalizedString(NormalizationForm normalizationForm, const char16* testString, int32 testStringLength)
        {
            Assert(testString != nullptr);
            UErrorCode errorCode = U_ZERO_ERROR;

            const UNormalizer* normalizer = TranslateToICUNormalizer(normalizationForm);
            Assert(normalizer != nullptr);
            bool isNormalized = unorm2_isNormalized(normalizer, (const UChar*)testString, testStringLength, &errorCode);

            Assert(U_SUCCESS(errorCode));
            return isNormalized;
        }
    };
};
#endif
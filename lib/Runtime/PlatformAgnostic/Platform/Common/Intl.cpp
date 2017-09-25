//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"

#ifdef INTL_ICU

// REVIEW (doilij): Where are these defined that is safe to include in PlatformAgnostic?
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
#include "Codex/Utf8Codex.h"

#ifndef _WIN32
// REVIEW (doilij): The PCH allegedly defines enough stuff to get AssertMsg to work -- why was compile failing for this file?
#ifdef AssertMsg
#undef AssertMsg
#endif
#define AssertMsg(test, message)
#ifdef AssertOrFailFastMsg
#undef AssertOrFailFastMsg
#endif
#define AssertOrFailFastMsg(test, message)
#endif // !_WIN32

// REVIEW (doilij): copied from lib/Common/CommonPal.h because including that file broke the compile on Linux.
// Only VC compiler support overflow guard
#if defined(__GNUC__) || defined(__clang__)
#define DECLSPEC_GUARD_OVERFLOW
#else // Windows
#define DECLSPEC_GUARD_OVERFLOW __declspec(guard(overflow))
#endif

#include "Common/MathUtil.h"
#include "Core/AllocSizeMath.h"

#include "Intl.h"
#include "IPlatformAgnosticResource.h"

#define U_STATIC_IMPLEMENTATION
#define U_SHOW_CPLUSPLUS_API 0
#pragma warning(push)
#pragma warning(disable:4995) // deprecation warning
#include <unicode/uloc.h>
#include <unicode/numfmt.h>
//#include <unicode/decimfmt.h>
#pragma warning(pop)

namespace PlatformAgnostic
{
namespace Intl
{
    using namespace PlatformAgnostic::Resource;

    bool IsWellFormedLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch)
    {
        bool success = false;
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        char icuLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };
        char icuLangTag[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t parsedLength = 0;
        int32_t forLangTagResultLength = 0;
        int32_t toLangTagResultLength = 0;
        size_t inputLangTagUtf8SizeActual = 0;

        // Allocate memory for the UTF8 output buffer. Need 3 bytes for each (code point + null) to satisfy SAL.
        const size_t inputLangTagUtf8SizeAllocated = AllocSizeMath::Mul(AllocSizeMath::Add(cch, 1), 3);
        unsigned char *inputLangTagUtf8 = new unsigned char[inputLangTagUtf8SizeAllocated];
        PlatformAgnostic::Resource::StringBufferAutoPtr<unsigned char> guard(inputLangTagUtf8);

        if (!inputLangTagUtf8)
        {
            AssertOrFailFastMsg(false, "OOM: failed to allocate inputLangTagUtf8");
            goto LReturn;
        }

        // explicitly zero the allocated array
        for (size_t index = 0; index < inputLangTagUtf8SizeAllocated; ++index)
        {
            inputLangTagUtf8[index] = 0;
        }

        inputLangTagUtf8SizeActual = utf8::EncodeIntoAndNullTerminate(inputLangTagUtf8, languageTag, cch);

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        forLangTagResultLength = uloc_forLanguageTag(reinterpret_cast<const char *>(inputLangTagUtf8),
            icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        success = (forLangTagResultLength > 0) && (parsedLength > 0) &&
            U_SUCCESS(error) && ((size_t)parsedLength == inputLangTagUtf8SizeActual);
        if (!success)
        {
            goto LReturn;
        }

        toLangTagResultLength = uloc_toLanguageTag(icuLocaleId, icuLangTag, ULOC_FULLNAME_CAPACITY, TRUE, &error);
        success = toLangTagResultLength && U_SUCCESS(error);
        if (!success)
        {
            if (error == UErrorCode::U_ILLEGAL_ARGUMENT_ERROR)
            {
                AssertMsg(false, "uloc_toLanguageTag: error U_ILLEGAL_ARGUMENT_ERROR");
            }
            else
            {
                AssertMsg(false, "uloc_toLanguageTag: unexpected error (besides U_ILLEGAL_ARGUMENT_ERROR)");
            }

            goto LReturn;
        }

    LReturn:
        return success;
    }

    HRESULT NormalizeLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _Out_ char16 *normalized, _Out_ size_t *normalizedLength)
    {
        bool success = false;
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        char icuLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };
        char icuLangTag[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t parsedLength = 0;
        int32_t forLangTagResultLength = 0;
        int32_t toLangTagResultLength = 0;

        // Allocate memory for the UTF8 output buffer. Need 3 bytes for each (code point + null) to satisfy SAL.
        const size_t inputLangTagUtf8SizeAllocated = AllocSizeMath::Mul(AllocSizeMath::Add(cch, 1), 3);
        unsigned char *inputLangTagUtf8 = new unsigned char[inputLangTagUtf8SizeAllocated];
        PlatformAgnostic::Resource::StringBufferAutoPtr<unsigned char> guard(inputLangTagUtf8);

        if (!inputLangTagUtf8)
        {
            AssertOrFailFastMsg(false, "OOM: failed to allocate inputLangTagUtf8.");
            goto LReturn;
        }

        // explicitly zero the allocated array
        for (size_t index = 0; index < inputLangTagUtf8SizeAllocated; ++index)
        {
            inputLangTagUtf8[index] = 0;
        }

        utf8::EncodeIntoAndNullTerminate(inputLangTagUtf8, languageTag, cch);

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        forLangTagResultLength = uloc_forLanguageTag(reinterpret_cast<const char *>(inputLangTagUtf8), icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        success = forLangTagResultLength && parsedLength && U_SUCCESS(error);
        if (!success)
        {
            AssertMsg(false, "uloc_forLanguageTag failed");
            goto LReturn;
        }

        // Try to convert icuLocaleId (locale ID version of input locale string) to BCP47 language tag, using strict checks
        toLangTagResultLength = uloc_toLanguageTag(icuLocaleId, icuLangTag, ULOC_FULLNAME_CAPACITY, TRUE, &error);
        success = toLangTagResultLength && U_SUCCESS(error);
        if (!success)
        {
            if (error == UErrorCode::U_ILLEGAL_ARGUMENT_ERROR)
            {
                AssertMsg(false, "uloc_toLanguageTag: error U_ILLEGAL_ARGUMENT_ERROR");
            }
            else
            {
                AssertMsg(false, "uloc_toLanguageTag: unexpected error (besides U_ILLEGAL_ARGUMENT_ERROR)");
            }

            goto LReturn;
        }

        *normalizedLength = utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(normalized,
            reinterpret_cast<const utf8char_t *>(icuLangTag), reinterpret_cast<utf8char_t *>(icuLangTag + toLangTagResultLength), utf8::doDefault);

    LReturn:
        return success ? S_OK : E_INVALIDARG;
    }

    int32_t GetCurrencyFractionDigits(_In_z_ const char16 * currencyCode)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        const UChar *uCurrencyCode = reinterpret_cast<const UChar *>(currencyCode); // UChar, like char16, is guaranteed to be 2 bytes on all platforms.

        // REVIEW (doilij): What does the spec say to do if a currency is not supported? Does that affect this decision?
        int32_t minFracDigits = 2; // Picked a "reasonable" fallback value as a starting value here.

        // Note: The number of fractional digits specified for a currency is not locale-dependent.
        icu::NumberFormat *nf = icu::NumberFormat::createCurrencyInstance(error); // using default locale
        PlatformAgnosticResourceAutoPtr<icu::NumberFormat> guard(nf); // ICU requires that the caller explicitly deletes pointers allocated by ICU (otherwise will leak)

        if (U_FAILURE(error))
        {
#ifdef INTL_ICU_DEBUG
            if (error == UErrorCode::U_MISSING_RESOURCE_ERROR)
            {
                Output::Print(_u("EntryIntl_CurrencyDigits > icu::NumberFormat::createCurrencyInstance(error) > U_MISSING_RESOURCE_ERROR (%d)\n"), error);
            }
            else
            {
                Output::Print(_u("EntryIntl_CurrencyDigits > icu::NumberFormat::createCurrencyInstance(error) > UErrorCode (%d)\n"), error);
            }
#endif
            goto LReturn;
        }

        nf->setCurrency(uCurrencyCode, error);
        if (U_FAILURE(error))
        {
#ifdef INTL_ICU_DEBUG
            if (error == UErrorCode::U_MISSING_RESOURCE_ERROR)
            {
                Output::Print(_u("EntryIntl_CurrencyDigits > nf->setCurrency(uCurrencyCode (%s), error) > U_MISSING_RESOURCE_ERROR (%d)\n"), currencyCode, error);
            }
            else
            {
                Output::Print(_u("EntryIntl_CurrencyDigits > nf->setCurrency(uCurrencyCode (%s), error) > UErrorCode (%d)\n"), currencyCode, error);
            }
#endif
            goto LReturn;
        }

        minFracDigits = nf->getMinimumFractionDigits();

#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_CurrencyDigits > nf->getMinimumFractionDigits() successful > returned (%d)\n"), minFracDigits);
#endif

    LReturn:
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_CurrencyDigits > returning (%d)\n"), minFracDigits);
#endif

        return minFracDigits;
    }

    IPlatformAgnosticResource *CreateNumberFormat()
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        icu::NumberFormat *nf = icu::NumberFormat::createInstance(error);
        IPlatformAgnosticResource *obj = new PlatformAgnosticIntlObject<icu::NumberFormat>(nf);
        AssertMsg(U_SUCCESS(error), "Creating icu::NumberFormat failed");
        return obj;
    }

    // We explicitly declare these specializations of FormatNumber so the compiler creates them
    // because they will be used in another compilation unit,
    // at which time we cannot generate code for specializations of this template.
    template const char16 *FormatNumber<>(IPlatformAgnosticResource *formatter, int32_t val);
    template const char16 *FormatNumber<>(IPlatformAgnosticResource *formatter, double val);

    template <typename T>
    const char16 *FormatNumber(IPlatformAgnosticResource *formatter, T val)
    {
        icu::UnicodeString result;

        auto *formatterHolder = reinterpret_cast<PlatformAgnosticIntlObject<icu::NumberFormat> *>(formatter);
        if (!formatterHolder)
        {
            AssertOrFailFastMsg(formatterHolder, "Formatter did not hold an object of type `FinalizableIntlObject<icu::NumberFormat>`");
            return nullptr;
        }

        icu::NumberFormat *numberFormatter = formatterHolder->GetInstance();
        numberFormatter->format(val, result);
        int32_t length = result.length();
        char16 *ret = new char16[length + 1];
        result.extract(0, length, reinterpret_cast<UChar *>(ret));
        ret[length] = 0;
        return ret;
    }
} // namespace Intl
} // namespace PlatformAgnostic

#endif // INTL_ICU

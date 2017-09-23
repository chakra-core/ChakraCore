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
#define AssertMsg(test, message)
#endif
#ifdef AssertOrFailFastMessage
#undef AssertOrFailFastMessage
#define AssertOrFailFastMessage(test, message)
#endif
#define DECLSPEC_GUARD_OVERFLOW
#endif // !_WIN32

#include "Common/MathUtil.h"
#include "Core/AllocSizeMath.h"

#include "Intl.h"

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

    /*
    // NOTE: cannot return non-pointer instance of a pure virtual class, so move ctor and move asgn will not be applicable.

    // TODO (doilij): Remove if unused
    template <typename T>
    inline PlatformAgnosticIntlObject<T> PlatformAgnosticIntlObject<T>::operator=(PlatformAgnosticIntlObject<T> &&other)
    {
        // Only move if the object is different.
        if (this != &other)
        {
            if (this->intlObject)
            {
                // REVIEW (doilij): Ensure that this statement is true in practice, or delete this assert.
                AssertMsg(false, "FinalizableIntlObject move assignment shouldn't need to delete other->intlObject because it should be initialized to nullptr: check usage.");
                delete this->intlObject;
            }

            this->move(other);
        }

        return *this;
    }
    */

    bool IsWellFormedLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch)
    {
        // Allocate memory for the UTF8 output buffer. Need 3 bytes for each (code point + null) to satisfy SAL.
        const size_t inputLangTagUtf8SizeAllocated = AllocSizeMath::Mul(AllocSizeMath::Add(cch, 1), 3);
        // REVIEW (doilij): not perf critical so I used HeapNewArrayZ to zero-out the allocated array
        unsigned char *inputLangTagUtf8 = new unsigned char[inputLangTagUtf8SizeAllocated];
        if (!inputLangTagUtf8)
        {
            AssertOrFailFastMsg(false, "OOM: HeapNewArrayZ failed to allocate.");
        }
        const size_t inputLangTagUtf8SizeActual = utf8::EncodeIntoAndNullTerminate(inputLangTagUtf8, languageTag, cch);

        bool success = false;
        UErrorCode error = UErrorCode::U_ZERO_ERROR;

        // REVIEW (doilij): should we / do we need to zero these stack-allocated arrays?
        char icuLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };
        char icuLangTag[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t parsedLength = 0;
        int32_t forLangTagResultLength = 0;
        int32_t toLangTagResultLength = 0;

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        forLangTagResultLength = uloc_forLanguageTag(reinterpret_cast<const char *>(inputLangTagUtf8),
            icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        success = (forLangTagResultLength > 0) && (parsedLength > 0) &&
            U_SUCCESS(error) && ((size_t)parsedLength == inputLangTagUtf8SizeActual);
        if (!success)
        {
            goto cleanup;
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

            goto cleanup;
        }

    cleanup:
        delete[] inputLangTagUtf8;
        inputLangTagUtf8 = nullptr;
        return success;
    }

    HRESULT NormalizeLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _Out_ char16 *normalized, _Out_ size_t *normalizedLength)
    {
        // Allocate memory for the UTF8 output buffer. Need 3 bytes for each (code point + null) to satisfy SAL.
        const size_t inputLangTagUtf8SizeAllocated = AllocSizeMath::Mul(AllocSizeMath::Add(cch, 1), 3);
        // REVIEW (doilij): not perf critical so I used HeapNewArrayZ to zero-out the allocated array
        unsigned char *inputLangTagUtf8 = new unsigned char[inputLangTagUtf8SizeAllocated];
        if (!inputLangTagUtf8)
        {
            AssertOrFailFastMsg(false, "OOM: HeapNewArrayZ failed to allocate.");
        }
        const size_t inputLangTagUtf8SizeActual = utf8::EncodeIntoAndNullTerminate(inputLangTagUtf8, languageTag, cch);

        // REVIEW (doilij): should we / do we need to zero these stack-allocated arrays?
        char icuLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };
        char icuLangTag[ULOC_FULLNAME_CAPACITY] = { 0 };
        bool success = false;
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        int32_t parsedLength = 0;
        int32_t forLangTagResultLength = 0;
        int32_t toLangTagResultLength = 0;

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        forLangTagResultLength = uloc_forLanguageTag(reinterpret_cast<const char *>(inputLangTagUtf8), icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        success = forLangTagResultLength && parsedLength && U_SUCCESS(error);
        if (!success)
        {
            AssertMsg(false, "uloc_forLanguageTag failed");
            goto cleanup;
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

            goto cleanup;
        }

        *normalizedLength = utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(normalized,
            reinterpret_cast<const utf8char_t *>(icuLangTag), reinterpret_cast<utf8char_t *>(icuLangTag + toLangTagResultLength), utf8::doDefault);

    cleanup:
        delete[] inputLangTagUtf8;
        inputLangTagUtf8 = nullptr;
        return success ? S_OK : E_INVALIDARG;
    }

    // REVIEW (doilij): Is scriptContext needed as a param here?
    int32_t GetCurrencyFractionDigits(_In_z_ const char16 * currencyCode)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        const UChar *uCurrencyCode = (const UChar *)currencyCode; // UChar, like char16, is guaranteed to be 2 bytes on all platforms.
        int32_t minFracDigits = 2; // REVIEW: fallback value is a good starting value here

                                   // Note: The number of fractional digits specified for a currency is not locale-dependent.
        icu::NumberFormat *nf = icu::NumberFormat::createCurrencyInstance(error); // using default locale
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
        // Since something failed, return "reasonable" default of 2 fractional digits (obviously won't be the case for some currencies like JPY).
        // REVIEW (doilij): What does the spec say to do if a currency is not supported?

#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_CurrencyDigits > returning (%d)\n"), minFracDigits);
#endif

        return minFracDigits;
    }

    IPlatformAgnosticResource *CreateNumberFormat()
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        // REVIEW (doilij): is allocating with `new` here okay?
        icu::NumberFormat *nf = icu::NumberFormat::createInstance(error);
        IPlatformAgnosticResource *obj = new PlatformAgnosticIntlObject<icu::NumberFormat>(nf);
        AssertMsg(U_SUCCESS(error), "Creating icu::NumberFormat failed");
        return obj;
    }

    template const char16 *FormatNumber<>(IPlatformAgnosticResource *formatter, int32_t val);
    template const char16 *FormatNumber<>(IPlatformAgnosticResource *formatter, double val);

    template <typename T>
    const char16 *FormatNumber(IPlatformAgnosticResource *formatter, T val)
    {
        icu::UnicodeString result;

        auto *formatterHolder = (PlatformAgnosticIntlObject<icu::NumberFormat> *)formatter;
        AssertOrFailFastMsg(formatterHolder, "Formatter did not hold an object of type `FinalizableIntlObject<icu::NumberFormat>`");
        icu::NumberFormat *numberFormatter = formatterHolder->GetInstance();

        numberFormatter->format(val, result);
        int32_t length = result.length();
        char16 *ret = new char16[length + 1];
        result.extract(0, length, (UChar *)ret);
        ret[length] = 0;
        return ret;
    }
} // namespace Intl
} // namespace PlatformAgnostic

#endif // INTL_ICU

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"

#ifdef INTL_ICU

#include <stdint.h>
// REVIEW (doilij): Where are these (non-'_t' types) typedef'd that is safe to include in PlatformAgnostic?
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

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
#define U_SHOW_CPLUSPLUS_API 1
#pragma warning(push)
#pragma warning(disable:4995) // deprecation warning
#include <unicode/uloc.h>
#include <unicode/numfmt.h>
#include <unicode/enumset.h>
#include <unicode/decimfmt.h>
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
        if (!inputLangTagUtf8)
        {
            AssertOrFailFastMsg(false, "OOM: failed to allocate inputLangTagUtf8");
            return false;
        }

        StringBufferAutoPtr<unsigned char> guard(inputLangTagUtf8);
        memset(inputLangTagUtf8, 0, inputLangTagUtf8SizeAllocated); // explicitly zero the array

        inputLangTagUtf8SizeActual = utf8::EncodeIntoAndNullTerminate(inputLangTagUtf8, languageTag, cch);

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        // We used utf8 conversion to turn char16* into utf8char_t* (unsigned char *) but uloc_forLanguageTag takes char*
        // LangTags must be 7-bit-ASCII to be valid and any of these chars being "negative" is irrelevant.
        forLangTagResultLength = uloc_forLanguageTag(reinterpret_cast<char *>(inputLangTagUtf8),
            icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        success = (forLangTagResultLength > 0) && (parsedLength > 0) &&
            U_SUCCESS(error) && ((size_t)parsedLength == inputLangTagUtf8SizeActual);
        if (!success)
        {
            return false;
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
        }

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
        if (!inputLangTagUtf8)
        {
            AssertOrFailFastMsg(false, "OOM: failed to allocate inputLangTagUtf8.");
            return E_OUTOFMEMORY;
        }

        StringBufferAutoPtr<unsigned char> guard(inputLangTagUtf8);
        memset(inputLangTagUtf8, 0, inputLangTagUtf8SizeAllocated); // explicitly zero the array

        utf8::EncodeIntoAndNullTerminate(inputLangTagUtf8, languageTag, cch);

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        // We used utf8 conversion to turn char16* into utf8char_t* (unsigned char *) but uloc_forLanguageTag takes char*
        // LangTags must be 7-bit-ASCII to be valid and any of these chars being "negative" is irrelevant.
        forLangTagResultLength = uloc_forLanguageTag(reinterpret_cast<char *>(inputLangTagUtf8),
            icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        success = forLangTagResultLength && parsedLength && U_SUCCESS(error);
        if (!success)
        {
            AssertMsg(false, "uloc_forLanguageTag failed");
            return E_INVALIDARG;
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

            return E_INVALIDARG;
        }

        *normalizedLength = utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(normalized,
            reinterpret_cast<utf8char_t *>(icuLangTag),
            reinterpret_cast<utf8char_t *>(icuLangTag + toLangTagResultLength),
            utf8::doDefault);

        return S_OK;
    }

    int32_t GetCurrencyFractionDigits(_In_z_ const char16 * currencyCode)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;

        // [[ Ctrl-F: UChar_cast_explainer ]]
        // UChar, like char16, is guaranteed to be 2 bytes on all platforms.
        // However, we cannot use static_cast because on Windows:
        // - char16 => WCHAR => wchar_t (native type) -- see Core/CommonTypedefs.h or Codex/Utf8Codex.h
        // - (wchar_t is a native 2-byte type on Windows, but native 4-byte elsewhere)
        // On other platforms:
        // - char16 => char16_t -- see pal/inc/pal_mstypes.h
        // All platforms:
        // - UChar -> char16_t (all platforms)
        const UChar *uCurrencyCode = reinterpret_cast<const UChar *>(currencyCode);

        // REVIEW (doilij): What does the spec say to do if a currency is not supported? Does that affect this decision?
        int32_t minFracDigits = 2; // Picked a "reasonable" fallback value as a starting value here.

        // Note: The number of fractional digits specified for a currency is not locale-dependent.
        icu::NumberFormat *nf = icu::NumberFormat::createCurrencyInstance(error); // using default locale
        AutoPtr<icu::NumberFormat> guard(nf); // ICU requires that the caller explicitly deletes pointers allocated by ICU (otherwise will leak)

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

    template <typename Func>
    HRESULT CreateFormatter(Func function, _In_z_ const char16 *languageTag, _In_ const charcount_t cch, _Out_ IPlatformAgnosticResource **resource)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;

        // Allocate memory for the UTF8 output buffer. Need 3 bytes for each (code point + null) to satisfy SAL.
        const size_t inputLangTagUtf8SizeAllocated = AllocSizeMath::Mul(AllocSizeMath::Add(cch, 1), 3);
        unsigned char *inputLangTagUtf8 = new unsigned char[inputLangTagUtf8SizeAllocated];
        if (!inputLangTagUtf8)
        {
            AssertOrFailFastMsg(false, "OOM: failed to allocate inputLangTagUtf8.");
            return E_OUTOFMEMORY;
        }

        StringBufferAutoPtr<unsigned char> guard(inputLangTagUtf8);
        memset(inputLangTagUtf8, 0, inputLangTagUtf8SizeAllocated); // explicitly zero the array

        utf8::EncodeIntoAndNullTerminate(inputLangTagUtf8, languageTag, cch);

        char *localeName = reinterpret_cast<char *>(inputLangTagUtf8);
        icu::Locale locale = icu::Locale::createFromName(localeName);

        icu::NumberFormat *nf = function(locale, error);

        if (U_FAILURE(error))
        {
            AssertMsg(false, "Creating icu::NumberFormat failed");
            return E_INVALIDARG;
        }

        IPlatformAgnosticResource *formatterResource = new PlatformAgnosticIntlObject<icu::NumberFormat>(nf);
        if (!formatterResource)
        {
            return E_OUTOFMEMORY;
        }

        *resource = formatterResource;
        return S_OK;
    }

    HRESULT CreateNumberFormatter(_In_z_ const char16 *languageTag, _In_ const charcount_t cch, _Out_ IPlatformAgnosticResource **resource)
    {
        return CreateFormatter(
            [](icu::Locale &locale, UErrorCode &error) { return icu::NumberFormat::createInstance(locale, error); },
            languageTag, cch, resource);
    }

    HRESULT CreatePercentFormatter(_In_z_ const char16 *languageTag, _In_ const charcount_t cch, _Out_ IPlatformAgnosticResource **resource)
    {
        return CreateFormatter(
            [](icu::Locale &locale, UErrorCode &error) { return icu::NumberFormat::createPercentInstance(locale, error); },
            languageTag, cch, resource);
    }

    HRESULT CreateCurrencyFormatter(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _In_z_ const char16 *currencyCode, _In_ const NumberFormatCurrencyDisplay currencyDisplay, _Out_ IPlatformAgnosticResource **resource)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        unsigned char *inputLangTagUtf8 = nullptr;

        // Allocate memory for the UTF8 output buffer. Need 3 bytes for each (code point + null) to satisfy SAL.
        const size_t inputLangTagUtf8SizeAllocated = AllocSizeMath::Mul(AllocSizeMath::Add(cch, 1), 3);
        inputLangTagUtf8 = new unsigned char[inputLangTagUtf8SizeAllocated];
        if (!inputLangTagUtf8)
        {
            AssertOrFailFastMsg(false, "OOM: failed to allocate inputLangTagUtf8.");
            return E_OUTOFMEMORY;
        }

        StringBufferAutoPtr<unsigned char> guard(inputLangTagUtf8);
        memset(inputLangTagUtf8, 0, inputLangTagUtf8SizeAllocated); // explicitly zero the array

        utf8::EncodeIntoAndNullTerminate(inputLangTagUtf8, languageTag, cch);

        char *localeName = reinterpret_cast<char *>(inputLangTagUtf8);
        icu::Locale locale = icu::Locale::createFromName(localeName);

        icu::NumberFormat *nf = nullptr;
        if (currencyDisplay == NumberFormatCurrencyDisplay::SYMBOL || currencyDisplay >= NumberFormatCurrencyDisplay::MAX)
        {
            // 0 (or default) => use symbol (e.g. "$" or "US$")
            nf = icu::NumberFormat::createCurrencyInstance(locale, error);
            if (U_FAILURE(error))
            {
                AssertMsg(false, "icu::NumberFormat::createCurrencyInstance failed");
                return E_INVALIDARG;
            }

            nf->setCurrency(reinterpret_cast<const UChar *>(currencyCode), error); // Ctrl-F: UChar_cast_explainer
            if (U_FAILURE(error))
            {
                AssertMsg(false, "Failed to set currency on icu::NumberFormat");
                return E_INVALIDARG;
            }
        }
        else if (currencyDisplay == NumberFormatCurrencyDisplay::CODE || currencyDisplay == NumberFormatCurrencyDisplay::NAME)
        {
            // CODE e.g. "USD 42.00"; NAME (e.g. "42.00 US dollars")
            // In both cases we need to be able to format in decimal and add the code or name afterwards.
            // We will decide how to do this when platform.formatNumber is invoked (based on currencyDisplay again).
            // TODO (future) (doilij): How do we handle which side of the number to put the code or name? Can ICU do this? It doesn't seem clear how at the moment.

            nf = icu::NumberFormat::createInstance(locale, error);
            if (U_FAILURE(error))
            {
                AssertMsg(false, "icu::NumberFormat::createInstance failed");
                return E_INVALIDARG;
            }
        }

        if (U_FAILURE(error))
        {
            AssertMsg(false, "Creating icu::NumberFormat failed");
            return E_INVALIDARG;
        }

        IPlatformAgnosticResource *formatterResource = new PlatformAgnosticIntlObject<icu::NumberFormat>(nf);
        if (!formatterResource)
        {
            return E_OUTOFMEMORY;
        }

        *resource = formatterResource;
        return S_OK;
    }

    HRESULT SetNumberFormatSignificantDigits(IPlatformAgnosticResource *resource, const uint16 minSigDigits, const uint16 maxSigDigits)
    {
        // We know what actual type we stored in the IPlatformAgnosticResource*, so cast to it.
        auto *numFormatResource = reinterpret_cast<PlatformAgnosticIntlObject<icu::NumberFormat> *>(resource);
        icu::NumberFormat *nf = numFormatResource->GetInstance();
        // REVIEW (doilij): Without RTTI support, we can't do dynamic_cast,
        // which is the preferred way to know whether it was indeed created as a DecimalFormat,
        // which is not something the API will indicate to us directly (create returns a NumberFormat which may or may not be a DecimalFormat)
        icu::DecimalFormat *df = reinterpret_cast<icu::DecimalFormat *>(nf);
        df->setMinimumSignificantDigits(minSigDigits);
        df->setMaximumSignificantDigits(maxSigDigits);
        return S_OK;
    }

    HRESULT SetNumberFormatIntFracDigits(IPlatformAgnosticResource *resource, const uint16 minFracDigits, const uint16 maxFracDigits, const uint16 minIntDigits)
    {
        // We know what actual type we stored in the IPlatformAgnosticResource*, so cast to it.
        auto *numFormatResource = reinterpret_cast<PlatformAgnosticIntlObject<icu::NumberFormat> *>(resource);
        icu::NumberFormat *nf = numFormatResource->GetInstance();
        nf->setMinimumIntegerDigits(minIntDigits);
        nf->setMinimumFractionDigits(minFracDigits);
        nf->setMaximumFractionDigits(maxFracDigits);
        return S_OK;
    }

    void SetGroupingUsed(IPlatformAgnosticResource *resource, const bool isGroupingUsed)
    {
        icu::NumberFormat *nf = reinterpret_cast<PlatformAgnosticIntlObject<icu::NumberFormat> *>(resource)->GetInstance();
        nf->setGroupingUsed(isGroupingUsed);
        return;
    }

    // We explicitly declare these specializations of FormatNumber so the compiler creates them
    // because they will be used in another compilation unit,
    // at which time we cannot generate code for specializations of this template.
    template const char16 *FormatNumber<>(IPlatformAgnosticResource *formatter, const int32_t val, const NumberFormatStyle formatterToUse, const NumberFormatCurrencyDisplay currencyDisplay, const char16 *currencyCode);
    template const char16 *FormatNumber<>(IPlatformAgnosticResource *formatter, const double val, const NumberFormatStyle formatterToUse, const NumberFormatCurrencyDisplay currencyDisplay, const char16 *currencyCode);

    template <typename T>
    const char16 *FormatNumber(IPlatformAgnosticResource *formatter, const T val, const NumberFormatStyle formatterToUse, const NumberFormatCurrencyDisplay currencyDisplay, const char16 *currencyCode)
    {
        icu::UnicodeString result;

        auto *formatterHolder = reinterpret_cast<PlatformAgnosticIntlObject<icu::NumberFormat> *>(formatter);
        if (!formatterHolder)
        {
            AssertOrFailFastMsg(formatterHolder, "Formatter did not hold an object of type `FinalizableIntlObject<icu::NumberFormat>`");
            return nullptr;
        }

        icu::NumberFormat *numberFormatter = formatterHolder->GetInstance();

        if (formatterToUse == NumberFormatStyle::DECIMAL || formatterToUse == NumberFormatStyle::PERCENT)
        {
            // we already created the formatter to format things according to the above options, so nothing else to do
            numberFormatter->format(val, result);
        }
        else if (formatterToUse == NumberFormatStyle::CURRENCY)
        {
            UErrorCode error = UErrorCode::U_ZERO_ERROR;

            const UChar *uCurrencyCode = reinterpret_cast<const UChar *>(currencyCode); // Ctrl-F: UChar_cast_explainer
            const char *localeName = numberFormatter->getLocale(ULocDataLocaleType::ULOC_ACTUAL_LOCALE, error).getName();

            if (U_FAILURE(error))
            {
                AssertMsg(false, "numberFormatter->getLocale failed");
            }

            UBool isChoiceFormat = false;
            int32_t currencyNameLen = 0;

            if (currencyDisplay == NumberFormatCurrencyDisplay::SYMBOL || currencyDisplay >= NumberFormatCurrencyDisplay::MAX) // (e.g. "$42.00")
            {
                // the formatter is set up to render the symbol by default
                numberFormatter->format(val, result);
            }
            else if (currencyDisplay == NumberFormatCurrencyDisplay::CODE) // (e.g. "USD 42.00")
            {
                result.append(uCurrencyCode);
                result.append("\u00a0"); // NON-BREAKING SPACE
                numberFormatter->format(val, result);
            }
            else if (currencyDisplay == NumberFormatCurrencyDisplay::NAME) // (e.g. "US dollars")
            {
                const char *pluralCount = nullptr; // REVIEW (doilij): is this okay? It's not entirely clear from the documentation whether this is an optional parameter.
                const UChar *currencyLongName = ucurr_getPluralName(uCurrencyCode, localeName, &isChoiceFormat, pluralCount, &currencyNameLen, &error);

                if (U_FAILURE(error))
                {
                    AssertMsg(false, "Failed to format");
                }

                numberFormatter->format(val, result);
                result.append(" ");
                result.append(currencyLongName);
            }
        }
        else
        {
            AssertMsg(false, "No other value of NumberFormatStyle is allowed");
        }

        int32_t length = result.length();
        char16 *ret = new char16[length + 1];
        result.extract(0, length, reinterpret_cast<UChar *>(ret)); // Ctrl-F: UChar_cast_explainer
        ret[length] = 0;
        return ret;
    }

    bool ResolveLocaleLookup(_In_z_ const char16 *locale, _Out_ char16 *resolved)
    {
        // TODO (doilij): implement ResolveLocaleLookup
        resolved[0] = '\0';
        return false;
    }

    bool ResolveLocaleBestFit(_In_z_ const char16 *locale, _Out_ char16 *resolved)
    {
        // Note: the "best fit" matcher is implementation-defined, so it is okay to return the same result as ResolveLocaleLookup.
        // TODO (doilij): implement a better "best fit" matcher
        return ResolveLocaleLookup(locale, resolved);
    }

    size_t GetUserDefaultLanguageTag(_Out_ char16* langtag, _In_ size_t cchLangtag)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        char bcp47[ULOC_FULLNAME_CAPACITY] = { 0 };
        char defaultLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };

        int32_t written = uloc_getName(nullptr, defaultLocaleId, ULOC_FULLNAME_CAPACITY, &error);
        if (U_SUCCESS(error) && written > 0 && written < ULOC_FULLNAME_CAPACITY)
        {
            defaultLocaleId[written] = 0;
            error = UErrorCode::U_ZERO_ERROR;
        }
        else
        {
            AssertMsg(false, "uloc_getName: unexpected error getting default localeId");
            return 0;
        }

        written = uloc_toLanguageTag(defaultLocaleId, bcp47, ULOC_FULLNAME_CAPACITY, true, &error);
        if (U_FAILURE(error) || written <= 0)
        {
            AssertMsg(false, "uloc_toLanguageTag: unexpected error getting default langtag");
            return 0;
        }

        if (written < cchLangtag)
        {
            return utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(langtag, reinterpret_cast<const utf8char_t *>(bcp47), reinterpret_cast<utf8char_t *>(bcp47 + written));
        }
        else
        {
            AssertMsg(false, "User default language tag is larger than the provided buffer");
            return 0;
        }
    }
} // namespace Intl
} // namespace PlatformAgnostic

#endif // INTL_ICU

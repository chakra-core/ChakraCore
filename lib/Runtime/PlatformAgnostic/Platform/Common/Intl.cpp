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

#include "Codex/Utf8Helper.h"

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

#include "Intl.h"

#ifndef U_STATIC_IMPLEMENTATION
#define U_STATIC_IMPLEMENTATION
#endif

#ifdef U_SHOW_CPLUSPLUS_API
#undef U_SHOW_CPLUSPLUS_API
#endif
#define U_SHOW_CPLUSPLUS_API 1

// pal/inc/rt/sal.h defines __out, which is used as a variable in libstdc++,
// which gets included by ICU
#ifdef __out
#undef __out
#endif

#pragma warning(push)
#pragma warning(disable:4995) // deprecation warning
#include <unicode/uloc.h>
#include <unicode/numfmt.h>
#include <unicode/enumset.h>
#include <unicode/decimfmt.h>
#pragma warning(pop)

#define __out

#include "CommonDefines.h"
#ifndef INTL_ICU_DEBUG
#define INTL_ICU_DEBUG 0
#endif

// Forward declare output printer
class Output {
public:
    static size_t __cdecl Print(const char16 *form, ...);
};

#define ICU_ERROR_FMT _u("INTL: %S failed with error code %S\n")
#define ICU_EXPR_FMT _u("INTL: %S failed expression check %S\n")

#define ICU_RETURN(e, expr, r)                                                \
    do                                                                        \
    {                                                                         \
        if (U_FAILURE(e))                                                     \
        {                                                                     \
            if (INTL_ICU_DEBUG)                                               \
            {                                                                 \
                Output::Print(ICU_ERROR_FMT, __func__, u_errorName(error));   \
            }                                                                 \
            return r;                                                         \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            if (INTL_ICU_DEBUG)                                               \
            {                                                                 \
                Output::Print(ICU_EXPR_FMT, __func__, #expr);                 \
            }                                                                 \
            return r;                                                         \
        }                                                                     \
    } while (false)

#define ICU_ASSERT(e, expr)                                                   \
    do                                                                        \
    {                                                                         \
        if (U_FAILURE(e))                                                     \
        {                                                                     \
            if (INTL_ICU_DEBUG)                                               \
            {                                                                 \
                Output::Print(ICU_ERROR_FMT, __func__, u_errorName(error));   \
            }                                                                 \
            AssertMsg(false, u_errorName(e));                                 \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            if (INTL_ICU_DEBUG)                                               \
            {                                                                 \
                Output::Print(ICU_EXPR_FMT, __func__, #expr);                 \
            }                                                                 \
            Assert(expr);                                                     \
        }                                                                     \
    } while (false)

#define ICU_ASSERT_RETURN(e, expr, r)                                         \
    do                                                                        \
    {                                                                         \
        if (U_FAILURE(e))                                                     \
        {                                                                     \
            if (INTL_ICU_DEBUG)                                               \
            {                                                                 \
                Output::Print(ICU_ERROR_FMT, __func__, u_errorName(error));   \
            }                                                                 \
            AssertMsg(false, u_errorName(e));                                 \
            return r;                                                         \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            if (INTL_ICU_DEBUG)                                               \
            {                                                                 \
                Output::Print(ICU_EXPR_FMT, __func__, #expr);                 \
            }                                                                 \
            Assert(expr);                                                     \
            return r;                                                         \
        }                                                                     \
    } while (false)

namespace PlatformAgnostic
{
namespace Intl
{
    using namespace PlatformAgnostic::Resource;

    bool IsWellFormedLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        char icuLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };
        char icuLangTag[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t parsedLength = 0;
        utf8::WideToNarrow languageTagUtf8(languageTag);

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        // We used utf8 conversion to turn char16* into utf8char_t* (unsigned char *) but uloc_forLanguageTag takes char*
        // LangTags must be 7-bit-ASCII to be valid and any of these chars being "negative" is irrelevant.
        int32_t forLangTagResultLength = uloc_forLanguageTag(languageTagUtf8, icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        ICU_RETURN(error, forLangTagResultLength > 0 && parsedLength > 0 && ((size_t) parsedLength) == languageTagUtf8.Length(), false);

        int32_t toLangTagResultLength = uloc_toLanguageTag(icuLocaleId, icuLangTag, ULOC_FULLNAME_CAPACITY, TRUE, &error);
        ICU_RETURN(error, toLangTagResultLength > 0, false);

        return true;
    }

    HRESULT NormalizeLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _Out_ char16 *normalized, _Out_ size_t *normalizedLength)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        char icuLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };
        char icuLangTag[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t parsedLength = 0;
        utf8::WideToNarrow languageTagUtf8(languageTag);

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        // We used utf8 conversion to turn char16* into utf8char_t* (unsigned char *) but uloc_forLanguageTag takes char*
        // LangTags must be 7-bit-ASCII to be valid and any of these chars being "negative" is irrelevant.
        int32_t forLangTagResultLength = uloc_forLanguageTag(languageTagUtf8, icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        ICU_ASSERT_RETURN(error, forLangTagResultLength > 0 && parsedLength > 0, E_INVALIDARG);

        // Try to convert icuLocaleId (locale ID version of input locale string) to BCP47 language tag, using strict checks
        int32_t toLangTagResultLength = uloc_toLanguageTag(icuLocaleId, icuLangTag, ULOC_FULLNAME_CAPACITY, true, &error);
        ICU_ASSERT_RETURN(error, toLangTagResultLength > 0, E_INVALIDARG);

        *normalizedLength = utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(
            normalized,
            reinterpret_cast<LPCUTF8>(icuLangTag),
            reinterpret_cast<LPCUTF8>(icuLangTag + toLangTagResultLength)
        );

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
        const int32_t fallback = 2; // Picked a "reasonable" fallback value as a starting value here.

        // Note: The number of fractional digits specified for a currency is not locale-dependent.
        icu::NumberFormat *nf = icu::NumberFormat::createCurrencyInstance(error); // use default locale
        AutoPtr<icu::NumberFormat> guard(nf);
        ICU_RETURN(error, true, fallback);

        nf->setCurrency(uCurrencyCode, error);
        ICU_RETURN(error, true, fallback);

        return nf->getMinimumFractionDigits();
    }

    template <typename Func>
    HRESULT CreateFormatter(Func formatterFactory, _In_z_ const char16 *languageTag, _In_ const charcount_t cch, _Out_ IPlatformAgnosticResource **resource)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        utf8::WideToNarrow languageTagUtf8(languageTag);

        // TODO(jahorto): Should this createCanonical instead?
        icu::Locale locale = icu::Locale::createFromName(languageTagUtf8);
        if (locale.isBogus())
        {
            return E_INVALIDARG;
        }

        icu::NumberFormat *nf = formatterFactory(locale, error);
        ICU_ASSERT_RETURN(error, true, E_INVALIDARG);

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
        return CreateFormatter(
            [&currencyDisplay, currencyCode](icu::Locale &locale, UErrorCode &error) -> icu::NumberFormat*
            {
                icu::NumberFormat *nf = nullptr;
                if (currencyDisplay == NumberFormatCurrencyDisplay::SYMBOL || currencyDisplay >= NumberFormatCurrencyDisplay::MAX)
                {
                    // 0 (or default) => use symbol (e.g. "$" or "US$")
                    nf = icu::NumberFormat::createCurrencyInstance(locale, error);
                    ICU_ASSERT_RETURN(error, true, nullptr);

                    nf->setCurrency(reinterpret_cast<const UChar *>(currencyCode), error); // Ctrl-F: UChar_cast_explainer
                    ICU_ASSERT_RETURN(error, true, nullptr);
                }
                else if (currencyDisplay == NumberFormatCurrencyDisplay::CODE || currencyDisplay == NumberFormatCurrencyDisplay::NAME)
                {
                    // CODE e.g. "USD 42.00"; NAME (e.g. "42.00 US dollars")
                    // In both cases we need to be able to format in decimal and add the code or name afterwards.
                    // We will decide how to do this when platform.formatNumber is invoked (based on currencyDisplay again).
                    // TODO(doilij): How do we handle which side of the number to put the code or name? Can ICU do this? It doesn't seem clear how at the moment.
                    nf = icu::NumberFormat::createInstance(locale, error);
                    ICU_ASSERT_RETURN(error, true, nullptr);
                }

                return nf;
            },
            languageTag,
            cch,
            resource
        );
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

    // We explicitly declare these specializations of FormatNumber so the compiler creates them
    // because they will be used in another compilation unit,
    // at which time we cannot generate code for specializations of this template.
    template const char16 *FormatNumber<>(IPlatformAgnosticResource *formatter, const int32_t val, const NumberFormatStyle formatterToUse,
        const NumberFormatCurrencyDisplay currencyDisplay, const char16 *currencyCode);
    template const char16 *FormatNumber<>(IPlatformAgnosticResource *formatter, const double val, const NumberFormatStyle formatterToUse,
        const NumberFormatCurrencyDisplay currencyDisplay, const char16 *currencyCode);

    template <typename T>
    const char16 *FormatNumber(IPlatformAgnosticResource *formatter, const T val, const NumberFormatStyle formatterToUse,
        const NumberFormatCurrencyDisplay currencyDisplay, const char16 *currencyCode)
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
            ICU_ASSERT(error, true);

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
                const UChar *currencyLongName = ucurr_getPluralName(uCurrencyCode, localeName, &isChoiceFormat, nullptr, &currencyNameLen, &error);
                ICU_ASSERT(error, true);

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
        ICU_ASSERT_RETURN(error, written > 0 && written < ULOC_FULLNAME_CAPACITY, 0);

        defaultLocaleId[written] = 0;
        error = UErrorCode::U_ZERO_ERROR;

        written = uloc_toLanguageTag(defaultLocaleId, bcp47, ULOC_FULLNAME_CAPACITY, true, &error);
        ICU_ASSERT_RETURN(error, written > 0 && written < cchLangtag, 0);

        return utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(
            langtag,
            reinterpret_cast<LPCUTF8>(bcp47),
            reinterpret_cast<LPCUTF8>(bcp47 + written)
        );
    }
} // namespace Intl
} // namespace PlatformAgnostic

#endif // INTL_ICU

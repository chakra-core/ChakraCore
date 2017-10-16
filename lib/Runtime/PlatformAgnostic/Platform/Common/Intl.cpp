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

#include "CommonDefines.h" // INTL_ICU_DEBUG

#if defined(__GNUC__) || defined(__clang__)
// Output gets included through some other path on Windows
class Output
{
public:
    static size_t __cdecl Print(const char16 *form, ...);
};
#endif

#define ICU_ERROR_FMT _u("INTL: %S failed with error code %S\n")
#define ICU_EXPR_FMT _u("INTL: %S failed expression check %S\n")

#ifdef INTL_ICU_DEBUG
#define ICU_DEBUG_PRINT(fmt, msg) Output::Print(fmt, __func__, (msg))
#else
#define ICU_DEBUG_PRINT(fmt, msg)
#endif

#define ICU_RETURN(e, expr, r)                                                \
    do                                                                        \
    {                                                                         \
        if (U_FAILURE(e))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_ERROR_FMT, u_errorName(e));                   \
            return r;                                                         \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_EXPR_FMT, u_errorName(e));                    \
            return r;                                                         \
        }                                                                     \
    } while (false)

#define ICU_ASSERT(e, expr)                                                   \
    do                                                                        \
    {                                                                         \
        if (U_FAILURE(e))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_ERROR_FMT, u_errorName(e));                   \
            AssertMsg(false, u_errorName(e));                                 \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_EXPR_FMT, u_errorName(e));                    \
            Assert(expr);                                                     \
        }                                                                     \
    } while (false)

#define ICU_ASSERT_RETURN(e, expr, r)                                         \
    do                                                                        \
    {                                                                         \
        if (U_FAILURE(e))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_ERROR_FMT, u_errorName(e));                   \
            AssertMsg(false, u_errorName(e));                                 \
            return r;                                                         \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_EXPR_FMT, u_errorName(e));                    \
            Assert(expr);                                                     \
            return r;                                                         \
        }                                                                     \
    } while (false)

#define CHECK_UTF_CONVERSION(str, ret)                                        \
    do                                                                        \
    {                                                                         \
        if (str == nullptr)                                                   \
        {                                                                     \
            AssertOrFailFastMsg(false, "OOM: failed to allocate string buffer"); \
            return ret;                                                       \
        }                                                                     \
    } while (false)

#define UNWRAP_RESOURCE(resource, innerType) reinterpret_cast<PlatformAgnosticIntlObject<innerType> *>(resource)->GetInstance();

namespace PlatformAgnostic
{
namespace Intl
{
    using namespace PlatformAgnostic::Resource;

    // This file uses dynamic_cast and RTTI
    // While most of ChakraCore has left this feature disabled for a number of reasons, ICU uses it heavily internally
    // For instance, in creating NumberFormats, the documentation recommends the use of the NumberFormat::create* factory methods
    // However, to use attributes and significant digits, you must be working with a DecimalFormat
    // NumberFormat offers no indicator fields or convenience methods to convert to a DecimalFormat, as ICU expects RTTI to be enabled
    // As such, RTTI is enabled only in the PlatformAgnostic project and only if IntlICU=true

    // lots of logic below requires utf8char_t ~= char and UChar ~= char16
    static_assert(sizeof(utf8char_t) == sizeof(char), "ICU-based Intl logic assumes that utf8char_t is compatible with char");

    // [[ Ctrl-F: UChar_cast_explainer ]]
    // UChar, like char16, is guaranteed to be 2 bytes on all platforms.
    // However, we cannot use static_cast because on Windows:
    // - char16 => WCHAR => wchar_t (native type) -- see Core/CommonTypedefs.h or Codex/Utf8Codex.h
    // - (wchar_t is a native 2-byte type on Windows, but native 4-byte elsewhere)
    // On other platforms:
    // - char16 => char16_t -- see pal/inc/pal_mstypes.h
    // All platforms:
    // - UChar -> char16_t (all platforms)
    static_assert(sizeof(UChar) == sizeof(char16), "ICU-based Intl logic assumes that UChar is compatible with char16");

    // This function duplicates the Utf8Helper.h classes/methods
    // See https://github.com/Microsoft/ChakraCore/pull/3913 for a (failed) attempt to use those directly
    // Use CHECK_UTF_CONVERSION macro at the callsite to check the return value
    // The caller is responsible for delete[]ing the memory returned here
    // TODO(jahorto): As long as this function exists, it _must_ be kept in sync with utf8::WideStringToNarrow (Utf8Helper.h)
    // TODO(jahorto 10/13/2017): This function should not exist for long, and should eventually be replaced with official utf8:: code
    static utf8char_t *Utf16ToUtf8(_In_ const char16 *src, _In_ const charcount_t srcLength, _Out_ size_t *destLength)
    {
        // Allocate memory for the UTF8 output buffer. Need 3 bytes for each (code point + null) to satisfy SAL.
        size_t allocSize = AllocSizeMath::Mul(AllocSizeMath::Add(srcLength, 1), 3);
        utf8char_t *dest = new utf8char_t[allocSize];
        if (!dest)
        {
            // this check has to happen in the caller regardless, so asserting here doesn't do much good
            return nullptr;
        }

        *destLength = utf8::EncodeIntoAndNullTerminate(dest, src, srcLength);
        return dest;
    }

    bool IsWellFormedLanguageTag(_In_z_ const char16 *langtag16, _In_ const charcount_t cch)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        char icuLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };
        char icuLangTag[ULOC_FULLNAME_CAPACITY] = { 0 };
        
        size_t langtag8Length = 0;
        const utf8char_t *langtag8 = Utf16ToUtf8(langtag16, cch, &langtag8Length);
        StringBufferAutoPtr<utf8char_t> guard(langtag8);
        CHECK_UTF_CONVERSION(langtag8, false);

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        // We used utf8 conversion to turn char16* into utf8char_t* (unsigned char *) but uloc_forLanguageTag takes char*
        // LangTags must be 7-bit-ASCII to be valid and any of these chars being "negative" is irrelevant.
        int32_t parsedLength = 0;
        int32_t forLangTagResultLength = uloc_forLanguageTag(reinterpret_cast<const char *>(langtag8),
            icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        ICU_RETURN(error, forLangTagResultLength > 0 && parsedLength > 0 && ((size_t) parsedLength) == langtag8Length, false);

        int32_t toLangTagResultLength = uloc_toLanguageTag(icuLocaleId, icuLangTag, ULOC_FULLNAME_CAPACITY, true, &error);
        ICU_RETURN(error, toLangTagResultLength > 0, false);

        return true;
    }

    HRESULT NormalizeLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _Out_ char16 *normalized, _Out_ size_t *normalizedLength)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        char icuLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };
        char icuLangTag[ULOC_FULLNAME_CAPACITY] = { 0 };

        size_t langtag8Length = 0;
        const utf8char_t *langtag8 = Utf16ToUtf8(languageTag, cch, &langtag8Length);
        StringBufferAutoPtr<utf8char_t> guard(langtag8);
        CHECK_UTF_CONVERSION(langtag8, E_OUTOFMEMORY);

        // Convert input language tag to a locale ID for use in uloc_toLanguageTag API.
        // We used utf8 conversion to turn char16* into utf8char_t* (unsigned char *) but uloc_forLanguageTag takes char*
        // LangTags must be 7-bit-ASCII to be valid and any of these chars being "negative" is irrelevant.
        int32_t parsedLength = 0;
        int32_t forLangTagResultLength = uloc_forLanguageTag(reinterpret_cast<const char *>(langtag8),
            icuLocaleId, ULOC_FULLNAME_CAPACITY, &parsedLength, &error);
        ICU_ASSERT_RETURN(error, forLangTagResultLength > 0 && parsedLength > 0, E_INVALIDARG);

        // Try to convert icuLocaleId (locale ID version of input locale string) to BCP47 language tag, using strict checks
        int32_t toLangTagResultLength = uloc_toLanguageTag(icuLocaleId, icuLangTag, ULOC_FULLNAME_CAPACITY, true, &error);
        ICU_ASSERT_RETURN(error, toLangTagResultLength > 0, E_INVALIDARG);

        *normalizedLength = utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(
            normalized,
            reinterpret_cast<utf8char_t *>(icuLangTag),
            reinterpret_cast<utf8char_t *>(icuLangTag + toLangTagResultLength)
        );

        return S_OK;
    }

    int32_t GetCurrencyFractionDigits(_In_z_ const char16 * currencyCode)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
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

        size_t langtag8Length = 0;
        const utf8char_t *langtag8 = Utf16ToUtf8(languageTag, cch, &langtag8Length);
        StringBufferAutoPtr<utf8char_t> guard(langtag8);
        CHECK_UTF_CONVERSION(langtag8, E_OUTOFMEMORY);

        // TODO(jahorto): Should this createCanonical instead?
        icu::Locale locale = icu::Locale::createFromName(reinterpret_cast<const char*>(langtag8));
        if (locale.isBogus())
        {
            return E_INVALIDARG;
        }

        icu::NumberFormat *nf = formatterFactory(locale, error);
        ICU_ASSERT_RETURN(error, true, E_INVALIDARG);

        // If the formatter produced a DecimalFormat, force it to round up
        icu::DecimalFormat *df = dynamic_cast<icu::DecimalFormat *>(nf);
        if (df)
        {
            df->setAttribute(UNUM_ROUNDING_MODE, UNUM_ROUND_HALFUP, error);
            ICU_ASSERT(error, true);
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

    void SetNumberFormatSignificantDigits(IPlatformAgnosticResource *resource, const uint16 minSigDigits, const uint16 maxSigDigits)
    {
        // We know what actual type we stored in the IPlatformAgnosticResource*, so cast to it.
        icu::NumberFormat *nf = UNWRAP_RESOURCE(resource, icu::NumberFormat);

        // TODO(jahorto): Determine if we could ever have a NumberFormat that isn't a DecimalFormat (and if so, what to do here)
        icu::DecimalFormat *df = dynamic_cast<icu::DecimalFormat *>(nf);
        if (df)
        {
            df->setMinimumSignificantDigits(minSigDigits);
            df->setMaximumSignificantDigits(maxSigDigits);
        }
        else
        {
            // if we can't use DecimalFormat-specific features because we didn't get a DecimalFormat, we should not crash.
            // Best effort is good enough for Intl outputs, and we'd assume this is a transient issue.
            // However, non-DecimalFormat-based output might be regarded as buggy, especially if consistently wrong.
            // We'd like to use Debug builds to detect if this is the case and how prevalent it is, and we can make a further determination if and when we see failures here.
            AssertMsg(false, "Could not cast an icu::NumberFormat to an icu::DecimalFormat");
        }
    }

    void SetNumberFormatIntFracDigits(IPlatformAgnosticResource *resource, const uint16 minFracDigits, const uint16 maxFracDigits, const uint16 minIntDigits)
    {
        // We know what actual type we stored in the IPlatformAgnosticResource*, so cast to it.
        icu::NumberFormat *nf = UNWRAP_RESOURCE(resource, icu::NumberFormat);
        nf->setMinimumIntegerDigits(minIntDigits);
        nf->setMinimumFractionDigits(minFracDigits);
        nf->setMaximumFractionDigits(maxFracDigits);
    }

    void SetNumberFormatGroupingUsed(IPlatformAgnosticResource *resource, const bool isGroupingUsed)
    {
        // We know what actual type we stored in the IPlatformAgnosticResource*, so cast to it.
        icu::NumberFormat *nf = UNWRAP_RESOURCE(resource, icu::NumberFormat);
        nf->setGroupingUsed(isGroupingUsed);
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
                const char *pluralCount = nullptr; // REVIEW (doilij): is this okay? It's not entirely clear from the documentation whether this is an optional parameter.
                const UChar *currencyLongName = ucurr_getPluralName(uCurrencyCode, localeName, &isChoiceFormat, pluralCount, &currencyNameLen, &error);
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

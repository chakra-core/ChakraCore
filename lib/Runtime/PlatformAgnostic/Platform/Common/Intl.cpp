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
#include <unicode/ucol.h>
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

// #define INTL_ICU_DEBUG

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
            AssertOrFailFastMsg(false, u_errorName(e));                       \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_EXPR_FMT, u_errorName(e));                    \
            AssertOrFailFast(expr);                                           \
        }                                                                     \
    } while (false)

#define ICU_GOTO(e, expr, l)                                                  \
    do                                                                        \
    {                                                                         \
        if (U_FAILURE(e))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_ERROR_FMT, u_errorName(e));                   \
            goto l;                                                           \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_EXPR_FMT, u_errorName(e));                    \
            goto l;                                                           \
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

#define ASSERT_ENUM(T, e)                                                     \
    do                                                                        \
    {                                                                         \
        if ((int)(e) < 0 || (e) >= T::Max)                                    \
        {                                                                     \
            AssertOrFailFastMsg(false, #e " of type " #T " has an invalid value"); \
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
        ICU_ASSERT(error, forLangTagResultLength > 0 && parsedLength > 0);

        // Try to convert icuLocaleId (locale ID version of input locale string) to BCP47 language tag, using strict checks
        int32_t toLangTagResultLength = uloc_toLanguageTag(icuLocaleId, icuLangTag, ULOC_FULLNAME_CAPACITY, true, &error);
        ICU_ASSERT(error, toLangTagResultLength > 0);

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
        ICU_ASSERT(error, true);

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
                if (currencyDisplay == NumberFormatCurrencyDisplay::Symbol || currencyDisplay >= NumberFormatCurrencyDisplay::Max)
                {
                    // 0 (or default) => use symbol (e.g. "$" or "US$")
                    nf = icu::NumberFormat::createCurrencyInstance(locale, error);
                    ICU_ASSERT(error, true);

                    nf->setCurrency(reinterpret_cast<const UChar *>(currencyCode), error); // Ctrl-F: UChar_cast_explainer
                    ICU_ASSERT(error, true);
                }
                else if (currencyDisplay == NumberFormatCurrencyDisplay::Code || currencyDisplay == NumberFormatCurrencyDisplay::Name)
                {
                    // CODE e.g. "USD 42.00"; NAME (e.g. "42.00 US dollars")
                    // In both cases we need to be able to format in decimal and add the code or name afterwards.
                    // We will decide how to do this when platform.formatNumber is invoked (based on currencyDisplay again).
                    // TODO(doilij): How do we handle which side of the number to put the code or name? Can ICU do this? It doesn't seem clear how at the moment.
                    nf = icu::NumberFormat::createInstance(locale, error);
                    ICU_ASSERT(error, true);
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
        ASSERT_ENUM(NumberFormatStyle, formatterToUse);
        ASSERT_ENUM(NumberFormatCurrencyDisplay, currencyDisplay);
        icu::UnicodeString result;

        auto *formatterHolder = reinterpret_cast<PlatformAgnosticIntlObject<icu::NumberFormat> *>(formatter);
        if (!formatterHolder)
        {
            AssertOrFailFastMsg(formatterHolder, "Formatter did not hold an object of type `FinalizableIntlObject<icu::NumberFormat>`");
            return nullptr;
        }

        icu::NumberFormat *numberFormatter = formatterHolder->GetInstance();

        if (formatterToUse == NumberFormatStyle::Decimal || formatterToUse == NumberFormatStyle::Percent)
        {
            // we already created the formatter to format things according to the above options, so nothing else to do
            numberFormatter->format(val, result);
        }
        else if (formatterToUse == NumberFormatStyle::Currency)
        {
            UErrorCode error = UErrorCode::U_ZERO_ERROR;

            const UChar *uCurrencyCode = reinterpret_cast<const UChar *>(currencyCode); // Ctrl-F: UChar_cast_explainer
            const char *localeName = numberFormatter->getLocale(ULocDataLocaleType::ULOC_ACTUAL_LOCALE, error).getName();
            ICU_ASSERT(error, true);

            UBool isChoiceFormat = false;
            int32_t currencyNameLen = 0;

            if (currencyDisplay == NumberFormatCurrencyDisplay::Symbol || currencyDisplay == NumberFormatCurrencyDisplay::Default) // (e.g. "$42.00")
            {
                // the formatter is set up to render the symbol by default
                numberFormatter->format(val, result);
            }
            else if (currencyDisplay == NumberFormatCurrencyDisplay::Code) // (e.g. "USD 42.00")
            {
                result.append(uCurrencyCode);
                result.append("\u00a0"); // NON-BREAKING SPACE
                numberFormatter->format(val, result);
            }
            else if (currencyDisplay == NumberFormatCurrencyDisplay::Name) // (e.g. "US dollars")
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

    bool IsLocaleAvailable(_In_z_ const char *locale)
    {
        int32_t countAvailable = uloc_countAvailable();
        Assert(countAvailable > 0);
        for (int i = 0; i < countAvailable; i++)
        {
            const char *candidate = uloc_getAvailable(i);
            if (strcmp(locale, candidate) == 0)
            {
                return true;
            }
        }

        return false;
    }

    bool ResolveLocaleBestFit(_In_z_ const char16 *locale, _Out_ char16 *resolved)
    {
        // Note: the "best fit" matcher is implementation-defined, so ICU currently uses the JS implementation of resolveLocaleLookup as its best fit
        // TODO (doilij): implement a better "best fit" matcher
        resolved[0] = '\0';
        return false;
    }

    size_t GetUserDefaultLocaleName(_Out_ char16* langtag, _In_ size_t cchLangtag)
    {
        UErrorCode error = UErrorCode::U_ZERO_ERROR;
        char bcp47[ULOC_FULLNAME_CAPACITY] = { 0 };
        char defaultLocaleId[ULOC_FULLNAME_CAPACITY] = { 0 };

        int32_t written = uloc_getName(nullptr, defaultLocaleId, ULOC_FULLNAME_CAPACITY, &error);
        ICU_ASSERT(error, written > 0 && written < ULOC_FULLNAME_CAPACITY);

        defaultLocaleId[written] = 0;
        error = UErrorCode::U_ZERO_ERROR;

        written = uloc_toLanguageTag(defaultLocaleId, bcp47, ULOC_FULLNAME_CAPACITY, true, &error);
        ICU_ASSERT(error, written > 0 && static_cast<size_t>(written) < cchLangtag);

        return utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(
            langtag,
            reinterpret_cast<LPCUTF8>(bcp47),
            reinterpret_cast<LPCUTF8>(bcp47 + written)
        );
    }

    // Determines the BCP47 collation value that a given language tag will actually use
    // returns the count of bytes written into collation (guaranteed to be less than cchCollation)
    size_t CollatorGetCollation(_In_z_ const char *langtag, _Out_ char *collation, _In_ size_t cchCollation)
    {
        AssertOrFailFast(langtag != nullptr && collation != nullptr && cchCollation < ULOC_FULLNAME_CAPACITY);
        collation[0] = 0;
        size_t ret = 0;

        UErrorCode error = U_ZERO_ERROR;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t length = 0;
        uloc_forLanguageTag(langtag, localeID, ULOC_FULLNAME_CAPACITY, &length, &error);
        ICU_ASSERT(error, length > 0);

        UCollator *collator = ucol_open(localeID, &error);
        ICU_ASSERT(error, true);

        const char *collatorLocaleID = ucol_getLocaleByType(collator, ULOC_VALID_LOCALE, &error);
        ICU_ASSERT(error, true);

        char collatorCollation[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t collatorCollationLength = uloc_getKeywordValue(collatorLocaleID, "collation", collatorCollation, _countof(collatorCollation), &error);
        ICU_ASSERT(error, collatorCollationLength >= 0);
        if (collatorCollationLength == 0)
        {
            // We were given a langtag without a -u-co value, which is completely valid. Simply don't write anything into *collation
            goto LCloseCollator;
        }

        const char *bcpValue = uloc_toUnicodeLocaleType("collation", collatorCollation);
        size_t cchBcpValue = strlen(bcpValue);
        if (cchBcpValue != 0 && cchBcpValue < cchCollation)
        {
            errno_t err = memcpy_s(collation, cchCollation, bcpValue, cchBcpValue);
            if (err == 0)
            {
                collation[cchBcpValue] = 0;
            }
            else
            {
                AssertOrFailFastMsg(false, "Could not copy result of CollatorGetCollation to out param");
            }
        }
        else
        {
            AssertOrFailFastMsg(false, "UCollator says it's using a collation value that has no equivalent unicode extension");
        }

        ret = cchBcpValue;

LCloseCollator:
        ucol_close(collator);
        return ret;
    }

    // Compares left and right in the given locale with the given options
    // returns 0 on error, 1 for less, 2 for equal, and 3 for greater to match Win32 CompareStringEx API
    // *hr is set in all cases
    // TODO(jahorto): cache this UCollator object for later use
    int CollatorCompare(_In_z_ const char *langtag, _In_z_ const char16 *left, _In_ charcount_t cchLeft, _In_z_ const char16 *right, _In_ charcount_t cchRight,
        _In_ CollatorSensitivity sensitivity, _In_ bool ignorePunctuation, _In_ bool numeric, _In_ CollatorCaseFirst caseFirst, _Out_ HRESULT *hr)
    {
        ASSERT_ENUM(CollatorSensitivity, sensitivity);
        ASSERT_ENUM(CollatorCaseFirst, caseFirst);
        Assert(langtag != nullptr && left != nullptr && right != nullptr && hr != nullptr);
        int ret = 0;
        *hr = E_INVALIDARG;

        UErrorCode error = U_ZERO_ERROR;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t length = 0;
        uloc_forLanguageTag(reinterpret_cast<const char *>(langtag), localeID, ULOC_FULLNAME_CAPACITY, &length, &error);
        ICU_ASSERT(error, length > 0);

        UCollator *collator = ucol_open(localeID, &error);
        ICU_ASSERT(error, true);

        if (sensitivity == CollatorSensitivity::Base)
        {
            ucol_setStrength(collator, UCOL_PRIMARY);
        }
        else if (sensitivity == CollatorSensitivity::Accent)
        {
            ucol_setStrength(collator, UCOL_SECONDARY);
        }
        else if (sensitivity == CollatorSensitivity::Case)
        {
            // see "description" for the caseLevel default option: http://userguide.icu-project.org/collation/customization
            ucol_setStrength(collator, UCOL_PRIMARY);
            ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_ON, &error);
            ICU_ASSERT(error, true);
        }
        else if (sensitivity == CollatorSensitivity::Variant)
        {
            ucol_setStrength(collator, UCOL_TERTIARY);
        }
        else
        {
            goto LCloseCollator;
        }

        if (ignorePunctuation)
        {
            // see http://userguide.icu-project.org/collation/customization/ignorepunct
            ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &error);
            ICU_ASSERT(error, true);
        }

        if (numeric)
        {
            ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, UCOL_ON, &error);
            ICU_ASSERT(error, true);
        }

        if (caseFirst == CollatorCaseFirst::Upper)
        {
            ucol_setAttribute(collator, UCOL_CASE_FIRST, UCOL_UPPER_FIRST, &error);
            ICU_ASSERT(error, true);
        }
        else if (caseFirst == CollatorCaseFirst::Lower)
        {
            ucol_setAttribute(collator, UCOL_CASE_FIRST, UCOL_LOWER_FIRST, &error);
            ICU_ASSERT(error, true);
        }

        *hr = S_OK;
        UCollationResult result = ucol_strcoll(collator, reinterpret_cast<const UChar *>(left), cchLeft, reinterpret_cast<const UChar *>(right), cchRight);
        if (result == UCOL_LESS)
        {
            ret = 1;
        }
        else if (result == UCOL_EQUAL)
        {
            ret = 2;
        }
        else if (result == UCOL_GREATER)
        {
            ret = 3;
        }
        else
        {
            *hr = E_FAIL;
            ret = 0;
        }

LCloseCollator:
        ucol_close(collator);
        return ret;
    }
} // namespace Intl
} // namespace PlatformAgnostic

#endif // INTL_ICU

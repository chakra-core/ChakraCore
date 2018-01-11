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
#include <unicode/uloc.h>
#include <unicode/numfmt.h>
#include <unicode/enumset.h>
#include <unicode/decimfmt.h>
#include <unicode/ucol.h>
#include <unicode/ucal.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>

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

#define ICU_FAILURE(e) (U_FAILURE(e) || e == U_STRING_NOT_TERMINATED_WARNING)

#define ICU_RETURN(e, expr, r)                                                \
    do                                                                        \
    {                                                                         \
        if (ICU_FAILURE(e))                                                   \
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
        if (ICU_FAILURE(e))                                                   \
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
        if (ICU_FAILURE(e))                                                   \
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

// Some ICU functions don't like being given null/0-length buffers.
// We can work around this by making a local allocation of size 1 that will still
// trigger a U_BUFFER_OVERFLOW_ERROR but will allow the function to return the required length.
#define ICU_FIXBUF(type, bufArg, bufArgLen)                                   \
    type __temp; \
    const int __tempLen = 1; \
    bool isTemporaryBuffer = false; \
    if (bufArg == nullptr && bufArgLen == 0) \
    { \
        bufArg = &__temp; \
        bufArgLen = __tempLen; \
        isTemporaryBuffer = true; \
    }

// UNWRAP_* macros are convenience macros to turn an IPlatformAgnosticResource into the desired innerType
// UNWRAP_PAIO (PlatformAgnosticIntlObject) should only be called on IPlatformAgnosticResources that were originally
// created as PlatformAgnosticIntlObjects
// UNWRAP_C_OBJECT should only be called on IPlatformAgnosticResources that were originally created as IcuCObjects
#define UNWRAP_PAIO(resource, innerType) reinterpret_cast<PlatformAgnosticIntlObject<innerType> *>(resource)->GetInstance();
#define UNWRAP_C_OBJECT(resource, innerType) reinterpret_cast<IcuCObject<innerType> *>(resource)->GetInstance();

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
        int32_t collatorCollationLength = uloc_getKeywordValue(collatorLocaleID, "collation", collatorCollation, ULOC_FULLNAME_CAPACITY, &error);
        ICU_ASSERT(error, collatorCollationLength >= 0);
        if (collatorCollationLength != 0)
        {
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
        }

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
            AssertMsg(false, "sensitivity is not one of the CollatorSensitivity values");
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

        ucol_close(collator);
        return ret;
    }

    // Gets the system time zone. If tz is non-null, it is written into tz
    // Returns the number of characters written into tz (including a null terminator)
    int GetDefaultTimeZone(_Out_writes_opt_(tzLen) char16 *tz, _In_ int tzLen)
    {
        UErrorCode status = U_ZERO_ERROR;
        ICU_FIXBUF(char16, tz, tzLen); // sets bool isTemporaryBuffer
        int required = ucal_getDefaultTimeZone(reinterpret_cast<UChar *>(tz), tzLen, &status);
        if (isTemporaryBuffer && status == U_BUFFER_OVERFLOW_ERROR)
        {
            // buffer overflow is expected when we are just trying to get the length returned
            return required + 1;
        }

        ICU_ASSERT(status, required > 0 && required < tzLen);
        return required + 1; // return enough space for null character
    }

    // Determines if a time zone is valid. If it is and tzOut is non-null, the canonicalized version is written into tzOut
    // Returns the number of characters written into tzOut (including a null terminator), or 0 on invalid time zone
    int ValidateAndCanonicalizeTimeZone(_In_z_ const char16 *tzIn, _Out_writes_opt_(tzOutLen) char16 *tzOut, _In_ int tzOutLen)
    {
        UErrorCode status = U_ZERO_ERROR;
        ICU_FIXBUF(char16, tzOut, tzOutLen); // sets bool isTemporaryBuffer

        int required = ucal_getCanonicalTimeZoneID(reinterpret_cast<const UChar *>(tzIn), -1, reinterpret_cast<UChar *>(tzOut), tzOutLen, nullptr, &status);
        if (isTemporaryBuffer && status == U_BUFFER_OVERFLOW_ERROR)
        {
            // buffer overflow is expected when we are just trying to get the length returned
            return required + 1;
        }
        else if (status == U_ILLEGAL_ARGUMENT_ERROR)
        {
            // illegal argument here means that tzIn is an invalid time zone
            return 0;
        }

        ICU_ASSERT(status, required > 0 && required < tzOutLen);
        return required + 1; // return enough space for null character
    }

    // Generates an LDML pattern for the given LDML skeleton in the given locale. If pattern is non-null, the result is written into pattern
    // LDML here means the Unicode Locale Data Markup Language: http://www.unicode.org/reports/tr35/tr35-dates.html#Date_Field_Symbol_Table
    // Returns the number of characters written into pattern (including a null terminator) [should always be positive]
    int GetPatternForSkeleton(_In_z_ const char *langtag, _In_z_ const char16 *skeleton, _Out_writes_opt_(patternLen) char16 *pattern, _In_ int patternLen)
    {
        UErrorCode status = U_ZERO_ERROR;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        int length = 0;
        uloc_forLanguageTag(langtag, localeID, ULOC_FULLNAME_CAPACITY, &length, &status);
        ICU_ASSERT(status, length > 0);

        UDateTimePatternGenerator *dtpg = udatpg_open(localeID, &status);
        ICU_ASSERT(status, true);

        int bestPatternLen = udatpg_getBestPatternWithOptions(
            dtpg,
            reinterpret_cast<const UChar *>(skeleton),
            -1,
            UDATPG_MATCH_ALL_FIELDS_LENGTH,
            reinterpret_cast<UChar *>(pattern),
            patternLen,
            &status
        );

        if (pattern == nullptr && patternLen == 0 && status == U_BUFFER_OVERFLOW_ERROR)
        {
            // just counting bytes, U_BUFFER_OVERFLOW_ERROR is expected
            AssertOrFailFast(bestPatternLen > 0);
        }
        else
        {
            ICU_ASSERT(status, bestPatternLen > 0 && bestPatternLen < patternLen);
        }

        udatpg_close(dtpg);
        return bestPatternLen + 1; // return enough space for null character
    }

    // creates a UDateFormat and wraps it in an IPlatformAgnosticResource
    void CreateDateTimeFormat(_In_z_ const char *langtag, _In_z_ const char16 *timeZone, _In_z_ const char16 *pattern, _Out_ IPlatformAgnosticResource **resource)
    {
        UErrorCode status = U_ZERO_ERROR;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        int length = 0;
        uloc_forLanguageTag(langtag, localeID, ULOC_FULLNAME_CAPACITY, &length, &status);
        ICU_ASSERT(status, length > 0);

        UDateFormat *dtf = udat_open(
            UDAT_PATTERN,
            UDAT_PATTERN,
            localeID,
            reinterpret_cast<const UChar *>(timeZone),
            -1,
            reinterpret_cast<const UChar *>(pattern),
            -1,
            &status
        );
        ICU_ASSERT(status, true);

        // DateTimeFormat is expected to use the "proleptic Gregorian calendar", which means that the Julian calendar should never be used.
        // To accomplish this, we can set the switchover date between julian/gregorian
        // to the ECMAScript beginning of time, which is -8.64e15 according to ecma262 #sec-time-values-and-time-range
        UCalendar *cal = const_cast<UCalendar *>(udat_getCalendar(dtf));
        ucal_setGregorianChange(cal, -8.64e15, &status);

        // status can be U_UNSUPPORTED_ERROR if the calendar isn't gregorian, which
        // there does not seem to be a way to check for ahead of time in the C API
        AssertOrFailFastMsg(U_SUCCESS(status) || status == U_UNSUPPORTED_ERROR, u_errorName(status));

        IPlatformAgnosticResource *formatterResource = new IcuCObject<UDateFormat>(dtf, &udat_close);
        AssertOrFailFast(formatterResource);
        *resource = formatterResource;
    }

    // Formats `date` using the UDateFormat wrapped by `resource`. If `formatted` is non-null, the result is written into `formatted`
    // Returns the number of characters written into `formatted` (including a null terminator) [should always be positive]
    int FormatDateTime(_In_ IPlatformAgnosticResource *resource, _In_ double date, _Out_writes_opt_(formattedLen) char16 *formatted, _In_ int formattedLen)
    {
        UErrorCode status = U_ZERO_ERROR;
        UDateFormat *dtf = UNWRAP_C_OBJECT(resource, UDateFormat);

        int required = udat_format(dtf, date, reinterpret_cast<UChar *>(formatted), formattedLen, /* UFieldPosition */ nullptr, &status);
        if (formatted == nullptr && formattedLen == 0 && status == U_BUFFER_OVERFLOW_ERROR)
        {
            // when we are just counting bytes, we can ignore errors
            AssertOrFailFast(required > 0);
        }
        else
        {
            ICU_ASSERT(status, required > 0 && required < formattedLen);
        }

        return required + 1; // return enough space for null character
    }

    // Formats `date` using the UDateFormat wrapped by `resource`. If `formatted` is non-null, the result is written into `formatted`
    // Takes an additional parameter, fieldIterator, which is set to a IPlatformAgnosticResource* that should later be given to GetDateTimePartInfo
    // Returns the number of characters written into `formatted` (includes a null terminator) [should always be positive]
    int FormatDateTimeToParts(_In_ IPlatformAgnosticResource *resource, _In_ double date, _Out_writes_opt_(formattedLen) char16 *formatted,
        _In_ int formattedLen, _Out_opt_ IPlatformAgnosticResource **fieldIterator)
    {
        UErrorCode status = U_ZERO_ERROR;
        UDateFormat *dtf = UNWRAP_C_OBJECT(resource, UDateFormat);

        UFieldPositionIterator *fpi = nullptr;

        if (fieldIterator)
        {
            fpi = ufieldpositer_open(&status);
            ICU_ASSERT(status, true);
            IPlatformAgnosticResource *fpiResource = new IcuCObject<UFieldPositionIterator>(fpi, &ufieldpositer_close);
            AssertOrFailFastMsg(fpiResource, "Out of memory");
            *fieldIterator = fpiResource;
        }

        int required = udat_formatForFields(dtf, date, reinterpret_cast<UChar *>(formatted), formattedLen, fpi, &status);
        if (formatted == nullptr && formattedLen == 0 && status == U_BUFFER_OVERFLOW_ERROR)
        {
            // when we are just counting bytes, we can ignore ICU errors
            AssertOrFailFast(required > 0);
        }
        else
        {
            ICU_ASSERT(status, required > 0 && required < formattedLen);
        }

        return required + 1; // return enough space for null character
    }

    // Given a stateful fieldIterator, sets partStart and partEnd to the start (inclusive) and end (exclusive) of the substring for the part
    // and sets partKind to be the type of the part (really a UDateFormatField -- see GetDateTimePartKind)
    bool GetDateTimePartInfo(_In_ IPlatformAgnosticResource *fieldIterator, _Out_ int *partStart, _Out_ int *partEnd, _Out_ int *partKind)
    {
        UFieldPositionIterator *fpi = UNWRAP_C_OBJECT(fieldIterator, UFieldPositionIterator);

        *partKind = ufieldpositer_next(fpi, partStart, partEnd);
        return *partKind > 0;
    }

    // Given a partKind set by GetDateTimePartInfo, return the corresponding string for the "type" field of the formatToParts return object
    // NOTE - keep this up to date with the map in Intl.js#getPatternForSkeleton and the UDateFormatField enum
    // See ECMA-402: #sec-partitiondatetimepattern
    const char16 *GetDateTimePartKind(_In_ int partKind)
    {
        UDateFormatField field = (UDateFormatField) partKind;
        switch (field)
        {
        case UDAT_ERA_FIELD:
            return _u("era");
        case UDAT_YEAR_FIELD:
        case UDAT_EXTENDED_YEAR_FIELD:
        case UDAT_YEAR_NAME_FIELD:
            return _u("year");
        case UDAT_MONTH_FIELD:
        case UDAT_STANDALONE_MONTH_FIELD:
            return _u("month");
        case UDAT_DATE_FIELD:
            return _u("day");
        case UDAT_HOUR_OF_DAY1_FIELD:
        case UDAT_HOUR_OF_DAY0_FIELD:
        case UDAT_HOUR1_FIELD:
        case UDAT_HOUR0_FIELD:
            return _u("hour");
        case UDAT_MINUTE_FIELD:
            return _u("minute");
        case UDAT_SECOND_FIELD:
            return _u("second");
        case UDAT_DAY_OF_WEEK_FIELD:
        case UDAT_STANDALONE_DAY_FIELD:
        case UDAT_DOW_LOCAL_FIELD:
            return _u("weekday");
        case UDAT_AM_PM_FIELD:
            return _u("dayPeriod");
        case UDAT_TIMEZONE_FIELD:
        case UDAT_TIMEZONE_RFC_FIELD:
        case UDAT_TIMEZONE_GENERIC_FIELD:
        case UDAT_TIMEZONE_SPECIAL_FIELD:
        case UDAT_TIMEZONE_LOCALIZED_GMT_OFFSET_FIELD:
        case UDAT_TIMEZONE_ISO_FIELD:
            return _u("timeZone");
        default:
            return _u("unknown");
        }
    }
} // namespace Intl
} // namespace PlatformAgnostic

#endif // INTL_ICU

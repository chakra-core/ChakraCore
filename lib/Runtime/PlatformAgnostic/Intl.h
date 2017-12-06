//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "IPlatformAgnosticResource.h"

namespace PlatformAgnostic
{
namespace Intl
{
    // NOTE(jahorto): Keep these enums in sync with those by the same name in Intl.js
    // These enums are used by both WinGlob- and ICU-backed Intl
    enum class NumberFormatStyle
    {
        Decimal, // Intl.NumberFormat(locale, { style: "decimal" }); // aka in our code as "number"
        Percent, // Intl.NumberFormat(locale, { style: "percent" });
        Currency, // Intl.NumberFormat(locale, { style: "currency", ... });

        Max,
        Default = Decimal,
    };

    enum class NumberFormatCurrencyDisplay
    {
        Symbol, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "symbol" }); // e.g. "$" or "US$" depeding on locale
        Code, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "code" }); // e.g. "USD"
        Name, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "name" }); // e.g. "US dollar"

        Max,
        Default = Symbol,
    };

    enum class CollatorSensitivity
    {
        Base,
        Accent,
        Case,
        Variant,

        Max,
        Default = Variant,
    };

    enum class CollatorCaseFirst
    {
        Upper,
        Lower,
        False,

        Max,
        Default = False,
    };

#ifdef INTL_ICU
    using namespace PlatformAgnostic::Resource;

    template <typename T>
    class PlatformAgnosticIntlObject : public IPlatformAgnosticResource
    {
    private:
        T *intlObject;

    public:
        PlatformAgnosticIntlObject(T *object) :
            intlObject(object)
        {
        }

        T *GetInstance()
        {
            return intlObject;
        }

        void Cleanup() override
        {
            delete intlObject;
            intlObject = nullptr;
        }

        // Explicitly not overriding the default destructor; ensure that Cleanup() is used instead.
        // Note: Cleanup() is called by, e.g., AutoIcuJsObject
    };

    template<typename TResource>
    class IcuCObject : public IPlatformAgnosticResource
    {
    public:
        typedef void (*CleanupFunc)(TResource *);

    private:
        TResource *resource;
        CleanupFunc cleanupFunc;

    public:
        IcuCObject(TResource *resource, CleanupFunc cleanupFunc) :
            resource(resource),
            cleanupFunc(cleanupFunc)
        {

        }

        TResource *GetInstance()
        {
            return resource;
        }

        void Cleanup() override
        {
            cleanupFunc(resource);
            resource = nullptr;
        }
    };

    // Generic spec operations
    bool IsWellFormedLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch);
    HRESULT NormalizeLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _Out_ char16 *normalized, _Out_ size_t *normalizedLength);
    bool IsLocaleAvailable(_In_z_ const char *locale);
    bool ResolveLocaleBestFit(_In_z_ const char16 *locale, _Out_ char16 *resolved);
    size_t GetUserDefaultLocaleName(_Out_ char16* langtag, _In_ size_t cchLangtag);

    // NumberFormat
    int32_t GetCurrencyFractionDigits(_In_z_ const char16 * currencyCode);
    template <typename Func>
    HRESULT CreateFormatter(Func function, _In_z_ const char16 *languageTag, _In_ const charcount_t cch, _Out_ IPlatformAgnosticResource **resource);
    HRESULT CreateNumberFormatter(_In_z_ const char16 *languageTag, _In_ const charcount_t cch, _Out_ IPlatformAgnosticResource **resource);
    HRESULT CreatePercentFormatter(_In_z_ const char16 *languageTag, _In_ const charcount_t cch, _Out_ IPlatformAgnosticResource **resource);
    HRESULT CreateCurrencyFormatter(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _In_z_ const char16 *currencyCode, _In_ const NumberFormatCurrencyDisplay currencyDisplay, _Out_ IPlatformAgnosticResource **resource);
    void SetNumberFormatSignificantDigits(IPlatformAgnosticResource *resource, const uint16 minSigDigits, const uint16 maxSigDigits);
    void SetNumberFormatIntFracDigits(IPlatformAgnosticResource *resource, const uint16 minFracDigits, const uint16 maxFracDigits, const uint16 minIntDigits);
    void SetNumberFormatGroupingUsed(_In_ IPlatformAgnosticResource *resource, _In_ const bool isGroupingUsed);
    template <typename T>
    const char16 *FormatNumber(IPlatformAgnosticResource *formatter, const T val, const NumberFormatStyle formatterToUse,
        const NumberFormatCurrencyDisplay currencyDisplay, const char16 *currencyCode);

    // Collator
    size_t CollatorGetCollation(_In_z_ const char *langtag, _Out_ char *collation, _In_ size_t cchCollation);
    int CollatorCompare(_In_z_ const char *langtag, _In_z_ const char16 *left, _In_ charcount_t cchLeft, _In_z_ const char16 *right, _In_ charcount_t cchRight,
        _In_ CollatorSensitivity sensitivity, _In_ bool ignorePunctuation, _In_ bool numeric, _In_ CollatorCaseFirst caseFirst, _Out_ HRESULT *hr);

    // DateTimeFormat
    int GetDefaultTimeZone(_Out_writes_opt_(tzLen) char16 *tz = nullptr, _In_ int tzLen = -1);
    int ValidateAndCanonicalizeTimeZone(_In_z_ const char16 *tzIn, _Out_writes_opt_(tzOutLen) char16 *tzOut = nullptr, _In_ int tzOutLen = -1);
    int GetPatternForSkeleton(_In_z_ const char *langtag, _In_z_ const char16 *skeleton, _Out_writes_opt_(patternLen) char16 *pattern = nullptr, _In_ int patternLen = -1);
    void CreateDateTimeFormat(_In_z_ const char *langtag, _In_z_ const char16 *timeZone, _In_z_ const char16 *pattern, _Out_ IPlatformAgnosticResource **resource);
    int FormatDateTime(_In_ IPlatformAgnosticResource *resource, _In_ double date, _Out_writes_opt_(formattedLen) char16 *formatted = nullptr, _In_ int formattedLen = -1);
    int FormatDateTimeToParts(_In_ IPlatformAgnosticResource *resource, _In_ double date, _Out_writes_opt_(formattedLen) char16 *formatted = nullptr,
        _In_ int formattedLen = -1, _Out_opt_ IPlatformAgnosticResource **fieldIterator = nullptr);
    bool GetDateTimePartInfo(_In_ IPlatformAgnosticResource *fieldIterator, _Out_ int *partStart, _Out_ int *partEnd, _Out_ int *partKind);
    const char16 *GetDateTimePartKind(_In_ int partKind);
#endif // INTL_ICU
} // namespace Intl
} // namespace PlatformAgnostic

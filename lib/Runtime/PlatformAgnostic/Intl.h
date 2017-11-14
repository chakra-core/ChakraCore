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

    bool IsWellFormedLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch);
    HRESULT NormalizeLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _Out_ char16 *normalized, _Out_ size_t *normalizedLength);

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

    bool IsLocaleAvailable(_In_z_ const char *locale);
    bool ResolveLocaleBestFit(_In_z_ const char16 *locale, _Out_ char16 *resolved);
    size_t GetUserDefaultLocaleName(_Out_ char16* langtag, _In_ size_t cchLangtag);

    size_t CollatorGetCollation(_In_z_ const char *langtag, _Out_ char *collation, _In_ size_t cchCollation);
    int CollatorCompare(_In_z_ const char *langtag, _In_z_ const char16 *left, _In_ charcount_t cchLeft, _In_z_ const char16 *right, _In_ charcount_t cchRight,
        _In_ CollatorSensitivity sensitivity, _In_ bool ignorePunctuation, _In_ bool numeric, _In_ CollatorCaseFirst caseFirst, _Out_ HRESULT *hr);
#endif // INTL_ICU
} // namespace Intl
} // namespace PlatformAgnostic

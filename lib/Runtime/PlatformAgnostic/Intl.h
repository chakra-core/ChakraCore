//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef INTL_ICU

#include "IPlatformAgnosticResource.h"

namespace PlatformAgnostic
{
namespace Intl
{
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

    // Keep these enums in sync with those by the same name in Intl.js
    enum class NumberFormatStyle
    {
        DEFAULT = 0, // "decimal" is the default
        DECIMAL = 0, // Intl.NumberFormat(locale, { style: "decimal" }); // aka in our code as "number"
        PERCENT = 1, // Intl.NumberFormat(locale, { style: "percent" });
        CURRENCY = 2, // Intl.NumberFormat(locale, { style: "currency", ... });
        MAX = 3
    };
    enum class NumberFormatCurrencyDisplay
    {
        DEFAULT = 0, // "symbol" is the default
        SYMBOL = 0, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "symbol" }); // e.g. "$" or "US$" depeding on locale
        CODE = 1, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "code" }); // e.g. "USD"
        NAME = 2, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "name" }); // e.g. "US dollar"
        MAX = 3
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
    const char16 *FormatNumber(IPlatformAgnosticResource *formatter, const T val, const NumberFormatStyle formatterToUse, const NumberFormatCurrencyDisplay currencyDisplay, const char16 *currencyCode);

    bool ResolveLocaleLookup(_In_z_ const char16 *locale, _Out_ char16 *resolved);
    bool ResolveLocaleBestFit(_In_z_ const char16 *locale, _Out_ char16 *resolved);
    size_t GetUserDefaultLanguageTag(_Out_ char16* langtag, _In_ size_t cchLangtag);

} // namespace Intl
} // namespace PlatformAgnostic

#endif // INTL_ICU

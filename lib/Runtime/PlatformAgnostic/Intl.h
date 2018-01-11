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
            delete this;
        }

        // Explicitly not overriding the default destructor; ensure that Cleanup() is used instead.
        // Note: Cleanup() is called by, e.g., AutoIcuJsObject
    };

    template<typename TResource>
    class IcuCObject : public IPlatformAgnosticResource
    {
    public:
        typedef void (__cdecl *CleanupFunc)(TResource *);

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
            delete this;
        }
    };

    // Generic spec operations
    size_t GetUserDefaultLocaleName(_Out_ char16* langtag, _In_ size_t cchLangtag);

    // Collator
    int CollatorCompare(_In_z_ const char *langtag, _In_z_ const char16 *left, _In_ charcount_t cchLeft, _In_z_ const char16 *right, _In_ charcount_t cchRight,
        _In_ CollatorSensitivity sensitivity, _In_ bool ignorePunctuation, _In_ bool numeric, _In_ CollatorCaseFirst caseFirst, _Out_ HRESULT *hr);

    // DateTimeFormat
    int GetDefaultTimeZone(_Out_writes_opt_(tzLen) char16 *tz = nullptr, _In_ int tzLen = 0);
    int ValidateAndCanonicalizeTimeZone(_In_z_ const char16 *tzIn, _Out_writes_opt_(tzOutLen) char16 *tzOut = nullptr, _In_ int tzOutLen = 0);
    int GetPatternForSkeleton(_In_z_ const char *langtag, _In_z_ const char16 *skeleton, _Out_writes_opt_(patternLen) char16 *pattern = nullptr, _In_ int patternLen = 0);
    void CreateDateTimeFormat(_In_z_ const char *langtag, _In_z_ const char16 *timeZone, _In_z_ const char16 *pattern, _Out_ IPlatformAgnosticResource **resource);
    int FormatDateTime(_In_ IPlatformAgnosticResource *resource, _In_ double date, _Out_writes_opt_(formattedLen) char16 *formatted = nullptr, _In_ int formattedLen = 0);
    int FormatDateTimeToParts(_In_ IPlatformAgnosticResource *resource, _In_ double date, _Out_writes_opt_(formattedLen) char16 *formatted = nullptr,
        _In_ int formattedLen = 0, _Out_opt_ IPlatformAgnosticResource **fieldIterator = nullptr);
    bool GetDateTimePartInfo(_In_ IPlatformAgnosticResource *fieldIterator, _Out_ int *partStart, _Out_ int *partEnd, _Out_ int *partKind);
    const char16 *GetDateTimePartKind(_In_ int partKind);
#endif // INTL_ICU
} // namespace Intl
} // namespace PlatformAgnostic

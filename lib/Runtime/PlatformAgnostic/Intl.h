//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef RUNTIME_PLATFORM_AGNOSTIC_COMMON_INTL
#define RUNTIME_PLATFORM_AGNOSTIC_COMMON_INTL

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

    bool IsWellFormedLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch);
    HRESULT NormalizeLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
        _Out_ char16 *normalized, _Out_ size_t *normalizedLength);

    int32_t GetCurrencyFractionDigits(_In_z_ const char16 * currencyCode);

    IPlatformAgnosticResource *CreateNumberFormat();

    template <typename T>
    const char16 *FormatNumber(IPlatformAgnosticResource *numberFormatter, T val);
} // namespace Intl
} // namespace PlatformAgnostic

#endif // INTL_ICU

#endif /* RUNTIME_PLATFORM_AGNOSTIC_COMMON_INTL */

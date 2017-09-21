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

        /*
        // NOTE: cannot return non-pointer instance of a pure virtual class, so move ctor and move asgn will not be applicable.

        // TODO (doilij): Remove if unused
        // move ctor
        PlatformAgnosticIntlObject(PlatformAgnosticIntlObject &&other) :
        intlObject(nullptr)
        {
        this->move(other);
        }

        // TODO (doilij): Remove if unused
        // move assignment
        PlatformAgnosticIntlObject operator=(PlatformAgnosticIntlObject &&other);

        void move(PlatformAgnosticIntlObject &&other)
        {
        // Take control of other's intlObject.
        this->intlObject = other->intlObject;
        other->intlObject = nullptr;
        }
        */

        T *GetInstance()
        {
            return intlObject;
        }

        void Cleanup() override
        {
            delete intlObject;
            intlObject = nullptr;
        }

        // explicitly not overriding the default destructor, allow Cleanup() to cleanup
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

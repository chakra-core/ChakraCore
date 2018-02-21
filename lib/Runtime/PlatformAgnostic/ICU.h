//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef INTL_ICU // TODO(jahorto): this needs to eventually be HAS_ICU, but HAS_ICU (HAS_REAL_ICU) and ENABLE_GLOBALIZATION conflict
#ifdef WINDOWS10_ICU
#include <icu.h>
#else
#define U_STATIC_IMPLEMENTATION 1
#define U_SHOW_CPLUSPLUS_API 0
#include "unicode/ucal.h"
#include "unicode/ucol.h"
#include "unicode/udat.h"
#include "unicode/udatpg.h"
#include "unicode/uloc.h"
#include "unicode/unumsys.h"
#endif

namespace PlatformAgnostic
{
    namespace ICUHelpers
    {
        template<typename TObject, void(__cdecl * CloseFunction)(TObject)>
        class ScopedICUObject
        {
        private:
            TObject object;
        public:
            explicit ScopedICUObject(TObject object) : object(object)
            {

            }
            ScopedICUObject(const ScopedICUObject &other) = delete;
            ScopedICUObject(ScopedICUObject &&other) = delete;
            ScopedICUObject &operator=(const ScopedICUObject &other) = delete;
            ScopedICUObject &operator=(ScopedICUObject &&other) = delete;
            ~ScopedICUObject()
            {
                if (object != nullptr)
                {
                    CloseFunction(object);
                }
            }

            operator TObject()
            {
                return object;
            }
        };

        typedef ScopedICUObject<UEnumeration *, uenum_close> ScopedUEnumeration;
        typedef ScopedICUObject<UCollator *, ucol_close> ScopedUCollator;
        typedef ScopedICUObject<UDateFormat *, udat_close> ScopedUDateFormat;
        typedef ScopedICUObject<UNumberFormat *, unum_close> ScopedUNumberFormat;
        typedef ScopedICUObject<UNumberingSystem *, unumsys_close> ScopedUNumberingSystem;
        typedef ScopedICUObject<UDateTimePatternGenerator *, udatpg_close> ScopedUDateTimePatternGenerator;
    }
}
#endif

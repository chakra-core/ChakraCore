//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "CommonDefines.h"

#ifdef HAS_ICU
#ifdef WINDOWS10_ICU
// if WINDOWS10_ICU is defined, pretend like we are building for recent Redstone,
// even if that isn't necessarily true
#pragma push_macro("NTDDI_VERSION")
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS3
#include <icu.h>
#pragma pop_macro("NTDDI_VERSION")
#else
#include "unicode/ucal.h"
#include "unicode/ucol.h"
#include "unicode/udat.h"
#include "unicode/udatpg.h"
#include "unicode/uloc.h"
#include "unicode/unumsys.h"
#include "unicode/ustring.h"
#include "unicode/unorm2.h"
#endif

// Different assertion code is used in ChakraFull that enforces that messages are char literals
#ifdef _CHAKRACOREBUILD
#define ICU_ERRORMESSAGE(e) u_errorName(e)
#else
#define ICU_ERRORMESSAGE(e) "Bad status returned from ICU"
#endif

#ifdef INTL_ICU_DEBUG
#define ICU_DEBUG_PRINT(fmt, msg) Output::Print(fmt, __func__, (msg))
#else
#define ICU_DEBUG_PRINT(fmt, msg)
#endif

#define ICU_FAILURE(e) (U_FAILURE(e) || e == U_STRING_NOT_TERMINATED_WARNING)
#define ICU_BUFFER_FAILURE(e) (e == U_BUFFER_OVERFLOW_ERROR || e == U_STRING_NOT_TERMINATED_WARNING)

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
        typedef ScopedICUObject<UFieldPositionIterator *, ufieldpositer_close> ScopedUFieldPositionIterator;

        // This function implements retry logic for calling ICU C APIs,
        // where it is common that a first call might not succeed because a destination buffer is not big enough
        template<typename TBuffer, typename ICUFunc, typename AllocatorFunc>
        inline bool ExecuteICUWithRetry(AllocatorFunc allocate, ICUFunc func, int firstTryLen, TBuffer **ret, int *returnLen)
        {
            UErrorCode status = U_ZERO_ERROR;
            *ret = allocate(firstTryLen);
            *returnLen = func(reinterpret_cast<UChar *>(*ret), firstTryLen, &status);
            if (ICU_BUFFER_FAILURE(status))
            {
                int secondTryLen = *returnLen + 1;
                *ret = allocate(secondTryLen);
                status = U_ZERO_ERROR;
                *returnLen = func(reinterpret_cast<UChar *>(*ret), secondTryLen, &status);
            }
            return U_SUCCESS(status) && status != U_STRING_NOT_TERMINATED_WARNING && *returnLen > 0;
        }

        inline int GetICUMajorVersion()
        {
            UVersionInfo version = { 0 };
            u_getVersion(version);
            return version[0];
        }
    }
}
#endif

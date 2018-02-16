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
        template<typename T>
        class AutoICUObject
        {
        public:
            typedef void(__cdecl * CloseFunction)(T);
        private:
            T object;
            CloseFunction closeFunc;
        public:
            explicit AutoICUObject(T object);
            AutoICUObject(const AutoICUObject &other) = delete;
            ~AutoICUObject();
            operator T();
            T Detatch();
        };

        template<typename T>
        constexpr typename AutoICUObject<T>::CloseFunction CloseFunctionForType(T type);

        template<typename T>
        AutoICUObject<T>::AutoICUObject(T object) : object(object)
        {
            closeFunc = CloseFunctionForType(object);
        }

        template<typename T>
        AutoICUObject<T>::~AutoICUObject()
        {
            if (object != nullptr)
            {
                closeFunc(object);
            }
        }

        template<typename T>
        AutoICUObject<T>::operator T()
        {
            return object;
        }

        template<typename T>
        T AutoICUObject<T>::Detatch()
        {
            T ret = object;
            object = nullptr;
            return ret;
        }

#define INSTANTIATE_AUTOICUOBJECT(T, Closer) \
        template<> \
        constexpr AutoICUObject<T *>::CloseFunction CloseFunctionForType(T *) \
        { \
            return Closer; \
        } \
        typedef AutoICUObject<T *> Auto##T;

        INSTANTIATE_AUTOICUOBJECT(UEnumeration, uenum_close)
        INSTANTIATE_AUTOICUOBJECT(UDateFormat, udat_close)
    }
}

#endif

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Banned.h"
#include "CommonDefines.h"
#define _CRT_RAND_S         // Enable rand_s in the CRT

#ifndef __has_feature
#define __has_feature(f) 0
#endif

#if __has_feature(address_sanitizer)
#define ADDRESS_SANITIZER_APPEND(x) , x
#define NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))
#define NO_SANITIZE_ADDRESS_FIXVC
#else
#define ADDRESS_SANITIZER_APPEND(x)
#define NO_SANITIZE_ADDRESS
#endif

// AddressSanitizer: check if an address is in asan fake stack
#if __has_feature(address_sanitizer)
extern "C"
{
    void *__asan_get_current_fake_stack();
    void *__asan_addr_is_in_fake_stack(void *fake_stack, void *addr, void **beg, void **end);
}
inline bool IsAsanFakeStackAddr(const void * p)
{
    void * fakeStack = __asan_get_current_fake_stack();
    return fakeStack && __asan_addr_is_in_fake_stack(fakeStack, const_cast<void*>(p), nullptr, nullptr);
}
#define IS_ASAN_FAKE_STACK_ADDR(p) IsAsanFakeStackAddr(p)
#else
#define IS_ASAN_FAKE_STACK_ADDR(p) false
#endif

#if defined(PROFILE_RECYCLER_ALLOC) || defined(HEAP_TRACK_ALLOC) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#ifdef __clang__
#include <typeinfo>
using std::type_info;
#endif
#endif

#include "CommonPal.h"

#include "Core/CommonMinMax.h"

// === Core Header Files ===
#include "Core/CommonTypedefs.h"
#include "Core/Api.h"
#include "Core/CriticalSection.h"
#include "Core/Assertions.h"

// === Exceptions Header Files ===
#include "Exceptions/Throw.h"
#include "Exceptions/ExceptionCheck.h"
#include "Exceptions/ReportError.h"

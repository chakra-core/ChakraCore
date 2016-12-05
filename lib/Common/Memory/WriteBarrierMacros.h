//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

//
// WriteBarrier Macros are weird because of "compat"
// In general, write barriers are used only on non-Win32 builds
// However, for perf, some classes are also allocated in Write-Barrier memory, even on Windows
// Those classes can force WriteBarrier types to always be generated for their definitions
//
// Usage:
//   In general, don't need to use anything, just use the macros to annotate your fields
//
//

#define FieldWithBarrier(type, ...) \
    typename WriteBarrierFieldTypeTraits<type, ##__VA_ARGS__>::Type

#if GLOBAL_ENABLE_WRITE_BARRIER
#define Field(type, ...) \
    FieldWithBarrier(type, ##__VA_ARGS__)
#define FieldNoBarrier(type) \
    typename WriteBarrierFieldTypeTraits<type, _no_write_barrier_policy, _no_write_barrier_policy>::Type
#else
#define Field(type, ...) type
#define FieldNoBarrier(type) type
#endif

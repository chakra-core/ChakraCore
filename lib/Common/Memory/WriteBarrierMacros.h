//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// WriteBarrier Macros are weird because of "compat"
// In general, write barriers are used only on non-Win32 builds
// However, for perf, some classes are also allocated in Write-Barrier memory, even on Windows
// Those classes can force WriteBarrier types to always be generated for their definitions
//
// Usage:
//   In general, don't need to use anything, just use the macros to annotate your fields
//
//   If you want to force your fields to always generate the right write-barrier types,
//   at the start of the file, add the following:
//     #define FORCE_USE_WRITE_BARRIER
//     #include <Memory/WriteBarrierMacros.h>
//   and at the end of the file:
//     RESTORE_WRITE_BARRIER_MACROS()
//
#if !defined(__WRITE_BARRIER_MACROS__) || defined(FORCE_USE_WRITE_BARRIER)

#ifndef __WRITE_BARRIER_MACROS__
#define __WRITE_BARRIER_MACROS__ 1
#define PopMacro(x) __pragma(pop_macro( #x ))
#define PushMacro(x) __pragma(push_macro( #x ))
#define SAVE_WRITE_BARRIER_MACROS() \
    PushMacro(Field) \
    PushMacro(FieldNoBarrier)
#define RESTORE_WRITE_BARRIER_MACROS() \
    PopMacro(Field) \
    PopMacro(FieldNoBarrier)
#endif

#ifdef FORCE_USE_WRITE_BARRIER
SAVE_WRITE_BARRIER_MACROS()
#undef Field
#undef FieldNoBarrier
#endif

#if GLOBAL_ENABLE_WRITE_BARRIER || defined(FORCE_USE_WRITE_BARRIER)
// Various macros for defining field attributes
#define Field(type, ...) \
    typename WriteBarrierFieldTypeTraits<type, ##__VA_ARGS__>::Type
#define FieldNoBarrier(type) \
    typename WriteBarrierFieldTypeTraits<type, _no_write_barrier_policy, _no_write_barrier_policy>::Type
#else
#define Field(type, ...) type
#define FieldNoBarrier(type) type
#endif

#ifdef FORCE_USE_WRITE_BARRIER
#undef FORCE_USE_WRITE_BARRIER
#endif

#endif

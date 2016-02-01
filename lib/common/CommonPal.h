//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once


#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4995) /* 'function': name was marked as #pragma deprecated */

// === Windows Header Files ===
#define INC_OLE2                 /* for windows.h */
#define CONST_VTABLE             /* for objbase.h */
#include <windows.h>

/* Don't want GetObject and GetClassName to be defined to GetObjectW and GetClassNameW */
#undef GetObject
#undef GetClassName
#undef Yield /* winbase.h defines this but we want to use it for Js::OpCode::Yield; it is Win16 legacy, no harm undef'ing it */
#pragma warning(pop)

typedef wchar_t wchar16;

// xplat-todo: get a better name for this macro
#define CH_WSTR(s) L##s

#define get_cpuid __cpuid

#else // !_WIN32

#include "inc/pal.h"
#include "inc/rt/palrt.h"
#include "inc/rt/no_sal2.h"

typedef char16_t wchar16;
#define CH_WSTR(s) u##s

// xplat-todo: verify below is correct
#include <cpuid.h>
inline int get_cpuid(int cpuInfo[4], int function_id)
{
    return __get_cpuid(
            static_cast<unsigned int>(function_id),
            reinterpret_cast<unsigned int*>(&cpuInfo[0]),
            reinterpret_cast<unsigned int*>(&cpuInfo[1]),
            reinterpret_cast<unsigned int*>(&cpuInfo[2]),
            reinterpret_cast<unsigned int*>(&cpuInfo[3]));
}

#define _BitScanForward BitScanForward
#define _BitScanForward64 BitScanForward64
#define _BitScanReverse BitScanReverse
#define _BitScanReverse64 BitScanReverse64
#define _bittest BitTest
#define _bittestandset BitTestAndSet
#define _interlockedbittestandset InterlockedBitTestAndSet

#ifdef PAL_STDCPP_COMPAT
// SAL.h doesn't define these if PAL_STDCPP_COMPAT is defined
// Apparently, some C++ headers will conflict with this-
// not sure which ones but stubbing them out for now in linux-
// we can revisit if we do hit a conflict
#define __in    _SAL1_Source_(__in, (), _In_)
#define __out   _SAL1_Source_(__out, (), _Out_)

#define fclose          PAL_fclose
#define fflush          PAL_fflush
#define fwprintf        PAL_fwprintf
#define wcschr          PAL_wcschr
#define wcscmp          PAL_wcscmp
#define wcslen          PAL_wcslen
#define wcsncmp         PAL_wcsncmp
#define wcsrchr         PAL_wcsrchr
#define wcsstr          PAL_wcsstr
#define wprintf         PAL_wprintf

#define stdout          PAL_stdout
#endif // PAL_STDCPP_COMPAT

#define FILE PAL_FILE

// These are not available in pal
#define fwprintf_s      fwprintf
#define wmemcmp         wcsncmp
// sprintf_s overloaded in safecrt.h. Not sure why palrt.h redefines sprintf_s.
#undef sprintf_s

// PAL LoadLibraryExW not supported
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0

// _countof
#if defined _M_X64 || defined _M_ARM || defined _M_ARM64
#define _UNALIGNED __unaligned
#else
#define _UNALIGNED
#endif

#ifdef __cplusplus
extern "C++"
{
  template <typename _CountofType, size_t _SizeOfArray>
  char(*__countof_helper(_UNALIGNED _CountofType(&_Array)[_SizeOfArray]))[_SizeOfArray];

#define __crt_countof(_Array) (sizeof(*__countof_helper(_Array)) + 0)
}
#else
#define __crt_countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifndef _countof
#define _countof __crt_countof
#endif
// _countof


//
//  Singly linked list structure. Can be used as either a list head, or
//  as link words.
//
typedef struct _SINGLE_LIST_ENTRY {
  struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

#if defined(_WIN64)

//
// The type SINGLE_LIST_ENTRY is not suitable for use with SLISTs.  For
// WIN64, an entry on an SLIST is required to be 16-byte aligned, while a
// SINGLE_LIST_ENTRY structure has only 8 byte alignment.
//
// Therefore, all SLIST code should use the SLIST_ENTRY type instead of the
// SINGLE_LIST_ENTRY type.
//

#pragma warning(push)
#pragma warning(disable:4324)   // structure padded due to align()

typedef struct DECLSPEC_ALIGN(16) _SLIST_ENTRY {
  struct _SLIST_ENTRY *Next;
} SLIST_ENTRY, *PSLIST_ENTRY;

#pragma warning(pop)

#else

typedef struct _SINGLE_LIST_ENTRY SLIST_ENTRY, *PSLIST_ENTRY;

#endif // _WIN64

#if defined(_AMD64_)

typedef union DECLSPEC_ALIGN(16) _SLIST_HEADER {
  struct {  // original struct
    ULONGLONG Alignment;
    ULONGLONG Region;
  } DUMMYSTRUCTNAME;
  struct {  // x64 16-byte header
    ULONGLONG Depth : 16;
    ULONGLONG Sequence : 48;
    ULONGLONG Reserved : 4;
    ULONGLONG NextEntry : 60; // last 4 bits are always 0's
  } HeaderX64;
} SLIST_HEADER, *PSLIST_HEADER;

#elif defined(_X86_)

typedef union _SLIST_HEADER {
  ULONGLONG Alignment;
  struct {
    SLIST_ENTRY Next;
    WORD   Depth;
    WORD   CpuId;
  } DUMMYSTRUCTNAME;
} SLIST_HEADER, *PSLIST_HEADER;

#elif defined(_ARM_)

typedef union _SLIST_HEADER {
  ULONGLONG Alignment;
  struct {
    SLIST_ENTRY Next;
    WORD   Depth;
    WORD   Reserved;
  } DUMMYSTRUCTNAME;
} SLIST_HEADER, *PSLIST_HEADER;

#endif

PALIMPORT VOID PALAPI InitializeSListHead(IN OUT PSLIST_HEADER ListHead);
PALIMPORT PSLIST_ENTRY PALAPI InterlockedPushEntrySList(IN OUT PSLIST_HEADER ListHead, IN OUT PSLIST_ENTRY  ListEntry);
PALIMPORT PSLIST_ENTRY PALAPI InterlockedPopEntrySList(IN OUT PSLIST_HEADER ListHead);

#define WRITE_WATCH_FLAG_RESET 1

#endif // _WIN32


// Use intsafe.h for internal builds (currently missing some files with stdint.h)
#if defined(_WIN32) && defined(NTBUILD)
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS 1
#include<intsafe.h>
#else
#include<stdint.h>
#endif


#ifdef _MSC_VER
// ms-specific keywords
#define _ABSTRACT abstract
// MSVC2015 does not support C++11 semantics for `typename QualifiedName` declarations
// outside of template code.
#define _TYPENAME
#else
#define _ABSTRACT
#define _TYPENAME typename
#endif

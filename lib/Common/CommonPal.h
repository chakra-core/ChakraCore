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

typedef wchar_t char16;

// xplat-todo: get a better name for this macro
#define _u(s) L##s
#define INIT_PRIORITY(x)

#define get_cpuid __cpuid

#else // !_WIN32

#define USING_PAL_STDLIB 1
#ifdef PAL_STDCPP_COMPAT
#include <math.h>
#include <time.h>
#include <smmintrin.h>
#include <xmmintrin.h>
#endif
#include "inc/pal.h"
#include "inc/rt/palrt.h"
#include "inc/rt/no_sal2.h"
#include "inc/rt/oaidl.h"

typedef char16_t char16;
#define _u(s) u##s
#define INIT_PRIORITY(x) __attribute__((init_priority(x)))

#ifdef PAL_STDCPP_COMPAT
#define __in
#define __out
#define FILE PAL_FILE
#endif

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

inline void DebugBreak()
{
    __builtin_trap();
}

#define _BitScanForward BitScanForward
#define _BitScanForward64 BitScanForward64
#define _BitScanReverse BitScanReverse
#define _BitScanReverse64 BitScanReverse64
#define _bittest BitTest
#define _bittestandset BitTestAndSet
#define _interlockedbittestandset InterlockedBitTestAndSet

#define DbgRaiseAssertionFailure() __builtin_trap()

// These are not available in pal
#define fwprintf_s      fwprintf
#define wmemcmp         wcsncmp
// sprintf_s overloaded in safecrt.h. Not sure why palrt.h redefines sprintf_s.
#undef sprintf_s

// PAL LoadLibraryExW not supported
#define LOAD_LIBRARY_SEARCH_SYSTEM32     0
// winerror.h
#define FACILITY_JSCRIPT                 2306
#define JSCRIPT_E_CANTEXECUTE            _HRESULT_TYPEDEF_(0x89020001L)
#define DISP_E_UNKNOWNINTERFACE          _HRESULT_TYPEDEF_(0x80020001L)
#define DISP_E_MEMBERNOTFOUND            _HRESULT_TYPEDEF_(0x80020003L)
#define DISP_E_UNKNOWNNAME               _HRESULT_TYPEDEF_(0x80020006L)
#define DISP_E_NONAMEDARGS               _HRESULT_TYPEDEF_(0x80020007L)
#define DISP_E_EXCEPTION                 _HRESULT_TYPEDEF_(0x80020009L)
#define DISP_E_BADINDEX                  _HRESULT_TYPEDEF_(0x8002000BL)
#define DISP_E_UNKNOWNLCID               _HRESULT_TYPEDEF_(0x8002000CL)
#define DISP_E_ARRAYISLOCKED             _HRESULT_TYPEDEF_(0x8002000DL)
#define DISP_E_BADPARAMCOUNT             _HRESULT_TYPEDEF_(0x8002000EL)
#define DISP_E_PARAMNOTOPTIONAL          _HRESULT_TYPEDEF_(0x8002000FL)
#define DISP_E_NOTACOLLECTION            _HRESULT_TYPEDEF_(0x80020011L)
#define TYPE_E_DLLFUNCTIONNOTFOUND       _HRESULT_TYPEDEF_(0x8002802FL)
#define TYPE_E_TYPEMISMATCH              _HRESULT_TYPEDEF_(0x80028CA0L)
#define TYPE_E_OUTOFBOUNDS               _HRESULT_TYPEDEF_(0x80028CA1L)
#define TYPE_E_IOERROR                   _HRESULT_TYPEDEF_(0x80028CA2L)
#define TYPE_E_CANTCREATETMPFILE         _HRESULT_TYPEDEF_(0x80028CA3L)
#define TYPE_E_CANTLOADLIBRARY           _HRESULT_TYPEDEF_(0x80029C4AL)
#define STG_E_TOOMANYOPENFILES           _HRESULT_TYPEDEF_(0x80030004L)
#define STG_E_ACCESSDENIED               _HRESULT_TYPEDEF_(0x80030005L)
#define STG_E_INSUFFICIENTMEMORY         _HRESULT_TYPEDEF_(0x80030008L)
#define STG_E_NOMOREFILES                _HRESULT_TYPEDEF_(0x80030012L)
#define STG_E_DISKISWRITEPROTECTED       _HRESULT_TYPEDEF_(0x80030013L)
#define STG_E_READFAULT                  _HRESULT_TYPEDEF_(0x8003001EL)
#define STG_E_SHAREVIOLATION             _HRESULT_TYPEDEF_(0x80030020L)
#define STG_E_LOCKVIOLATION              _HRESULT_TYPEDEF_(0x80030021L)
#define STG_E_MEDIUMFULL                 _HRESULT_TYPEDEF_(0x80030070L)
#define STG_E_INVALIDNAME                _HRESULT_TYPEDEF_(0x800300FCL)
#define STG_E_INUSE                      _HRESULT_TYPEDEF_(0x80030100L)
#define STG_E_NOTCURRENT                 _HRESULT_TYPEDEF_(0x80030101L)
#define STG_E_CANTSAVE                   _HRESULT_TYPEDEF_(0x80030103L)
#define REGDB_E_CLASSNOTREG              _HRESULT_TYPEDEF_(0x80040154L)
#define MK_E_UNAVAILABLE                 _HRESULT_TYPEDEF_(0x800401E3L)
#define MK_E_INVALIDEXTENSION            _HRESULT_TYPEDEF_(0x800401E6L)
#define MK_E_CANTOPENFILE                _HRESULT_TYPEDEF_(0x800401EAL)
#define CO_E_APPNOTFOUND                 _HRESULT_TYPEDEF_(0x800401F5L)
#define CO_E_APPDIDNTREG                 _HRESULT_TYPEDEF_(0x800401FEL)
#define GetScode(hr) ((SCODE) (hr))
// activscp.h
#define SCRIPT_E_RECORDED                _HRESULT_TYPEDEF_(0x86664004L)


typedef
enum tagBREAKPOINT_STATE
    {
        BREAKPOINT_DELETED  = 0,
        BREAKPOINT_DISABLED = 1,
        BREAKPOINT_ENABLED  = 2
    }   BREAKPOINT_STATE;

typedef
enum tagBREAKRESUME_ACTION
    {
        BREAKRESUMEACTION_ABORT     = 0,
        BREAKRESUMEACTION_CONTINUE  = ( BREAKRESUMEACTION_ABORT + 1 ) ,
        BREAKRESUMEACTION_STEP_INTO = ( BREAKRESUMEACTION_CONTINUE + 1 ) ,
        BREAKRESUMEACTION_STEP_OVER = ( BREAKRESUMEACTION_STEP_INTO + 1 ) ,
        BREAKRESUMEACTION_STEP_OUT  = ( BREAKRESUMEACTION_STEP_OVER + 1 ) ,
        BREAKRESUMEACTION_IGNORE    = ( BREAKRESUMEACTION_STEP_OUT + 1 ) ,
        BREAKRESUMEACTION_STEP_DOCUMENT = ( BREAKRESUMEACTION_IGNORE + 1 )
    }   BREAKRESUMEACTION;


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

#define ARRAYSIZE(A) _countof(A)

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


template <class T>
inline T InterlockedExchangeAdd(
    IN OUT T volatile *Addend,
    IN T Value)
{
    return __sync_fetch_and_add(Addend, Value);
}

template <class T>
inline T InterlockedExchangeSubtract(
    IN OUT T volatile *Addend,
    IN T Value)
{
    return __sync_fetch_and_sub(Addend, Value);
}

template <class T>
inline T InterlockedIncrement(
    IN OUT T volatile *Addend)
{
    return __sync_add_and_fetch(Addend, T(1));
}

template <class T>
inline T InterlockedDecrement(
    IN OUT T volatile *Addend)
{
    return __sync_sub_and_fetch(Addend, T(1));
}

inline __int64 _abs64(__int64 n)
{
    return n < 0 ? -n : n;
}

// xplat-todo: implement these for JIT and Concurrent/Partial GC
uintptr_t _beginthreadex(
   void *security,
   unsigned stack_size,
   unsigned ( __stdcall *start_address )( void * ),
   void *arglist,
   unsigned initflag,
   unsigned *thrdaddr);
BOOL WINAPI GetModuleHandleEx(
  _In_     DWORD   dwFlags,
  _In_opt_ LPCTSTR lpModuleName,
  _Out_    HMODULE *phModule
);

// xplat-todo: implement this function to get the stack bounds of the current
// thread
// For Linux, we could use pthread_getattr_np to get the stack limit (end)
// and then use the stack size to calculate the stack base
int GetCurrentThreadStackBounds(char** stackBase, char** stackEnd);

errno_t rand_s(unsigned int* randomValue);
errno_t __cdecl _ultow_s(unsigned _Value, WCHAR *_Dst, size_t _SizeInWords, int _Radix);
errno_t __cdecl _ui64tow_s(unsigned __int64 _Value, WCHAR *_Dst, size_t _SizeInWords, int _Radix);
void __cdecl qsort_s(void *base, size_t num, size_t width,
             int (__cdecl *compare )(void *, const void *, const void *),
             void * context
);
char16* __cdecl wmemset(char16* wcs, char16 wc, size_t n);
DWORD __cdecl CharLowerBuffW(const char16* lpsz, DWORD  cchLength);
DWORD __cdecl CharUpperBuffW(const char16* lpsz, DWORD  cchLength);

#define MAXUINT32   ((uint32_t)~((uint32_t)0))
#define MAXINT32    ((int32_t)(MAXUINT32 >> 1))
#define BYTE_MAX    0xff
#define USHORT_MAX  0xffff

#ifdef UNICODE
#define StringCchPrintf  StringCchPrintfW
#endif

#endif // _WIN32


// Use intsafe.h for internal builds (currently missing some files with stdint.h)
#if defined(_WIN32) && defined(NTBUILD)
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS 1
#include <intsafe.h>
#else
#include <stdint.h>
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

#if (defined(_MSC_VER) && _MSC_VER < 1900) || defined(NTBUILD)
// "noexcept" not supported before VS 2015
#define _NOEXCEPT_ throw()
#else
#define _NOEXCEPT_ noexcept
#endif

#ifdef _MSC_VER
extern "C" PVOID _ReturnAddress(VOID);
#pragma intrinsic(_ReturnAddress)
#else
#define _ReturnAddress() __builtin_return_address(0)
#endif

// xplat-todo: figure out why strsafe.h includes stdio etc
// which prevents me from directly including PAL's strsafe.h
#ifdef __cplusplus
#define _STRSAFE_EXTERN_C    extern "C"
#else
#define _STRSAFE_EXTERN_C    extern
#endif

// If you do not want to use these functions inline (and instead want to link w/ strsafe.lib), then
// #define STRSAFE_LIB before including this header file.
#if defined(STRSAFE_LIB)
#define STRSAFEAPI  _STRSAFE_EXTERN_C HRESULT __stdcall
#pragma comment(lib, "strsafe.lib")
#elif defined(STRSAFE_LIB_IMPL)
#define STRSAFEAPI  _STRSAFE_EXTERN_C HRESULT __stdcall
#else
#define STRSAFEAPI  __inline HRESULT __stdcall
#define STRSAFE_INLINE
#endif

STRSAFEAPI StringCchPrintfW(WCHAR* pszDest, size_t cchDest, const WCHAR* pszFormat, ...);

template <class TryFunc, class FinallyFunc>
void TryFinally(const TryFunc& tryFunc, const FinallyFunc& finallyFunc)
{
    bool hasException = true;
    try
    {
        tryFunc();
        hasException = false;
    }
    catch(...)
    {
        finallyFunc(hasException);
        throw;
    }

    finallyFunc(hasException);
}

__inline
HRESULT ULongMult(ULONG ulMultiplicand, ULONG ulMultiplier, ULONG* pulResult);

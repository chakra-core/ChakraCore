//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//

/*++



Module Name:

    misc/bstr.cpp

Abstract:

    Implementation of bstr related functionality

--*/

#include "pal/palinternal.h"

// Redefining here to not have to pull in all of palrt.h in order to get intsafe
#define S_OK ((HRESULT)0x00000000L)
#define E_INVALIDARG ((HRESULT) 0x80070057L)
#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#define FAILED(Status) ((HRESULT)(Status)<0)
#define STDAPICALLTYPE       __stdcall
#define NULL    0
#define STDAPI_(type)        EXTERN_C type STDAPICALLTYPE

#include "pal_assert.h"
#include "rt/intsafe.h"

#define WIN32_ALLOC_ALIGN (16 - 1)

PALIMPORT size_t __cdecl PAL_wcslen(const WCHAR *);

typedef WCHAR OLECHAR;
typedef WCHAR *BSTR;

inline HRESULT CbSysStringSize(ULONG cchSize, BOOL isByteLen, ULONG *result)
{
    if (result == nullptr)
        return E_INVALIDARG;

    // +2 for the null terminator
    // + DWORD_PTR to store the byte length of the string
    int constant = sizeof(WCHAR) + sizeof(DWORD_PTR) + WIN32_ALLOC_ALIGN;

    if (isByteLen)
    {
        if (SUCCEEDED(ULongAdd(constant, cchSize, result)))
        {
            *result = *result & ~WIN32_ALLOC_ALIGN;
            return NOERROR;
        }
    }
    else
    {
        ULONG temp = 0; // should not use in-place addition in ULongAdd
        if (SUCCEEDED(ULongMult(cchSize, sizeof(WCHAR), &temp)) &
            SUCCEEDED(ULongAdd(temp, constant, result)))
        {
            *result = *result & ~WIN32_ALLOC_ALIGN;
            return NOERROR;
        }
    }
    return INTSAFE_E_ARITHMETIC_OVERFLOW;
}



/***
 *BSTR SysAllocStringLen(char*, unsigned int)
 *Purpose:
 *  Allocation a bstr of the given length and initialize with
 *  the pasted in string
 *
 *Entry:
 *  [optional]
 *
 *Exit:
 *  return value = BSTR, NULL if the allocation failed.
 *
 ***********************************************************************/
STDAPI_(BSTR) SysAllocStringLen(const OLECHAR *psz, UINT len)
{

    BSTR bstr;
    DWORD cbTotal = 0;

    if (FAILED(CbSysStringSize(len, FALSE, &cbTotal)))
        return NULL;

    bstr = (OLECHAR *)HeapAlloc(GetProcessHeap(), 0, cbTotal);

    if(bstr != NULL) {

#if defined(_WIN64)
        // NOTE: There are some apps which peek back 4 bytes to look at
        // the size of the BSTR. So, in case of 64-bit code,
        // we need to ensure that the BSTR length can be found by
        // looking one DWORD before the BSTR pointer.
        *(DWORD_PTR *)bstr = (DWORD_PTR) 0;
        bstr = (BSTR) ((char *) bstr + sizeof (DWORD));
#endif
        *(DWORD FAR*)bstr = (DWORD)len * sizeof(OLECHAR);

        bstr = (BSTR) ((char*) bstr + sizeof(DWORD));

        if(psz != NULL){
            memcpy(bstr, psz, len * sizeof(OLECHAR));
        }

        bstr[len] = '\0'; // always 0 terminate
    }

    return bstr;
}

/***
 *void SysFreeString(BSTR)
 *Purpose:
 *  Free a bstr using the passed in string
 *
 *Entry:
 *  BSTR to free
 *
 *Exit:
 *  return value = void
 *
 ***********************************************************************/
STDAPI_(void) SysFreeString(BSTR bstr)
{
    if (bstr != NULL)
    {
        bstr = (BSTR) ((char*) bstr - sizeof(DWORD));
#if defined(_WIN64)
        bstr = (BSTR) ((char*) bstr - sizeof(DWORD));
#endif
        HeapFree(GetProcessHeap(), 0, (LPVOID) bstr);
    }
}

/***
 *UINT SysStringLen(BSTR)
 *Purpose:
 *  Return the length of the string in characters (not including null terminator)
 *
 *Entry:
 *  BSTR whose length to return
 *
 *Exit:
 *  return value = length of the string
 *
 ***********************************************************************/
STDAPI_(UINT) SysStringLen(BSTR bstr)
{
    if (bstr == NULL)
    {
        return 0;
    }

    return (UINT)((((DWORD FAR*)bstr)[-1]) / sizeof(OLECHAR));
}

/***
 *BSTR SysAllocString(char*)
 *Purpose:
 *  Allocation a bstr using the passed in string
 *
 *Entry:
 *  String to create a bstr for
 *
 *Exit:
 *  return value = BSTR, NULL if allocation failed
 *
 ***********************************************************************/
STDAPI_(BSTR) SysAllocString(const OLECHAR* psz)
{
    if(psz == NULL)
    {
        return NULL;
    }

    return SysAllocStringLen(psz, (DWORD)PAL_wcslen(psz));
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"

// PUBLIC ERROR codes

// verify HR is as-expected for the Legacy (private) error JSERR_CantExecute
C_ASSERT(JSCRIPT_E_CANTEXECUTE != JSERR_CantExecute);
// verify the HR value (as MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0x1393))
C_ASSERT(JSERR_CantExecute == 0x800A1393);

// verify HR matches between public SDK and private (.h) files
C_ASSERT(JSCRIPT_E_CANTEXECUTE == JSPUBLICERR_CantExecute);
// verify the HR value (as MAKE_HRESULT(SEVERITY_ERROR, FACILITY_JSCRIPT, 0x0001))
C_ASSERT(JSPUBLICERR_CantExecute == _HRESULT_TYPEDEF_(0x89020001L));

// /PUBLIC ERROR codes

// boundary check - all errNum should be capped to 10,000 (RTERROR_STRINGFORMAT_OFFSET) - except for VBSERR_CantDisplayDate==32812
#define VERIFY_BOUNDARY_ERRNUM(name,errnum) C_ASSERT(name == VBSERR_CantDisplayDate || errnum < RTERROR_STRINGFORMAT_OFFSET);

#define RT_ERROR_MSG(name, errnum, str1, str2, jst, errorNumSource) VERIFY_BOUNDARY_ERRNUM(name, errnum)
#define RT_PUBLICERROR_MSG(name, errnum, str1, str2, jst, errorNumSource) VERIFY_BOUNDARY_ERRNUM(name, errnum)
#include "rterrors.h"
#undef  RT_PUBLICERROR_MSG
#undef  RT_ERROR_MSG

//------------------------------------------------------------------------------
// xplat: simulate resource strings
#ifndef _WIN32
#include <rterrors_limits.h>

struct _ResourceStr
{
    UINT id;
    const char16* str;
};

static _ResourceStr s_resourceStrs[] =
{
    //
    // Copy from jserr.gen
    //
#define RT_ERROR_MSG(name, errnum, str1, str2, jst, errorNumSource) \
    { errnum, _u(str2) },
#define RT_PUBLICERROR_MSG(name, errnum, str1, str2, jst, errorNumSource) \
    { errnum + RTERROR_PUBLIC_RESOURCEOFFSET, _u(str2) },
#include <rterrors.h>
#undef RT_PUBLICERROR_MSG
#undef RT_ERROR_MSG

#define RT_ERROR_MSG(name, errnum, str1, str2, jst, errorNumSource) \
    { errnum + RTERROR_STRINGFORMAT_OFFSET, _u(str1) },
#define RT_PUBLICERROR_MSG(name, errnum, str1, str2, jst, errorNumSource) \
    { errnum + RTERROR_STRINGFORMAT_OFFSET + RTERROR_PUBLIC_RESOURCEOFFSET, \
      _u(str1) },
#include <rterrors.h>
#undef RT_PUBLICERROR_MSG
#undef RT_ERROR_MSG

#define LSC_ERROR_MSG(errnum, name, str) \
    { errnum, _u(str) },
#include <perrors.h>
#undef LSC_ERROR_MSG

    { IDS_COMPILATION_ERROR_SOURCE, _u("JavaScript compilation error") },
    { IDS_RUNTIME_ERROR_SOURCE,     _u("JavaScript runtime error") },
    { IDS_UNKNOWN_RUNTIME_ERROR,    _u("Unknown runtime error") },

    { IDS_INFINITY,                 _u("Infinity") },
    { IDS_MINUSINFINITY,            _u("-Infinity") }
};

static int compare_ResourceStr(const void* a, const void* b)
{
    UINT id1 = reinterpret_cast<const _ResourceStr*>(a)->id;
    UINT id2 = reinterpret_cast<const _ResourceStr*>(b)->id;
    return id1 - id2;
}

static bool s_resourceStrsSorted = false;

const char16* LoadResourceStr(UINT id)
{
    if (!s_resourceStrsSorted)
    {
        qsort(s_resourceStrs,
            _countof(s_resourceStrs), sizeof(s_resourceStrs[0]),
            compare_ResourceStr);
        s_resourceStrsSorted = true;
    }

    _ResourceStr key = { id, nullptr };
    const void* p = bsearch(&key,
        s_resourceStrs, _countof(s_resourceStrs), sizeof(s_resourceStrs[0]),
        compare_ResourceStr);

    return p ? reinterpret_cast<const _ResourceStr*>(p)->str : nullptr;
}

#endif

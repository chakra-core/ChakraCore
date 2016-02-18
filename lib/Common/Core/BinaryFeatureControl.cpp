//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCorePch.h"
#include "Core/BinaryFeatureControl.h"

//For making direct call in release binaries.
#if !defined(DELAYLOAD_SET_CFG_TARGET)

#ifdef _WIN32
extern "C"
WINBASEAPI
BOOL
WINAPI
GetProcessMitigationPolicy(
__in HANDLE hProcess,
__in PROCESS_MITIGATION_POLICY MitigationPolicy,
__out_bcount(nLength) PVOID lpBuffer,
__in SIZE_T nLength
);
#endif // ENABLE_DEBUG_CONFIG_OPTIONS
#endif

bool
BinaryFeatureControl::RecyclerTest()
{
#ifdef RECYCLER_TEST
    return true;
#else
    return false;
#endif
}

#ifdef _WIN32
BOOL
BinaryFeatureControl::GetMitigationPolicyForProcess(__in HANDLE hProcess, __in PROCESS_MITIGATION_POLICY MitigationPolicy, __out_bcount(nLength) PVOID lpBuffer, __in SIZE_T nLength)
{
#if !defined(DELAYLOAD_SET_CFG_TARGET)
    return GetProcessMitigationPolicy(hProcess, MitigationPolicy, lpBuffer, nLength);
#else
    return FALSE;
#endif
}
#endif

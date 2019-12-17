//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonCorePch.h"

#ifdef _WIN32

#include <VersionHelpers.h>


CriticalSection GlobalSecurityPolicy::s_policyCS;
GlobalSecurityPolicy GlobalSecurityObject;

#pragma section(".mrdata", read)

// Note:  'volatile' is necessary here otherwise the compiler assumes these are constants initialized to '0' and will constant propagate them...
__declspec(allocate(".mrdata")) volatile GlobalSecurityPolicy::ReadOnlyData GlobalSecurityPolicy::readOnlyData =
    {
#if defined(_CONTROL_FLOW_GUARD)
        nullptr,
        nullptr,
#endif
        false,
        false,
        false
    };

bool
GlobalSecurityPolicy::IsCFGEnabled()
{
    return readOnlyData.isCFGEnabled && !PHASE_OFF1(Js::CFGPhase);
}

bool 
GlobalSecurityPolicy::InitIsCFGEnabled()
{
#if defined(_CONTROL_FLOW_GUARD)
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY CfgPolicy;
    BOOL isGetMitigationPolicySucceeded = GlobalSecurityPolicy::GetMitigationPolicyForProcess(
        GetCurrentProcess(),
        ProcessControlFlowGuardPolicy,
        &CfgPolicy,
        sizeof(CfgPolicy));
    AssertOrFailFast(isGetMitigationPolicySucceeded);
    return CfgPolicy.EnableControlFlowGuard;

#else
    return false;
#endif // _CONTROL_FLOW_GUARD
}

GlobalSecurityPolicy::GlobalSecurityPolicy()
{
#if defined(_CONTROL_FLOW_GUARD)
    AutoCriticalSection autocs(&s_policyCS);
    DWORD oldProtect;

    // Make sure this is called only once
    AssertOrFailFast(!readOnlyData.isInitialized);

#if defined(CHAKRA_CORE_DOWN_COMPAT)
    if (AutoSystemInfo::Data.IsWinThresholdOrLater())
#endif
    {
        // Make readOnlyData read-write
        BOOL res = VirtualProtect((LPVOID)&readOnlyData, sizeof(ReadOnlyData), PAGE_READWRITE, &oldProtect);
        if ((res == FALSE) || (oldProtect != PAGE_READONLY))
        {
            RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
        }

        readOnlyData.isInitialized = true;

        EnsureFromSystemDirOnly();

        if (m_hModule)
        {
            readOnlyData.pfnGetProcessMitigationPolicy = (PFNCGetMitigationPolicyForProcess)GetFunction("GetProcessMitigationPolicy");
            if (readOnlyData.pfnGetProcessMitigationPolicy == nullptr)
            {
                RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
            }

            readOnlyData.isCFGEnabled = InitIsCFGEnabled();

            if (readOnlyData.isCFGEnabled)
            {
                readOnlyData.pfnSetProcessValidCallTargets = (PFNCSetProcessValidCallTargets)GetFunction("SetProcessValidCallTargets");
                if (readOnlyData.pfnSetProcessValidCallTargets == nullptr)
                {
                    RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
                }
            }
        }

        // Make readOnlyData read-only again.
        res = VirtualProtect((LPVOID)&readOnlyData, sizeof(ReadOnlyData), PAGE_READONLY, &oldProtect);
        if ((res == FALSE) || (oldProtect != PAGE_READWRITE))
        {
            RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
        }
    }

#endif //_CONTROL_FLOW_GUARD
    }

void
GlobalSecurityPolicy::DisableSetProcessValidCallTargets()
{
    // One-way transition from allowing SetProcessValidCallTargets to disabling
    // the API.
    if (!readOnlyData.disableSetProcessValidCallTargets)
    {
        AutoCriticalSection autocs(&s_policyCS);
        DWORD oldProtect;

        BOOL res = VirtualProtect((LPVOID)&readOnlyData, sizeof(ReadOnlyData), PAGE_READWRITE, &oldProtect);
        if ((res == FALSE) || (oldProtect != PAGE_READONLY))
        {
            RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
        }
    
        readOnlyData.disableSetProcessValidCallTargets = true;
    
        res = VirtualProtect((LPVOID)&readOnlyData, sizeof(ReadOnlyData), PAGE_READONLY, &oldProtect);
        if ((res == FALSE) || (oldProtect != PAGE_READWRITE))
        {
            RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
        }
    }
}

bool
GlobalSecurityPolicy::IsSetProcessValidCallTargetsAllowed()
{
    return !readOnlyData.disableSetProcessValidCallTargets;
}

#if defined(_CONTROL_FLOW_GUARD)
BOOL 
DECLSPEC_GUARDNOCF GlobalSecurityPolicy::GetMitigationPolicyForProcess(HANDLE hProcess, PROCESS_MITIGATION_POLICY mitigationPolicy, PVOID lpBuffer, SIZE_T dwLength)
{
    return GlobalSecurityPolicy::readOnlyData.pfnGetProcessMitigationPolicy(hProcess, mitigationPolicy, lpBuffer, dwLength);
}

BOOL
DECLSPEC_GUARDNOCF GlobalSecurityPolicy::SetProcessValidCallTargets(HANDLE hProcess, PVOID virtualAddress, SIZE_T regionSize, ULONG numberOfOffsets, PCFG_CALL_TARGET_INFO offsetInformation)
{
    return GlobalSecurityPolicy::readOnlyData.pfnSetProcessValidCallTargets(hProcess, virtualAddress, regionSize, numberOfOffsets, offsetInformation);
}
#endif //_CONTROL_FLOW_GUARD
#endif // _WIN32

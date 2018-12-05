//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "DelayLoadLibrary.h"

class GlobalSecurityPolicy : private DelayLoadLibrary
{
public:
    typedef BOOL FNCGetMitigationPolicyForProcess(HANDLE, PROCESS_MITIGATION_POLICY, PVOID, SIZE_T);
    typedef FNCGetMitigationPolicyForProcess* PFNCGetMitigationPolicyForProcess;

    typedef BOOL FNCSetProcessValidCallTargets(HANDLE, PVOID, SIZE_T, ULONG, PCFG_CALL_TARGET_INFO);
    typedef FNCSetProcessValidCallTargets* PFNCSetProcessValidCallTargets;

    GlobalSecurityPolicy();

    static void DisableSetProcessValidCallTargets();
    static bool IsSetProcessValidCallTargetsAllowed();
    static bool IsCFGEnabled();
    static FNCGetMitigationPolicyForProcess GetMitigationPolicyForProcess;
    static FNCSetProcessValidCallTargets SetProcessValidCallTargets;

    LPCTSTR GetLibraryName() const { return _u("api-ms-win-core-memory-l1-1-3.dll"); }

private:
    static CriticalSection s_policyCS;

    volatile static struct ReadOnlyData {
        PFNCGetMitigationPolicyForProcess pfnGetProcessMitigationPolicy;
        PFNCSetProcessValidCallTargets pfnSetProcessValidCallTargets;
        bool disableSetProcessValidCallTargets;
        bool isCFGEnabled;
        bool isInitialized;
    } readOnlyData;

    static bool InitIsCFGEnabled();
};

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonCorePch.h"

#pragma section(".mrdata", read)

CriticalSection GlobalSecurityPolicy::s_policyCS;

__declspec(allocate(".mrdata"))
volatile bool GlobalSecurityPolicy::s_ro_disableSetProcessValidCallTargets = false;

void
GlobalSecurityPolicy::DisableSetProcessValidCallTargets()
{
    // One-way transition from allowing SetProcessValidCallTargets to disabling
    // the API.
    if (!s_ro_disableSetProcessValidCallTargets)
    {
        AutoCriticalSection autocs(&s_policyCS);
        DWORD oldProtect;

        BOOL res = VirtualProtect((LPVOID)&s_ro_disableSetProcessValidCallTargets, sizeof(s_ro_disableSetProcessValidCallTargets), PAGE_READWRITE, &oldProtect);
        if ((res == FALSE) || (oldProtect != PAGE_READONLY))
        {
            RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
        }
    
        s_ro_disableSetProcessValidCallTargets = true;
    
        res = VirtualProtect((LPVOID)&s_ro_disableSetProcessValidCallTargets, sizeof(s_ro_disableSetProcessValidCallTargets), PAGE_READONLY, &oldProtect);
        if ((res == FALSE) || (oldProtect != PAGE_READWRITE))
        {
            RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
        }
    }
}

bool
GlobalSecurityPolicy::IsSetProcessValidCallTargetsAllowed()
{
    return !s_ro_disableSetProcessValidCallTargets;
}

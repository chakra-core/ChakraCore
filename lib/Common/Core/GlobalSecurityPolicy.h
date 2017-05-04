//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class GlobalSecurityPolicy
{
public:
    static void DisableSetProcessValidCallTargets();
    static bool IsSetProcessValidCallTargetsAllowed();

private:
    static CriticalSection s_policyCS;

    static volatile bool s_ro_disableSetProcessValidCallTargets;
};

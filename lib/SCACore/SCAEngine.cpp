//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "SCACorePch.h"
#ifdef ENABLE_SCRIPT_DEBUGGING
#include "Debug/ProbeContainer.h"
#include "Debug/DebugContext.h"
#endif

namespace Js
{
    void ScriptContextHolder::ThrowIfFailed(HRESULT hr) const
    {
        if (FAILED(hr))
        {
            m_scriptContext->GetHostScriptContext()->ThrowIfFailed(hr);

            // No debugger support yet.
        }
    }
};

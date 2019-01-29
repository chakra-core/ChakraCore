//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "SCACorePch.h"
#include "Debug\ProbeContainer.h"
#include "Debug\DebugContext.h"

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
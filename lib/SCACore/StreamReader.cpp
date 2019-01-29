//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "SCACorePch.h"

namespace Js
{
    void StreamReader::Read(void* pv, size_t cb)
    {
        Assert(cb < m_length);
        // Read from buffer
        js_memcpy_s(pv, cb, m_buffer + m_current, cb);
        m_current += cb;
    }

    Var StreamReader::ReadHostObject()
    {
        Var object = nullptr;
        ScriptContext* scriptContext = GetScriptContext();
        BEGIN_LEAVE_SCRIPT(scriptContext)
        {
            object = m_stream->ReadHostObject();
        }
        END_LEAVE_SCRIPT(scriptContext);
        return object;
    }

    //
    // Overload to count for buffer position.
    //
    scaposition_t StreamReader::GetPosition() const
    {
        return static_cast<scaposition_t>(m_current);
    }
}

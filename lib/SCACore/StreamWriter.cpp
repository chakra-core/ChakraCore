//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "SCACorePch.h"

namespace Js
{
    void StreamWriter::Write(const void* pv, size_t cb)
    {
        ScriptContext* scriptContext = GetScriptContext();
        uint32 newSize = UInt32Math::Add((uint32)m_current, (uint32)cb);
        if (newSize >= m_capacity)
        {
            size_t newCapacity = UInt32Math::Add(max(newSize, UInt32Math::Mul((uint32)m_capacity, 2)), 100);
            BEGIN_LEAVE_SCRIPT(scriptContext)
            {
                m_buffer = m_stream->ExtendBuffer(m_buffer, newCapacity, &m_capacity);
            }
            END_LEAVE_SCRIPT(scriptContext);
        }

        Assert(m_buffer != nullptr);
        js_memcpy_s(m_buffer + m_current, cb, pv, cb);
        m_current += cb;
    }

    void StreamWriter::WriteHostObject(void* data)
    {
        ScriptContext* scriptContext = GetScriptContext();
        BEGIN_LEAVE_SCRIPT(scriptContext)
        {
            m_stream->WriteHostObject(data);
        }
        END_LEAVE_SCRIPT(scriptContext);
    }

    //
    // Overload to count for buffer position.
    //
    scaposition_t StreamWriter::GetPosition() const
    {
        // If this overflows, we will throw during Flush/RealWrite. So skip checking here.
        return static_cast<scaposition_t>(m_current);
    }
}

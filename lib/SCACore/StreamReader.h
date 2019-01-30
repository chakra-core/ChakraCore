//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace Js
{
    //
    // A simple stream reader that hides low level stream details.
    //
    class StreamReader: public ScriptContextHolder
    {
    private:
        HostReadStream *m_stream;
        byte *m_buffer;
        size_t m_current;
        size_t m_length;

        //
        // Get number of bytes left for read in the buffer.
        //
        size_t GetBytesInBuffer() const
        {
            return m_length - m_current;
        }

        // ULONG RealRead(void* pv, ULONG cb);

    public:
        StreamReader(ScriptContext* scriptContext, byte* buffer, size_t length, HostReadStream *stream)
            : ScriptContextHolder(scriptContext),
            m_stream(stream),
            m_buffer(buffer),
            m_current(0),
            m_length(length)
        {
        }

        Var ReadHostObject();
        void Read(void* pv, size_t cb);

        void ReadRawBytes(void** pv, size_t cb)
        {
            Assert(cb < m_length);
            *pv = (m_buffer + m_current);
            m_current += cb;
        }

        template <typename T>
        void Read(T* value)
        {
            if (GetBytesInBuffer() >= sizeof(T))
            {
                *value = *(T*)(m_buffer + m_current);
                m_current += sizeof(T);
            }
            else
            {
                Read(value, sizeof(T));
            }
        }

        scaposition_t GetPosition() const;
    };

}

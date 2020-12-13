//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace Js
{
    //
    // A simple stream writer that hides low level stream details.
    //
    // Note:
    //      This stream writer uses an internal buffer. Must call Flush() at the end to ensure
    //      any remained content in the internal buffer is sent to the output stream.
    //
    class StreamWriter: public ScriptContextHolder
    {
    private:
        HostStream *m_stream;
        byte *m_buffer;
        size_t m_current;
        size_t m_capacity;

    public:
        StreamWriter(ScriptContext* scriptContext, HostStream* stream)
            : ScriptContextHolder(scriptContext),
            m_stream(stream),
            m_buffer(nullptr),
            m_current(0),
            m_capacity(0)
        {
        }

        byte* GetBuffer() { return m_buffer; }
        size_t GetLength() { return m_current; }

        void Write(const void* pv, size_t cb);
        void WriteHostObject(void* data);

        template <typename T>
        void Write(const T& value)
        {
            if ((m_current + sizeof(T)) < m_capacity)
            {
                *(T*)(m_buffer + m_current) = value;
                m_current += sizeof(T);
            }
            else
            {
                Write(&value, sizeof(T));
            }
        }


        //_Post_satisfies_(m_cur == 0)
        //void Flush();

        scaposition_t GetPosition() const;
    };
}

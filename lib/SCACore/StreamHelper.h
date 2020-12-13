//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace Js
{
    //
    // Helper class to implement stream reader/writer. Note that this stream helper class
    // maintains its own stream position and ensures the stream position fits scaposition_t.
    //
    class StreamHelper: public ScriptContextHolder
    {
    private:
        HostStream *m_stream;

    protected:
        HostStream* GetStream() const
        {
            return m_stream;
        }

        void ThrowOverflow() const
        {
            ::Math::DefaultOverflowPolicy();
        }

    public:
        StreamHelper(ScriptContext* scriptContext, HostStream* stream)
            : ScriptContextHolder(scriptContext),
            m_stream(stream)
        {
        }
    };
}

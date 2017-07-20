//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ENABLE_WASM
namespace Js
{
    class WebAssemblySource
    {
        BYTE* buffer;
        uint bufferLength;
        Js::Utf8SourceInfo* sourceInfo;
    public:
        WebAssemblySource(Var source, bool createNewContext, ScriptContext* scriptContext);
        // Caller is responsible to make a copy of the buffer source
        WebAssemblySource(BYTE* source, uint bufferLength, bool createNewContext, ScriptContext* scriptContext);

        BYTE* GetBuffer() const { return buffer; }
        uint GetBufferLength() const { return bufferLength; }
        Js::Utf8SourceInfo* GetSourceInfo() const { return sourceInfo; }
    private:
        void ReadBufferSource(Var val, ScriptContext* scriptContext);
        void CreateSourceInfo(bool createNewContext, ScriptContext* scriptContext);
    };
}
#endif
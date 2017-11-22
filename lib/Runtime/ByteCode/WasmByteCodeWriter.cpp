//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

#ifdef ENABLE_WASM
#include "WasmByteCodeWriter.h"

#define WASM_BYTECODE_WRITER
#define AsmJsByteCodeWriter WasmByteCodeWriter
#include "ByteCode/AsmJsByteCodeWriter.cpp"
#undef WASM_BYTECODE_WRITER
#undef AsmJsByteCodeWriter

namespace Js
{
void WasmByteCodeWriter::Create() 
{
    ByteCodeWriter::Create();
}
void WasmByteCodeWriter::End()
{
    ByteCodeWriter::End();
}
void WasmByteCodeWriter::Reset()
{
    ByteCodeWriter::Reset();
}
void WasmByteCodeWriter::Begin(FunctionBody* functionWrite, ArenaAllocator* alloc)
{
    ByteCodeWriter::Begin(functionWrite, alloc, true, true, false);
}
ByteCodeLabel WasmByteCodeWriter::DefineLabel()
{
    return ByteCodeWriter::DefineLabel();
}
void WasmByteCodeWriter::SetCallSiteCount(Js::ProfileId callSiteCount)
{
    ByteCodeWriter::SetCallSiteCount(callSiteCount);
}

uint32 WasmByteCodeWriter::WasmLoopStart(ByteCodeLabel loopEntrance, __in_ecount(WAsmJs::LIMIT) RegSlot* curRegs)
{
    uint loopId = m_functionWrite->IncrLoopCount();
    Assert((uint)m_loopHeaders->Count() == loopId);

    m_loopHeaders->Add(LoopHeaderData(m_byteCodeData.GetCurrentOffset(), 0, m_loopNest > 0));
    m_loopNest++;
    this->MarkAsmJsLabel(loopEntrance);
    MULTISIZE_LAYOUT_WRITE(WasmLoopStart, Js::OpCodeAsmJs::WasmLoopBodyStart, loopId, curRegs);

    return loopId;
}

template <typename SizePolicy>
bool WasmByteCodeWriter::TryWriteWasmLoopStart(OpCodeAsmJs op, uint loopId, __in_ecount(WAsmJs::LIMIT) RegSlot* curRegs)
{
    OpLayoutT_WasmLoopStart<SizePolicy> layout;
    if (SizePolicy::Assign(layout.loopId, loopId))
    {
        for (WAsmJs::Types type = WAsmJs::Types(0); type != WAsmJs::LIMIT; type = WAsmJs::Types(type + 1))
        {
            if (!SizePolicy::Assign(layout.curRegs[type], curRegs[type]))
            {
                return false;
            }
        }

        m_byteCodeData.EncodeT<SizePolicy::LayoutEnum>(op, &layout, sizeof(layout), this);
        return true;
    }
    return false;
}

}

#endif

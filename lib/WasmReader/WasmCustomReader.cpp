//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
namespace Wasm
{

WasmCustomReader::WasmCustomReader(ArenaAllocator* alloc) : m_nodes(alloc)
{

}

void WasmCustomReader::SeekToFunctionBody(class WasmFunctionInfo* funcInfo)
{
    Assert(funcInfo->GetCustomReader() == this);
    m_state = 0;
}

bool WasmCustomReader::IsCurrentFunctionCompleted() const
{
    return m_state >= (uint32)m_nodes.Count();
}

WasmOp WasmCustomReader::ReadExpr()
{
    if (m_state < (uint32)m_nodes.Count())
    {
        m_currentNode = m_nodes.Item(m_state++);
        return m_currentNode.op;
    }
    return wbEnd;
}

void WasmCustomReader::FunctionEnd()
{
}

void WasmCustomReader::AddNode(WasmNode node)
{
    m_nodes.Add(node);
}

uint32 WasmCustomReader::EstimateCurrentFunctionBytecodeSize() const
{
    uint32 count = min((uint32)m_nodes.Count(), (uint32)AutoSystemInfo::PageSize);
    // On average each node takes between 3 - 7 bytes to encode
    return count * 5;
}

};
#endif

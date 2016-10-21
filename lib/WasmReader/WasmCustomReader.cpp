//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
namespace Wasm
{

WasmCustomReader::WasmCustomReader(ArenaAllocator* alloc):
    m_nodes(alloc), m_iNode(0)
{

}

void
WasmCustomReader::SeekToFunctionBody(FunctionBodyReaderInfo readerInfo)
{
    // Reset state
    m_iNode = 0;
}

bool
WasmCustomReader::IsCurrentFunctionCompleted() const
{
    return m_iNode >= (uint32)m_nodes.Count();
}

WasmOp
WasmCustomReader::ReadExpr()
{
    if (!IsCurrentFunctionCompleted())
    {
        WasmNode node = m_nodes.Item(m_iNode++);
        m_currentNode = node;
        return node.op;
    }
    return wbEnd;
}

void WasmCustomReader::AddNode(WasmNode node)
{
    m_nodes.Add(node);
}

};
#endif
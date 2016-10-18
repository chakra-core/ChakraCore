//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
namespace Wasm
{

void
WasmCustomReader::SeekToFunctionBody(FunctionBodyReaderInfo readerInfo)
{

}

bool
WasmCustomReader::IsCurrentFunctionCompleted() const
{
    return true;
}

WasmOp
WasmCustomReader::ReadExpr()
{
    uint32 paramCount = callSignature->GetParamCount();
    if (state < paramCount)
    {
        m_currentNode.var.num = state;
        ++state;
        return wbGetLocal;
    }
    if (state == paramCount)
    {
        m_currentNode.call.funcType = FunctionIndexTypes::Import;
        m_currentNode.call.num = importIndex;
        ++state;
        return wbCall;
    }
    return wbEnd;
}

};
#endif
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM
namespace Wasm
{
    class WasmReaderBase
    {
    public:
        virtual void SeekToFunctionBody(class WasmFunctionInfo* funcInfo) = 0;
        virtual bool IsCurrentFunctionCompleted() const = 0;
        virtual WasmOp ReadExpr() = 0;
        virtual void FunctionEnd() = 0;
        virtual uint32 EstimateCurrentFunctionBytecodeSize() const = 0;
        WasmNode m_currentNode;
    };
} // namespace Wasm
#endif // ENABLE_WASM

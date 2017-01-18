//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM
namespace Wasm
{
    class WasmCustomReader : public WasmReaderBase
    {
    public:
        WasmCustomReader(ArenaAllocator* alloc);
        virtual void SeekToFunctionBody(class WasmFunctionInfo* funcInfo) override;
        virtual bool IsCurrentFunctionCompleted() const override;
        virtual WasmOp ReadExpr() override;
        virtual void FunctionEnd() override;

        void AddNode(WasmNode node);
    private:
        JsUtil::List<WasmNode, ArenaAllocator> m_nodes;
        uint32 m_state = 0;
    };
} // namespace Wasm
#endif // ENABLE_WASM

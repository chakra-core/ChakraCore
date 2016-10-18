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
        virtual void SeekToFunctionBody(FunctionBodyReaderInfo readerInfo) override;
        virtual bool IsCurrentFunctionCompleted() const override;
        virtual WasmOp ReadExpr() override;

        void AddNode(WasmNode node);
    private:
        uint32 m_iNode;
        JsUtil::List<WasmNode, ArenaAllocator> m_nodes;
    };
} // namespace Wasm
#endif // ENABLE_WASM

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
        virtual void SeekToFunctionBody(FunctionBodyReaderInfo readerInfo) override;
        virtual bool IsCurrentFunctionCompleted() const override;
        virtual WasmOp ReadExpr() override;

        WasmSignature* callSignature;
        uint32 importIndex;
    private:
        uint32 state = 0;
    };
} // namespace Wasm
#endif // ENABLE_WASM

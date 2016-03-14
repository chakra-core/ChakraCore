//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    typedef bool(*FunctionBodyCallback)(uint32 index, void* data);

    class BaseWasmReader
    {
    public:
        virtual void InitializeReader() = 0;
        virtual bool ReadNextSection(SectionCode nextSection) = 0;
        // Fully read the section in the reader. Return true if the section fully read
        virtual bool ProcessCurrentSection() = 0;

        virtual bool ReadFunctionBodies(FunctionBodyCallback callback, void* callbackdata) = 0;
        virtual WasmOp ReadExpr() = 0;
        virtual WasmOp ReadFromBlock() = 0;
        virtual WasmOp ReadFromCall() = 0;
        virtual bool IsBinaryReader() = 0;
        WasmNode    m_currentNode;
        ModuleInfo * m_moduleInfo;
        WasmModule * m_module;

    protected:
        WasmFunctionInfo *  m_funcInfo;
    };

} // namespace Wasm

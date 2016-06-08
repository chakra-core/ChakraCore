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
        virtual WasmOp GetLastOp() = 0;
        virtual bool IsBinaryReader() = 0;
        WasmNode    m_currentNode;
        ModuleInfo * m_moduleInfo;
        WasmModule * m_module;

        // TODO: Move this to somewhere more appropriate and possible make m_alloc part of
        // BaseWasmReader state.
        char16* CvtUtf8Str(ArenaAllocator* m_alloc, LPUTF8 name, uint32 nameLen)
        {
            utf8::DecodeOptions decodeOptions = utf8::doDefault;
            charcount_t utf16Len = utf8::ByteIndexIntoCharacterIndex(name, nameLen, decodeOptions);
            char16* contents = AnewArray(m_alloc, char16, utf16Len + 1);
            if (contents == nullptr)
            {
                Js::Throw::OutOfMemory();
            }
            utf8::DecodeIntoAndNullTerminate((char16*)contents, name, utf16Len, decodeOptions);
            return contents;
        }

    protected:
        WasmFunctionInfo *  m_funcInfo;
    };

} // namespace Wasm

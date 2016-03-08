//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
class ModuleInfo
{
private:
    struct Memory
    {
        Memory() : minSize(0)
        {
        }
        uint64 minSize;
        uint64 maxSize;
        bool exported;
        static const uint64 PAGE_SIZE = 64 * 1024;
    } m_memory;
public:
    ModuleInfo(ArenaAllocator * alloc);

    bool InitializeMemory(uint32 minSize, uint32 maxSize, bool exported);


    const Memory * GetMemory() const;

    uint32 AddSignature(WasmSignature * signature);
    WasmSignature * GetSignature(uint32 index) const;
    uint32 GetSignatureCount() const;

    void AddIndirectFunctionIndex(uint32 funcIndex);
    uint32 GetIndirectFunctionIndex(uint32 indirTableIndex) const;
    uint32 GetIndirectFunctionCount() const;

    void SetFunctionCount(uint count);
    uint GetFunctionCount() const;

    uint32 AddFunSig(WasmFunctionInfo* funsig);
    WasmFunctionInfo * GetFunSig(uint index) const;

private:
    typedef JsUtil::GrowingArray<WasmSignature*, ArenaAllocator> WasmSignatureArray;
    typedef JsUtil::GrowingArray<uint32, ArenaAllocator> WasmIndirectFuncArray;
    typedef JsUtil::GrowingArray<WasmFunctionInfo*, ArenaAllocator> WasmFuncSigArray;

    WasmSignatureArray * m_signatures;
    WasmIndirectFuncArray * m_indirectfuncs;
    WasmFuncSigArray * m_funsigs;

    uint m_funcCount;
    ArenaAllocator * m_alloc;
};

} // namespace Wasm

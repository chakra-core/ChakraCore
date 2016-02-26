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
        size_t minSize;
        size_t maxSize;
        bool exported;
    } m_memory;
public:
    ModuleInfo(ArenaAllocator * alloc);

    bool InitializeMemory(size_t minSize, size_t maxSize, bool exported);


    const Memory * GetMemory() const;

    uint32 AddSignature(WasmSignature * signature);
    WasmSignature * GetSignature(uint32 index) const;
    uint32 GetSignatureCount() const;

    void AddIndirectFunctionIndex(uint32 funcIndex);
    uint32 GetIndirectFunctionIndex(uint32 indirTableIndex) const;
    uint32 GetIndirectFunctionCount() const;

    void SetFunctionCount(uint count);
    uint GetFunctionCount() const;

private:
    typedef JsUtil::GrowingArray<WasmSignature*, ArenaAllocator> WasmSignatureArray;
    typedef JsUtil::GrowingArray<uint32, ArenaAllocator> WasmIndirectFuncArray;

    WasmSignatureArray * m_signatures;
    WasmIndirectFuncArray * m_indirectfuncs;

    uint m_funcCount;
    ArenaAllocator * m_alloc;
};

} // namespace Wasm

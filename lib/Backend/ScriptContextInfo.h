//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

typedef JsUtil::BaseDictionary<intptr_t, IR::JnHelperMethod, HeapAllocator, PowerOf2SizePolicy> JITDOMFastPathHelperMap;

class ScriptContextInfo
{
public:
    ScriptContextInfo(ScriptContextDataIDL * contextData);
    intptr_t GetNullAddr() const;
    intptr_t GetUndefinedAddr() const;
    intptr_t GetTrueAddr() const;
    intptr_t GetFalseAddr() const;
    intptr_t GetTrueOrFalseAddr(bool value) const;
    intptr_t GetUndeclBlockVarAddr() const;
    intptr_t GetEmptyStringAddr() const;
    intptr_t GetNegativeZeroAddr() const;
    intptr_t GetNumberTypeStaticAddr() const;
    intptr_t GetStringTypeStaticAddr() const;
    intptr_t GetObjectTypeAddr() const;
    intptr_t GetObjectHeaderInlinedTypeAddr() const;
    intptr_t GetRegexTypeAddr() const;
    intptr_t GetArrayTypeAddr() const;
    intptr_t GetNativeIntArrayTypeAddr() const;
    intptr_t GetNativeFloatArrayTypeAddr() const;
    intptr_t GetArrayConstructorAddr() const;
    intptr_t GetCharStringCacheAddr() const;
    intptr_t GetSideEffectsAddr() const;
    intptr_t GetArraySetElementFastPathVtableAddr() const;
    intptr_t GetIntArraySetElementFastPathVtableAddr() const;
    intptr_t GetFloatArraySetElementFastPathVtableAddr() const;
    intptr_t GetLibraryAddr() const;
    intptr_t GetNumberAllocatorAddr() const;
    intptr_t GetRecyclerAddr() const;
    intptr_t GetBuiltinFunctionsBaseAddr() const;

    intptr_t GetAddr() const;

    intptr_t GetVTableAddress(VTableValue vtableType) const;

    JITDOMFastPathHelperMap *  EnsureDOMFastPathHelperMap();

    bool IsRecyclerVerifyEnabled() const;
    uint GetRecyclerVerifyPad() const;
    bool IsPRNGSeeded() const;

    void BeginJIT();
    void EndJIT();
    bool IsJITActive();
private:
    ScriptContextDataIDL m_contextData;
    // TODO: OOP JIT, set this when we initialize PRNG
    bool m_isPRNGSeeded;

    uint m_activeJITCount;

    JITDOMFastPathHelperMap * m_domFastPathHelperMap;
};

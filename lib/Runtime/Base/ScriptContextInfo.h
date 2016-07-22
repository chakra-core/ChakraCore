//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ScriptContextInfo
{
public:
    ScriptContextInfo();
    ~ScriptContextInfo();

    intptr_t GetTrueOrFalseAddr(bool value) const;

    virtual intptr_t GetNullAddr() const = 0;
    virtual intptr_t GetUndefinedAddr() const = 0;
    virtual intptr_t GetTrueAddr() const = 0;
    virtual intptr_t GetFalseAddr() const = 0;
    virtual intptr_t GetUndeclBlockVarAddr() const = 0;
    virtual intptr_t GetEmptyStringAddr() const = 0;
    virtual intptr_t GetNegativeZeroAddr() const = 0;
    virtual intptr_t GetNumberTypeStaticAddr() const = 0;
    virtual intptr_t GetStringTypeStaticAddr() const = 0;
    virtual intptr_t GetObjectTypeAddr() const = 0;
    virtual intptr_t GetObjectHeaderInlinedTypeAddr() const = 0;
    virtual intptr_t GetRegexTypeAddr() const = 0;
    virtual intptr_t GetArrayTypeAddr() const = 0;
    virtual intptr_t GetNativeIntArrayTypeAddr() const = 0;
    virtual intptr_t GetNativeFloatArrayTypeAddr() const = 0;
    virtual intptr_t GetArrayConstructorAddr() const = 0;
    virtual intptr_t GetCharStringCacheAddr() const = 0;
    virtual intptr_t GetSideEffectsAddr() const = 0;
    virtual intptr_t GetArraySetElementFastPathVtableAddr() const = 0;
    virtual intptr_t GetIntArraySetElementFastPathVtableAddr() const = 0;
    virtual intptr_t GetFloatArraySetElementFastPathVtableAddr() const = 0;
    virtual intptr_t GetLibraryAddr() const = 0;
    virtual intptr_t GetNumberAllocatorAddr() const = 0;
    virtual intptr_t GetRecyclerAddr() const = 0;
    virtual bool GetRecyclerAllowNativeCodeBumpAllocation() const = 0;
    virtual bool IsSIMDEnabled() const = 0;
    virtual intptr_t GetBuiltinFunctionsBaseAddr() const = 0;

    virtual intptr_t GetAddr() const = 0;

    virtual intptr_t GetVTableAddress(VTableValue vtableType) const = 0;

    virtual bool IsRecyclerVerifyEnabled() const = 0;
    virtual uint GetRecyclerVerifyPad() const = 0;

    void AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper);
    IR::JnHelperMethod GetDOMFastPathHelper(intptr_t funcInfoAddr);

    bool IsPRNGSeeded() const;

    void BeginJIT();
    void EndJIT();
    bool IsJITActive();
private:
    // TODO: OOP JIT, set this when we initialize PRNG
    bool m_isPRNGSeeded;

    uint m_activeJITCount;

    typedef JsUtil::BaseDictionary<intptr_t, IR::JnHelperMethod, HeapAllocator, PowerOf2SizePolicy,
        DefaultComparer, JsUtil::SimpleDictionaryEntry, JsUtil::AsymetricResizeLock> JITDOMFastPathHelperMap;

    JITDOMFastPathHelperMap * m_domFastPathHelperMap;
};

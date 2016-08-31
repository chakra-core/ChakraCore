//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ServerScriptContext : public ScriptContextInfo
{
public:
    ServerScriptContext(ScriptContextDataIDL * contextData);
    ~ServerScriptContext();
    virtual intptr_t GetNullAddr() const override;
    virtual intptr_t GetUndefinedAddr() const override;
    virtual intptr_t GetTrueAddr() const override;
    virtual intptr_t GetFalseAddr() const override;
    virtual intptr_t GetUndeclBlockVarAddr() const override;
    virtual intptr_t GetEmptyStringAddr() const override;
    virtual intptr_t GetNegativeZeroAddr() const override;
    virtual intptr_t GetNumberTypeStaticAddr() const override;
    virtual intptr_t GetStringTypeStaticAddr() const override;
    virtual intptr_t GetObjectTypeAddr() const override;
    virtual intptr_t GetObjectHeaderInlinedTypeAddr() const override;
    virtual intptr_t GetRegexTypeAddr() const override;
    virtual intptr_t GetArrayTypeAddr() const override;
    virtual intptr_t GetNativeIntArrayTypeAddr() const override;
    virtual intptr_t GetNativeFloatArrayTypeAddr() const override;
    virtual intptr_t GetArrayConstructorAddr() const override;
    virtual intptr_t GetCharStringCacheAddr() const override;
    virtual intptr_t GetSideEffectsAddr() const override;
    virtual intptr_t GetArraySetElementFastPathVtableAddr() const override;
    virtual intptr_t GetIntArraySetElementFastPathVtableAddr() const override;
    virtual intptr_t GetFloatArraySetElementFastPathVtableAddr() const override;
    virtual intptr_t GetLibraryAddr() const override;
    virtual intptr_t GetGlobalObjectAddr() const override;
    virtual intptr_t GetGlobalObjectThisAddr() const override;
    virtual intptr_t GetNumberAllocatorAddr() const override;
    virtual intptr_t GetRecyclerAddr() const override;
    virtual bool GetRecyclerAllowNativeCodeBumpAllocation() const override;
    virtual bool IsSIMDEnabled() const override;
    virtual bool IsPRNGSeeded() const override;
    virtual intptr_t GetBuiltinFunctionsBaseAddr() const override;

    virtual intptr_t GetAddr() const override;

    virtual intptr_t GetVTableAddress(VTableValue vtableType) const override;

    virtual bool IsRecyclerVerifyEnabled() const override;
    virtual uint GetRecyclerVerifyPad() const override;

    virtual void AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper) override;
    virtual IR::JnHelperMethod GetDOMFastPathHelper(intptr_t funcInfoAddr) override;


    typedef JsUtil::BaseDictionary<uint, Js::ServerSourceTextModuleRecord*, Memory::HeapAllocator> ServerModuleRecords;
    ServerModuleRecords m_moduleRecords;

    virtual Js::Var* GetModuleExportSlotArrayAddress(uint moduleIndex, uint slotIndex) override;

    void AddModuleRecordInfo(unsigned int moduleId, __int64 localExportSlotsAddr);

    Js::ScriptContextProfiler *  GetCodeGenProfiler() const;

    void BeginJIT();
    void EndJIT();
    bool IsJITActive();
private:
    JITDOMFastPathHelperMap * m_domFastPathHelperMap;
#ifdef PROFILE_EXEC
    Js::ScriptContextProfiler * m_codeGenProfiler;
#endif

    ScriptContextDataIDL m_contextData;
    uint m_activeJITCount;

    // TODO: OOP JIT, set this when we initialize PRNG
    bool m_isPRNGSeeded;

};

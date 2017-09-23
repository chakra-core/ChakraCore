//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ServerScriptContext : public ScriptContextInfo
{
#if ENABLE_OOP_NATIVE_CODEGEN
private:
    struct ThreadContextHolder
    {
        ServerThreadContext* threadContextInfo;
        ThreadContextHolder(ServerThreadContext* threadContextInfo);
        ~ThreadContextHolder();
    };
    ThreadContextHolder threadContextHolder;

public:
    ServerScriptContext(ScriptContextDataIDL * contextData, ServerThreadContext* threadContextInfo);
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
#ifdef ENABLE_SIMDJS
    virtual bool IsSIMDEnabled() const override;
#endif
    virtual bool IsPRNGSeeded() const override;
    virtual bool IsClosed() const override;
    virtual intptr_t GetBuiltinFunctionsBaseAddr() const override;

#ifdef ENABLE_SCRIPT_DEBUGGING
    virtual intptr_t GetDebuggingFlagsAddr() const override;
    virtual intptr_t GetDebugStepTypeAddr() const override;
    virtual intptr_t GetDebugFrameAddressAddr() const override;
    virtual intptr_t GetDebugScriptIdWhenSetAddr() const override;
#endif

    virtual intptr_t GetAddr() const override;

    virtual intptr_t GetVTableAddress(VTableValue vtableType) const override;

    virtual bool IsRecyclerVerifyEnabled() const override;
    virtual uint GetRecyclerVerifyPad() const override;

    virtual void AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper) override;
    virtual IR::JnHelperMethod GetDOMFastPathHelper(intptr_t funcInfoAddr) override;


    typedef JsUtil::BaseDictionary<uint, Js::ServerSourceTextModuleRecord*, Memory::HeapAllocator> ServerModuleRecords;
    ServerModuleRecords m_moduleRecords;

    virtual Field(Js::Var)* GetModuleExportSlotArrayAddress(uint moduleIndex, uint slotIndex) override;

    void SetIsPRNGSeeded(bool value);
    void AddModuleRecordInfo(unsigned int moduleId, __int64 localExportSlotsAddr);
    void UpdateGlobalObjectThisAddr(intptr_t globalThis);
    OOPEmitBufferManager * GetEmitBufferManager(bool asmJsManager);
    void DecommitEmitBufferManager(bool asmJsManager);
    Js::ScriptContextProfiler *  GetCodeGenProfiler() const;
    ServerThreadContext* GetThreadContext() { return threadContextHolder.threadContextInfo; }

    ArenaAllocator * GetSourceCodeArena();
    void Close();
    void AddRef();
    void Release();

private:
    JITDOMFastPathHelperMap * m_domFastPathHelperMap;
#ifdef PROFILE_EXEC
    Js::ScriptContextProfiler * m_codeGenProfiler;
#endif
    ArenaAllocator m_sourceCodeArena;

    OOPEmitBufferManager m_interpreterThunkBufferManager;
    OOPEmitBufferManager m_asmJsInterpreterThunkBufferManager;

    ScriptContextDataIDL m_contextData;
    intptr_t m_globalThisAddr;

    uint m_refCount;

    bool m_isPRNGSeeded;
    bool m_isClosed;
#endif
};

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "JsrtRuntime.h"
class ChakraCoreHostScriptContext;

namespace Js
{
    namespace SCACore
    {
        class Serializer;
        class Deserializer;
    }
}

class JsrtContextCore sealed : public JsrtContext
{
public:
    static JsrtContextCore *New(JsrtRuntime * runtime);
    virtual void Finalize(bool isShutdown) override;
    virtual void Dispose(bool isShutdown) override;
    ChakraCoreHostScriptContext* GetHostScriptContext() const { return hostContext; }

    void OnScriptLoad(Js::JavascriptFunction * scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException);
private:
    DEFINE_VTABLE_CTOR(JsrtContextCore, JsrtContext);
    JsrtContextCore(JsrtRuntime * runtime);
    Js::ScriptContext* EnsureScriptContext();

    FieldNoBarrier(ChakraCoreHostScriptContext*) hostContext;
};

class ChakraCoreStreamWriter : public HostStream
{
    Js::SCACore::Serializer *m_serializerCore;
    byte* m_data;
    size_t m_length;

    ReallocateBufferMemoryFunc reallocateBufferMemory;
    WriteHostObjectFunc writeHostObject;
    void * callbackState;
public:
    ChakraCoreStreamWriter(ReallocateBufferMemoryFunc reallocateBufferMemory, WriteHostObjectFunc writeHostObject, void * callbackState) :
        reallocateBufferMemory(reallocateBufferMemory),
        writeHostObject(writeHostObject),
        callbackState(callbackState),
        m_data(nullptr),
        m_length(0),
        m_serializerCore(nullptr)
    {
    }

    ~ChakraCoreStreamWriter();

    void SetSerializer(Js::SCACore::Serializer *s);

    void WriteRawBytes(const void* source, size_t length);
    bool WriteValue(JsValueRef root);
    bool ReleaseData(byte** data, size_t *dataLength);
    bool DetachArrayBuffer();
    JsErrorCode SetTransferableVars(JsValueRef *transferableVars, size_t transferableVarsCount);
    void FreeSelf();

    virtual bool WriteHostObject(void* data) override;
    virtual byte * ExtendBuffer(byte *oldBuffer, size_t newSize, size_t *allocatedSize) override;
};

class ChakraHostDeserializerHandle : public HostReadStream
{
    Js::SCACore::Deserializer *m_deserializer;
    ReadHostObjectFunc readHostObject;
    GetSharedArrayBufferFromIdFunc getSharedArrayBufferFromId;
    void* callbackState;

public:
    ChakraHostDeserializerHandle(ReadHostObjectFunc readHostObject, GetSharedArrayBufferFromIdFunc getSharedArrayBufferFromId, void* callbackState) :
        readHostObject(readHostObject),
        getSharedArrayBufferFromId(getSharedArrayBufferFromId),
        callbackState(callbackState),
        m_deserializer(nullptr)
    { }

    ~ChakraHostDeserializerHandle();

    void SetDeserializer(Js::SCACore::Deserializer *deserializer);
    bool ReadRawBytes(size_t length, void **data);
    virtual bool ReadBytes(size_t length, void **data);
    virtual JsErrorCode SetTransferableVars(JsValueRef *transferableVars, size_t transferableVarsCount);
    JsValueRef ReadValue();
    void FreeSelf();

    virtual Js::Var ReadHostObject() override;

};

class ChakraCoreHostScriptContext sealed : public HostScriptContext
{
public:
    ChakraCoreHostScriptContext(Js::ScriptContext* scriptContext)
        : HostScriptContext(scriptContext),
        fetchImportedModuleCallback(nullptr),
        fetchImportedModuleFromScriptCallback(nullptr),
        notifyModuleReadyCallback(nullptr),
        initializeImportMetaCallback(nullptr),
        reportModuleCompletionCallback(nullptr)
    {
    }
    ~ChakraCoreHostScriptContext()
    {
    }

    virtual void Delete()
    {
        HeapDelete(this);
    }

    HRESULT GetPreviousHostScriptContext(__deref_out HostScriptContext** previousScriptSite)
    {
        *previousScriptSite = GetScriptContext()->GetThreadContext()->GetPreviousHostScriptContext();
        return NOERROR;
    }

    HRESULT SetCaller(IUnknown *punkNew, IUnknown **ppunkPrev)
    {
        return NOERROR;
    }

    BOOL HasCaller()
    {
        return FALSE;
    }

    HRESULT PushHostScriptContext()
    {
        GetScriptContext()->GetThreadContext()->PushHostScriptContext(this);
        return NOERROR;
    }

    void PopHostScriptContext()
    {
        GetScriptContext()->GetThreadContext()->PopHostScriptContext();
    }

    HRESULT GetDispatchExCaller(_Outptr_result_maybenull_ void** dispatchExCaller)
    {
        *dispatchExCaller = nullptr;
        return NOERROR;
    }

    void ReleaseDispatchExCaller(_In_ void* dispatchExCaller)
    {
        return;
    }

    Js::ModuleRoot * GetModuleRoot(int moduleID)
    {
        Assert(false);
        return nullptr;
    }

    HRESULT CheckCrossDomainScriptContext(_In_ Js::ScriptContext* scriptContext) override
    {
        // no cross domain for jsrt. Return S_OK
        return S_OK;
    }

    HRESULT GetHostContextUrl(_In_ DWORD_PTR hostSourceContext, _Out_ BSTR& pUrl) override
    {
        Assert(false);
        return E_NOTIMPL;
    }

    void CleanDynamicCodeCache() override
    {
        // Don't need this for jsrt core.
        return;
    }

    HRESULT VerifyDOMSecurity(Js::ScriptContext* targetContext, Js::Var obj) override
    {
        Assert(false);
        return E_NOTIMPL;
    }

#if DBG
    bool IsHostCrossSiteThunk(Js::JavascriptMethod address) override
    {
        Assert(false);
        return false;
    }
#endif

    bool SetCrossSiteForFunctionType(Js::JavascriptFunction * function) override
    {
        return false;
    }

    HRESULT CheckEvalRestriction() override
    {
        Assert(false);
        return E_NOTIMPL;
    }

    HRESULT HostExceptionFromHRESULT(HRESULT hr, Js::Var* outError) override
    {
        Assert(false);
        return E_NOTIMPL;
    }

    HRESULT GetExternalJitData(ExternalJitData id, void *data) override
    {
        Assert(false);
        return E_NOTIMPL;
    }

    HRESULT SetDispatchInvoke(Js::JavascriptMethod dispatchInvoke) override
    {
        AssertMsg(false, "no hostdispatch in jsrt");
        return E_NOTIMPL;
    }

    HRESULT ThrowIfFailed(HRESULT hr) override
    {
        hr;
        // No support yet
        return S_OK;
    }

    HRESULT EnqueuePromiseTask(Js::Var taskVar) override
    {
        return E_NOTIMPL;
    }

    HRESULT FetchImportedModule(Js::ModuleRecordBase* referencingModule, LPCOLESTR specifier, Js::ModuleRecordBase** dependentModuleRecord) override;
    HRESULT FetchImportedModuleFromScript(JsSourceContext dwReferencingSourceContext, LPCOLESTR specifier, Js::ModuleRecordBase** dependentModuleRecord) override;

    HRESULT NotifyHostAboutModuleReady(Js::ModuleRecordBase* referencingModule, Js::Var exceptionVar) override;

    HRESULT InitializeImportMeta(Js::ModuleRecordBase* referencingModule, Js::Var importMetaObject) override;
    bool ReportModuleCompletion(Js::ModuleRecordBase* module, Js::Var exception) override;

    void SetNotifyModuleReadyCallback(NotifyModuleReadyCallback notifyCallback) { this->notifyModuleReadyCallback = notifyCallback; }
    NotifyModuleReadyCallback GetNotifyModuleReadyCallback() const { return this->notifyModuleReadyCallback; }

    void SetFetchImportedModuleCallback(FetchImportedModuleCallBack fetchCallback) { this->fetchImportedModuleCallback = fetchCallback ; }
    FetchImportedModuleCallBack GetFetchImportedModuleCallback() const { return this->fetchImportedModuleCallback; }

    void SetFetchImportedModuleFromScriptCallback(FetchImportedModuleFromScriptCallBack fetchCallback) { this->fetchImportedModuleFromScriptCallback = fetchCallback; }
    FetchImportedModuleFromScriptCallBack GetFetchImportedModuleFromScriptCallback() const { return this->fetchImportedModuleFromScriptCallback; }

    void SetInitializeImportMetaCallback(InitializeImportMetaCallback initializeCallback) { this->initializeImportMetaCallback = initializeCallback; }
    InitializeImportMetaCallback GetInitializeImportMetaCallback() const { return this->initializeImportMetaCallback; }

    void SetReportModuleCompletionCallback(ReportModuleCompletionCallback processCallback) { this->reportModuleCompletionCallback = processCallback; }
    ReportModuleCompletionCallback GetReportModuleCompletionCallback() const { return this->reportModuleCompletionCallback; }

#if DBG_DUMP || defined(PROFILE_EXEC) || defined(PROFILE_MEM)
    void EnsureParentInfo(Js::ScriptContext* scriptContext = NULL) override
    {
        // nothing to do in jsrt.
        return;
    }
#endif

private:
    FetchImportedModuleCallBack fetchImportedModuleCallback;
    FetchImportedModuleFromScriptCallBack fetchImportedModuleFromScriptCallback;
    NotifyModuleReadyCallback notifyModuleReadyCallback;
    InitializeImportMetaCallback initializeImportMetaCallback;
    ReportModuleCompletionCallback reportModuleCompletionCallback;
};

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Runtime.h"
#include "JsrtContext.h"
#include "JsrtContextCore.h"

JsrtContext *JsrtContext::New(JsrtRuntime * runtime)
{
    return JsrtContextCore::New(runtime);
}

/* static */
bool JsrtContext::Is(void * ref)
{
    return VirtualTableInfo<JsrtContextCore>::HasVirtualTable(ref);
}

void JsrtContext::OnScriptLoad(Js::JavascriptFunction * scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException)
{
    ((JsrtContextCore *)this)->OnScriptLoad(scriptFunction, utf8SourceInfo, compileException);
}

#if ENABLE_TTD
void JsrtContext::OnScriptLoad_TTDCallback(Js::FunctionBody* body, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException, bool notify)
{
    JsrtContextCore* rcvr = ((JsrtContextCore*)this);

    JsrtDebugManager* jsrtDebugManager = rcvr->GetRuntime()->GetJsrtDebugManager();
    if(jsrtDebugManager != nullptr)
    {
        jsrtDebugManager->ReportScriptCompile_TTD(body, utf8SourceInfo, compileException, notify);
    }
}

void JsrtContext::OnReplayDisposeContext_TTDCallback(FinalizableObject* jsrtCtx)
{
    ((JsrtContextCore *)jsrtCtx)->Dispose(false);
}
#endif

JsrtContextCore::JsrtContextCore(JsrtRuntime * runtime) :
    JsrtContext(runtime)
{
    EnsureScriptContext();
    Link();
}

/* static */
JsrtContextCore *JsrtContextCore::New(JsrtRuntime * runtime)
{
    return RecyclerNewFinalized(runtime->GetThreadContext()->EnsureRecycler(), JsrtContextCore, runtime);
}

void JsrtContextCore::Dispose(bool isShutdown)
{
    if (this->GetJavascriptLibrary())
    {
        Unlink();
        this->SetJavascriptLibrary(nullptr);
    }
}

void JsrtContextCore::Finalize(bool isShutdown)
{
}

Js::ScriptContext* JsrtContextCore::EnsureScriptContext()
{
    Assert(this->GetJavascriptLibrary() == nullptr);

    ThreadContext* localThreadContext = this->GetRuntime()->GetThreadContext();

    AutoPtr<Js::ScriptContext> newScriptContext(Js::ScriptContext::New(localThreadContext));

    newScriptContext->Initialize();

    hostContext = HeapNew(ChakraCoreHostScriptContext, newScriptContext);
    newScriptContext->SetHostScriptContext(hostContext);

    this->SetJavascriptLibrary(newScriptContext.Detach()->GetLibrary());

    Js::JavascriptLibrary *library = this->GetScriptContext()->GetLibrary();
    Assert(library != nullptr);
    localThreadContext->GetRecycler()->RootRelease(library->GetGlobalObject());

    library->GetEvalFunctionObject()->SetEntryPoint(&Js::GlobalObject::EntryEval);
    library->GetFunctionConstructor()->SetEntryPoint(&Js::JavascriptFunction::NewInstance);

    return this->GetScriptContext();
}

void JsrtContextCore::OnScriptLoad(Js::JavascriptFunction * scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException)
{
#ifdef ENABLE_SCRIPT_DEBUGGING
    JsrtDebugManager* jsrtDebugManager = this->GetRuntime()->GetJsrtDebugManager();
    if (jsrtDebugManager != nullptr)
    {
        jsrtDebugManager->ReportScriptCompile(scriptFunction, utf8SourceInfo, compileException);
    }
#endif
}

HRESULT ChakraCoreHostScriptContext::FetchImportedModule(Js::ModuleRecordBase* referencingModule, LPCOLESTR specifier, Js::ModuleRecordBase** dependentModuleRecord)
{
    return FetchImportedModuleHelper(
        [=](Js::JavascriptString *specifierVar, JsModuleRecord *dependentRecord) -> JsErrorCode
        {
            return fetchImportedModuleCallback(referencingModule, specifierVar, dependentRecord);
        }, specifier, dependentModuleRecord);
}

HRESULT ChakraCoreHostScriptContext::FetchImportedModuleFromScript(JsSourceContext dwReferencingSourceContext, LPCOLESTR specifier, Js::ModuleRecordBase** dependentModuleRecord)
{
    return FetchImportedModuleHelper(
        [=](Js::JavascriptString *specifierVar, JsModuleRecord *dependentRecord) -> JsErrorCode
    {
        return fetchImportedModuleFromScriptCallback(dwReferencingSourceContext, specifierVar, dependentRecord);
    }, specifier, dependentModuleRecord);
}

template<typename Fn>
HRESULT ChakraCoreHostScriptContext::FetchImportedModuleHelper(Fn fetch, LPCOLESTR specifier, Js::ModuleRecordBase** dependentModuleRecord)
{
    if (fetchImportedModuleCallback == nullptr)
    {
        return E_INVALIDARG;
    }
    Js::JavascriptString* specifierVar = Js::JavascriptString::NewCopySz(specifier, GetScriptContext());
    JsModuleRecord dependentRecord = JS_INVALID_REFERENCE;
    {
        AUTO_NO_EXCEPTION_REGION;
        JsErrorCode errorCode = fetch(specifierVar, &dependentRecord);
        if (errorCode == JsNoError)
        {
            *dependentModuleRecord = static_cast<Js::ModuleRecordBase*>(dependentRecord);
            return NOERROR;
        }
    }
    return E_INVALIDARG;
}

HRESULT ChakraCoreHostScriptContext::NotifyHostAboutModuleReady(Js::ModuleRecordBase* referencingModule, Js::Var exceptionVar)
{
    if (notifyModuleReadyCallback == nullptr)
    {
        return E_INVALIDARG;
    }
    {
        AUTO_NO_EXCEPTION_REGION;
        JsErrorCode errorCode = notifyModuleReadyCallback(referencingModule, exceptionVar);
        if (errorCode == JsNoError)
        {
            return NOERROR;
        }
    }
    return E_INVALIDARG;
}


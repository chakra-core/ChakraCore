//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "JsrtInternal.h"
#include "jsrtHelper.h"
#include "JsrtContextCore.h"
#include "ChakraCore.h"

CHAKRA_API
JsInitializeModuleRecord(
    _In_opt_ JsModuleRecord referencingModule,
    _In_ JsValueRef normalizedSpecifier,
    _Outptr_result_maybenull_ JsModuleRecord* moduleRecord)
{
    PARAM_NOT_NULL(moduleRecord);

    Js::SourceTextModuleRecord* childModuleRecord = nullptr;

    JsErrorCode errorCode = ContextAPIWrapper_NoRecord<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        childModuleRecord = Js::SourceTextModuleRecord::Create(scriptContext);
        if (referencingModule == nullptr)
        {
            childModuleRecord->SetIsRootModule();
        }
        if (normalizedSpecifier != JS_INVALID_REFERENCE)
        {
            childModuleRecord->SetSpecifier(normalizedSpecifier);
            if (Js::SourceTextModuleRecord::Is(referencingModule) && Js::JavascriptString::Is(normalizedSpecifier))
            {
                childModuleRecord->SetParent(Js::SourceTextModuleRecord::FromHost(referencingModule), Js::JavascriptString::FromVar(normalizedSpecifier)->GetSz());
            }
        }
        return JsNoError;
    });
    if (errorCode == JsNoError)
    {
        *moduleRecord = childModuleRecord;
    }
    else
    {
        *moduleRecord = JS_INVALID_REFERENCE;
    }
    return errorCode;
}

CHAKRA_API
JsParseModuleSource(
    _In_ JsModuleRecord requestModule,
    _In_ JsSourceContext sourceContext,
    _In_ byte* sourceText,
    _In_ unsigned int sourceLength,
    _In_ JsParseModuleSourceFlags sourceFlag,
    _Outptr_result_maybenull_ JsValueRef* exceptionValueRef)
{
    PARAM_NOT_NULL(requestModule);
    PARAM_NOT_NULL(exceptionValueRef);
    if (sourceFlag > JsParseModuleSourceFlags_DataIsUTF8)
    {
        return JsErrorInvalidArgument;
    }

    *exceptionValueRef = JS_INVALID_REFERENCE;
    HRESULT hr;
    if (!Js::SourceTextModuleRecord::Is(requestModule))
    {
        return JsErrorInvalidArgument;
    }
    Js::SourceTextModuleRecord* moduleRecord = Js::SourceTextModuleRecord::FromHost(requestModule);
    if (moduleRecord->WasParsed())
    {
        return JsErrorModuleParsed;
    }
    Js::ScriptContext* scriptContext = moduleRecord->GetScriptContext();
    JsErrorCode errorCode = GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        SourceContextInfo* sourceContextInfo = scriptContext->GetSourceContextInfo(sourceContext, nullptr);
        if (sourceContextInfo == nullptr)
        {
            sourceContextInfo = scriptContext->CreateSourceContextInfo(sourceContext, nullptr, 0, nullptr, nullptr, 0);
        }
        SRCINFO si = {
            /* sourceContextInfo   */ sourceContextInfo,
            /* dlnHost             */ 0,
            /* ulColumnHost        */ 0,
            /* lnMinHost           */ 0,
            /* ichMinHost          */ 0,
            /* ichLimHost          */ static_cast<ULONG>(sourceLength),
            /* ulCharOffset        */ 0,
            /* mod                 */ 0,
            /* grfsi               */ 0
        };
        hr = moduleRecord->ParseSource(sourceText, sourceLength, &si, exceptionValueRef, sourceFlag == JsParseModuleSourceFlags_DataIsUTF8 ? true : false);
        if (FAILED(hr))
        {
            return JsErrorScriptCompile;
        }
        return JsNoError;
    });
    return errorCode;
}

CHAKRA_API
JsModuleEvaluation(
    _In_ JsModuleRecord requestModule,
    _Outptr_result_maybenull_ JsValueRef* result)
{
    if (!Js::SourceTextModuleRecord::Is(requestModule))
    {
        return JsErrorInvalidArgument;
    }
    Js::SourceTextModuleRecord* moduleRecord = Js::SourceTextModuleRecord::FromHost(requestModule);
    if (result != nullptr)
    {
        *result = JS_INVALID_REFERENCE;
    }
    Js::ScriptContext* scriptContext = moduleRecord->GetScriptContext();
    JsrtContext* jsrtContext = (JsrtContext*)scriptContext->GetLibrary()->GetJsrtContext();
    JsErrorCode errorCode = SetContextAPIWrapper(jsrtContext, [&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        SmartFPUControl smartFpuControl;
        if (smartFpuControl.HasErr())
        {
            return JsErrorBadFPUState;
        }
        JsValueRef returnRef = moduleRecord->ModuleEvaluation();
        if (result != nullptr)
        {
            *result = returnRef;
        }
        return JsNoError;
    });
    return errorCode;
}

CHAKRA_API
JsSetModuleHostInfo(
    _In_ JsModuleRecord requestModule,
    _In_ JsModuleHostInfoKind moduleHostInfo,
    _In_ void* hostInfo)
{
    if (!Js::SourceTextModuleRecord::Is(requestModule))
    {
        return JsErrorInvalidArgument;
    }
    Js::SourceTextModuleRecord* moduleRecord = Js::SourceTextModuleRecord::FromHost(requestModule);
    Js::ScriptContext* scriptContext = moduleRecord->GetScriptContext();
    JsrtContext* jsrtContext = (JsrtContext*)scriptContext->GetLibrary()->GetJsrtContext();
    JsErrorCode errorCode = SetContextAPIWrapper(jsrtContext, [&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        JsrtContextCore* currentContext = static_cast<JsrtContextCore*>(JsrtContextCore::GetCurrent());
        switch (moduleHostInfo)
        {
        case JsModuleHostInfo_Exception:
            moduleRecord->OnHostException(hostInfo);
            break;
        case JsModuleHostInfo_HostDefined:
            moduleRecord->SetHostDefined(hostInfo);
            break;
        case JsModuleHostInfo_FetchImportedModuleCallback:
            currentContext->GetHostScriptContext()->SetFetchImportedModuleCallback(reinterpret_cast<FetchImportedModuleCallBack>(hostInfo));
            break;
        case JsModuleHostInfo_FetchImportedModuleFromScriptCallback:
            currentContext->GetHostScriptContext()->SetFetchImportedModuleFromScriptCallback(reinterpret_cast<FetchImportedModuleFromScriptCallBack>(hostInfo));
            break;
        case JsModuleHostInfo_NotifyModuleReadyCallback:
            currentContext->GetHostScriptContext()->SetNotifyModuleReadyCallback(reinterpret_cast<NotifyModuleReadyCallback>(hostInfo));
            break;
        default:
            return JsInvalidModuleHostInfoKind;
        };
        return JsNoError;
    });
    return errorCode;
}

CHAKRA_API
JsGetModuleHostInfo(
    _In_  JsModuleRecord requestModule,
    _In_ JsModuleHostInfoKind moduleHostInfo,
    _Outptr_result_maybenull_ void** hostInfo)
{
    if (!Js::SourceTextModuleRecord::Is(requestModule) || (hostInfo == nullptr))
    {
        return JsErrorInvalidArgument;
    }
    *hostInfo = nullptr;
    Js::SourceTextModuleRecord* moduleRecord = Js::SourceTextModuleRecord::FromHost(requestModule);
    Js::ScriptContext* scriptContext = moduleRecord->GetScriptContext();
    JsrtContext* jsrtContext = (JsrtContext*)scriptContext->GetLibrary()->GetJsrtContext();
    JsErrorCode errorCode = SetContextAPIWrapper(jsrtContext, [&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        JsrtContextCore* currentContext = static_cast<JsrtContextCore*>(JsrtContextCore::GetCurrent());
        switch (moduleHostInfo)
        {
        case JsModuleHostInfo_Exception:
            if (moduleRecord->GetErrorObject() != nullptr)
            {
                *hostInfo = moduleRecord->GetErrorObject();
            }
            break;
        case JsModuleHostInfo_HostDefined:
            *hostInfo = moduleRecord->GetHostDefined();
            break;
        case JsModuleHostInfo_FetchImportedModuleCallback:
            *hostInfo = reinterpret_cast<void*>(currentContext->GetHostScriptContext()->GetFetchImportedModuleCallback());
            break;
        case JsModuleHostInfo_FetchImportedModuleFromScriptCallback:
            *hostInfo = reinterpret_cast<void*>(currentContext->GetHostScriptContext()->GetFetchImportedModuleFromScriptCallback());
            break;
        case JsModuleHostInfo_NotifyModuleReadyCallback:
            *hostInfo = reinterpret_cast<void*>(currentContext->GetHostScriptContext()->GetNotifyModuleReadyCallback());
            break;
        default:
            return JsInvalidModuleHostInfoKind;
        };
        return JsNoError;
    });
    return errorCode;
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "JsrtInternal.h"
#include "JsrtExternalObject.h"
#include "JsrtExternalArrayBuffer.h"
#include "jsrtHelper.h"
#include "SCACorePch.h"
#include "JsrtContextCore.h"
#include "ChakraCore.h"

#include "Common/ByteSwap.h"
#include "Library/DataView.h"
#include "Library/JavascriptExceptionMetadata.h"
#include "Base/ThreadContextTlsEntry.h"
#include "Library/JavascriptPromise.h"
#include "Codex/Utf8Helper.h"

CHAKRA_API
JsInitializeModuleRecord(
    _In_opt_ JsModuleRecord referencingModule,
    _In_opt_ JsValueRef normalizedSpecifier,
    _Outptr_result_maybenull_ JsModuleRecord* moduleRecord)
{
    PARAM_NOT_NULL(moduleRecord);

    Js::SourceTextModuleRecord* newModuleRecord = nullptr;

    JsErrorCode errorCode = ContextAPIWrapper_NoRecord<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        newModuleRecord = Js::SourceTextModuleRecord::Create(scriptContext);
        newModuleRecord->SetSpecifier(normalizedSpecifier);
        return JsNoError;
    });
    if (errorCode == JsNoError)
    {
        *moduleRecord = newModuleRecord;
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
            const char16 *moduleUrlSz = nullptr;
            size_t moduleUrlLen = 0;
            if (moduleRecord->GetSpecifier())
            {
                Js::JavascriptString *moduleUrl = Js::VarTo<Js::JavascriptString>(moduleRecord->GetSpecifier());
                moduleUrlSz = moduleUrl->GetSz();
                moduleUrlLen = moduleUrl->GetLength();
            }
            sourceContextInfo = scriptContext->CreateSourceContextInfo(sourceContext, moduleUrlSz, moduleUrlLen, nullptr, nullptr, 0);
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
    _In_opt_ JsModuleRecord requestModule,
    _In_ JsModuleHostInfoKind moduleHostInfo,
    _In_ void* hostInfo)
{
    Js::ScriptContext* scriptContext;
    Js::SourceTextModuleRecord* moduleRecord;
    if (!Js::SourceTextModuleRecord::Is(requestModule))
    {
        if (moduleHostInfo == JsModuleHostInfo_Exception ||
            moduleHostInfo == JsModuleHostInfo_HostDefined ||
            moduleHostInfo == JsModuleHostInfo_Url)
        {
            return JsErrorInvalidArgument;
        }
        scriptContext = JsrtContext::GetCurrent()->GetScriptContext();
    }
    else
    {
        moduleRecord = Js::SourceTextModuleRecord::FromHost(requestModule);
        scriptContext = moduleRecord->GetScriptContext();
    }
    JsrtContext* jsrtContext = (JsrtContext*)scriptContext->GetLibrary()->GetJsrtContext();
    JsErrorCode errorCode = SetContextAPIWrapper(jsrtContext, [&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        JsrtContextCore* currentContext = static_cast<JsrtContextCore*>(JsrtContextCore::GetCurrent());
        switch (moduleHostInfo)
        {
        case JsModuleHostInfo_Exception:
            {
            HRESULT hr = moduleRecord->OnHostException(hostInfo);
            return (hr == NOERROR) ? JsNoError : JsErrorInvalidArgument;
            }
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
        case JsModuleHostInfo_InitializeImportMetaCallback:
            currentContext->GetHostScriptContext()->SetInitializeImportMetaCallback(reinterpret_cast<InitializeImportMetaCallback>(hostInfo));
            break;
        case JsModuleHostInfo_ReportModuleCompletionCallback:
            currentContext->GetHostScriptContext()->SetReportModuleCompletionCallback(reinterpret_cast<ReportModuleCompletionCallback>(hostInfo));
            break;
        case JsModuleHostInfo_Url:
            moduleRecord->SetSpecifier(hostInfo);
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
        case JsModuleHostInfo_InitializeImportMetaCallback:
            *hostInfo = reinterpret_cast<void*>(currentContext->GetHostScriptContext()->GetInitializeImportMetaCallback());
            break;
        case JsModuleHostInfo_ReportModuleCompletionCallback:
            *hostInfo = reinterpret_cast<void*>(currentContext->GetHostScriptContext()->GetReportModuleCompletionCallback());
            break;
        case JsModuleHostInfo_Url:
            *hostInfo = reinterpret_cast<void*>(moduleRecord->GetSpecifier());
            break;
        default:
            return JsInvalidModuleHostInfoKind;
        };
        return JsNoError;
    });
    return errorCode;
}

CHAKRA_API JsGetModuleNamespace(_In_ JsModuleRecord requestModule, _Outptr_result_maybenull_ JsValueRef *moduleNamespace)
{
    PARAM_NOT_NULL(moduleNamespace);
    *moduleNamespace = nullptr;
    if (!Js::SourceTextModuleRecord::Is(requestModule))
    {
        return JsErrorInvalidArgument;
    }
    Js::SourceTextModuleRecord* moduleRecord = Js::SourceTextModuleRecord::FromHost(requestModule);
    if (!moduleRecord->WasEvaluated())
    {
        return JsErrorModuleNotEvaluated;
    }
    if (moduleRecord->GetErrorObject() != nullptr)
    {
        return JsErrorInvalidArgument;
    }
    *moduleNamespace = static_cast<JsValueRef>(moduleRecord->GetNamespace());
    return JsNoError;
}

CHAKRA_API
JsVarSerializer(
    _In_ ReallocateBufferMemoryFunc reallocateBufferMemory,
    _In_ WriteHostObjectFunc writeHostObject,
    _In_opt_ void * callbackState,
    _Out_ JsVarSerializerHandle *serializerHandle)
{
    PARAM_NOT_NULL(reallocateBufferMemory);
    PARAM_NOT_NULL(writeHostObject);
    PARAM_NOT_NULL(serializerHandle);
    JsErrorCode errorCode = ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        ChakraCoreStreamWriter *writer = HeapNew(ChakraCoreStreamWriter, reallocateBufferMemory, writeHostObject, callbackState);
        writer->SetSerializer(HeapNew(Js::SCACore::Serializer, scriptContext, writer));
        *serializerHandle = writer;
        return JsNoError;
    });

    return errorCode;

}

CHAKRA_API
JsVarSerializerWriteRawBytes(
    _In_ JsVarSerializerHandle serializerHandle,
    _In_ const void* source,
    _In_ size_t length)
{
    PARAM_NOT_NULL(serializerHandle);
    PARAM_NOT_NULL(source);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraCoreStreamWriter* streamWriter = reinterpret_cast<ChakraCoreStreamWriter*>(serializerHandle);
        streamWriter->WriteRawBytes(source, length);
        return JsNoError;
    });
}

CHAKRA_API
JsVarSerializerWriteValue(
    _In_ JsVarSerializerHandle serializerHandle,
    _In_ JsValueRef rootObject)
{
    PARAM_NOT_NULL(serializerHandle);
    PARAM_NOT_NULL(rootObject);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraCoreStreamWriter* streamWriter = reinterpret_cast<ChakraCoreStreamWriter*>(serializerHandle);
        streamWriter->WriteValue(rootObject);
        return JsNoError;
    });
}

CHAKRA_API
JsVarSerializerReleaseData(
    _In_ JsVarSerializerHandle serializerHandle,
    _Out_ byte** data,
    _Out_ size_t *dataLength)
{
    PARAM_NOT_NULL(serializerHandle);
    PARAM_NOT_NULL(data);
    PARAM_NOT_NULL(dataLength);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraCoreStreamWriter* streamWriter = reinterpret_cast<ChakraCoreStreamWriter*>(serializerHandle);
        if (!streamWriter->ReleaseData(data, dataLength))
        {
            return JsErrorInvalidArgument;
        }
        return JsNoError;
    });
}

CHAKRA_API
JsVarSerializerDetachArrayBuffer(_In_ JsVarSerializerHandle serializerHandle)
{
    PARAM_NOT_NULL(serializerHandle);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraCoreStreamWriter* streamWriter = reinterpret_cast<ChakraCoreStreamWriter*>(serializerHandle);
        if (!streamWriter->DetachArrayBuffer())
        {
            return JsErrorInvalidArgument;
        }
        return JsNoError;
    });
}

CHAKRA_API
JsVarSerializerSetTransferableVars(
    _In_ JsVarSerializerHandle serializerHandle,
    _In_opt_ JsValueRef *transferableVars,
    _In_ size_t transferableVarsCount)
{
    PARAM_NOT_NULL(serializerHandle);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraCoreStreamWriter* streamWriter = reinterpret_cast<ChakraCoreStreamWriter*>(serializerHandle);
        return streamWriter->SetTransferableVars(transferableVars, transferableVarsCount);
    });

}

CHAKRA_API
JsVarSerializerFree(_In_ JsVarSerializerHandle serializerHandle)
{
    PARAM_NOT_NULL(serializerHandle);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraCoreStreamWriter* streamWriter = reinterpret_cast<ChakraCoreStreamWriter*>(serializerHandle);
        streamWriter->FreeSelf();
        return JsNoError;
    });
}

CHAKRA_API
JsVarDeserializer(
    _In_ void *data,
    _In_ size_t size,
    _In_ ReadHostObjectFunc readHostObject,
    _In_ GetSharedArrayBufferFromIdFunc getSharedArrayBufferFromId,
    _In_opt_ void* callbackState,
    _Out_ JsVarDeserializerHandle *deserializerHandle)
{
    PARAM_NOT_NULL(data);
    PARAM_NOT_NULL(readHostObject);
    PARAM_NOT_NULL(getSharedArrayBufferFromId);
    PARAM_NOT_NULL(deserializerHandle);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraHostDeserializerHandle *reader = HeapNew(ChakraHostDeserializerHandle, readHostObject, getSharedArrayBufferFromId, callbackState);
        reader->SetDeserializer(HeapNew(Js::SCACore::Deserializer, data, size, scriptContext, reader));
        *deserializerHandle = reader;
        return JsNoError;
    });
}

CHAKRA_API
JsVarDeserializerReadRawBytes(_In_ JsVarDeserializerHandle deserializerHandle, _In_ size_t length, _Out_ void **data)
{
    PARAM_NOT_NULL(deserializerHandle);
    PARAM_NOT_NULL(data);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraHostDeserializerHandle* deserializer = reinterpret_cast<ChakraHostDeserializerHandle*>(deserializerHandle);
        if (!deserializer->ReadRawBytes(length, data))
        {
            return JsErrorInvalidArgument;
        }
        return JsNoError;
    });
}

CHAKRA_API
JsVarDeserializerReadBytes(_In_ JsVarDeserializerHandle deserializerHandle, _In_ size_t length, _Out_ void **data)
{
    PARAM_NOT_NULL(deserializerHandle);
    PARAM_NOT_NULL(data);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraHostDeserializerHandle* deserializer = reinterpret_cast<ChakraHostDeserializerHandle*>(deserializerHandle);
        if (!deserializer->ReadBytes(length, data))
        {
            return JsErrorInvalidArgument;
        }
        return JsNoError;
    });
}

CHAKRA_API
JsVarDeserializerReadValue(_In_ JsVarDeserializerHandle deserializerHandle, _Out_ JsValueRef* value)
{
    PARAM_NOT_NULL(deserializerHandle);
    PARAM_NOT_NULL(value);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraHostDeserializerHandle* deserializer = reinterpret_cast<ChakraHostDeserializerHandle*>(deserializerHandle);
        *value = deserializer->ReadValue();
        return JsNoError;
    });
}

CHAKRA_API
JsVarDeserializerSetTransferableVars(_In_ JsVarDeserializerHandle deserializerHandle, _In_opt_ JsValueRef *transferableVars, _In_ size_t transferableVarsCount)
{
    PARAM_NOT_NULL(deserializerHandle);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraHostDeserializerHandle* deserializer = reinterpret_cast<ChakraHostDeserializerHandle*>(deserializerHandle);
        return deserializer->SetTransferableVars(transferableVars, transferableVarsCount);
    });
}

CHAKRA_API
JsVarDeserializerFree(_In_ JsVarDeserializerHandle deserializerHandle)
{
    PARAM_NOT_NULL(deserializerHandle);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ChakraHostDeserializerHandle* deserializer = reinterpret_cast<ChakraHostDeserializerHandle*>(deserializerHandle);
        deserializer->FreeSelf();
        return JsNoError;
    });
}

CHAKRA_API
JsGetArrayBufferExtraInfo(
    _In_ JsValueRef arrayBuffer,
    _Out_ char *extraInfo)
{
    VALIDATE_JSREF(arrayBuffer);
    PARAM_NOT_NULL(extraInfo);
    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::VarIs<Js::ArrayBuffer>(arrayBuffer))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        *extraInfo = Js::VarTo<Js::ArrayBuffer>(arrayBuffer)->GetExtraInfoBits();
    }
    END_JSRT_NO_EXCEPTION

}

CHAKRA_API
JsSetArrayBufferExtraInfo(
    _In_ JsValueRef arrayBuffer,
    _In_ char extraInfo)
{
    VALIDATE_JSREF(arrayBuffer);
    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::VarIs<Js::ArrayBuffer>(arrayBuffer))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        Js::VarTo<Js::ArrayBuffer>(arrayBuffer)->SetExtraInfoBits(extraInfo);
    }
    END_JSRT_NO_EXCEPTION
}


CHAKRA_API
JsGetEmbedderData(
    _In_ JsValueRef instance,
    _Out_ JsValueRef* embedderData)
{
   VALIDATE_JSREF(instance);
   PARAM_NOT_NULL(embedderData);
   return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
        Js::RecyclableObject* object = Js::JavascriptOperators::TryFromVar<Js::RecyclableObject>(instance);
        if (!object)
        {
            return JsErrorInvalidArgument;
        }

        // Right now we know that we support these many. Lets find out if
        // there are more.
        Assert(Js::TypedArrayBase::Is(object->GetTypeId()) ||
               object->GetTypeId() == Js::TypeIds_ArrayBuffer ||
               object->GetTypeId() == Js::TypeIds_DataView ||
               object->GetTypeId() == Js::TypeIds_SharedArrayBuffer);

        if (!object->GetInternalProperty(object, Js::InternalPropertyIds::EmbedderData, embedderData, nullptr, scriptContext))
        {
            *embedderData = nullptr;
        }
        return JsNoError;
    });
}

CHAKRA_API
JsSetEmbedderData(_In_ JsValueRef instance, _In_ JsValueRef embedderData)
{
   VALIDATE_JSREF(instance);
   return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
        Js::RecyclableObject* object = Js::JavascriptOperators::TryFromVar<Js::RecyclableObject>(instance);
        if (!object)
        {
            return JsErrorInvalidArgument;
        }

        // Right now we know that we support these many. Lets find out if
        // there are more.
        Assert(Js::TypedArrayBase::Is(object->GetTypeId()) ||
               object->GetTypeId() == Js::TypeIds_ArrayBuffer ||
               object->GetTypeId() == Js::TypeIds_DataView ||
               object->GetTypeId() == Js::TypeIds_SharedArrayBuffer);

        if (!object->SetInternalProperty(Js::InternalPropertyIds::EmbedderData, embedderData, Js::PropertyOperationFlags::PropertyOperation_None, nullptr))
        {
           return JsErrorInvalidArgument;
        }

        return JsNoError;
    });
}

CHAKRA_API
JsExternalizeArrayBuffer(
    _In_ JsValueRef arrayBufferVar)
{
    VALIDATE_JSREF(arrayBufferVar);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        Js::ArrayBuffer* arrayBuffer = Js::JavascriptOperators::TryFromVar<Js::ArrayBuffer>(arrayBufferVar);
        if (!arrayBuffer)
        {
            return JsErrorInvalidArgument;
        }

        arrayBuffer->Externalize();

        return JsNoError;
    });
}

CHAKRA_API
JsHasOwnItem(_In_ JsValueRef object,
    _In_ uint32_t index,
    _Out_ bool* hasOwnItem)
{
  return ContextAPIWrapper<true>(
      [&](Js::ScriptContext* scriptContext,
          TTDRecorder& _actionEntryPopper) -> JsErrorCode {

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(hasOwnItem);

        *hasOwnItem = !!Js::JavascriptOperators::HasOwnItem(
            Js::VarTo<Js::RecyclableObject>(object), index);

        return JsNoError;
      });
}

CHAKRA_API
JsDetachArrayBuffer(
    _In_ JsValueRef arrayBuffer)
{
    VALIDATE_JSREF(arrayBuffer);
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        if (!Js::VarIs<Js::ArrayBuffer>(arrayBuffer))
        {
            return JsErrorInvalidArgument;
        }

        Js::VarTo<Js::ArrayBuffer>(arrayBuffer)->Detach();
        return JsNoError;
    });
}

CHAKRA_API JsCreateSharedArrayBufferWithSharedContent(_In_ JsSharedArrayBufferContentHandle sharedContents, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {

        PARAM_NOT_NULL(result);

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = library->CreateSharedArrayBuffer((Js::SharedContents*)sharedContents);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

CHAKRA_API JsGetSharedArrayBufferContent(_In_ JsValueRef sharedArrayBuffer, _Out_ JsSharedArrayBufferContentHandle *sharedContents)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {

        PARAM_NOT_NULL(sharedContents);

        if (!Js::VarIs<Js::SharedArrayBuffer>(sharedArrayBuffer))
        {
            return JsErrorInvalidArgument;
        }

        Js::SharedContents**& content = (Js::SharedContents**&)sharedContents;
        *content = Js::VarTo<Js::SharedArrayBuffer>(sharedArrayBuffer)->GetSharedContents();

        if (*content == nullptr)
        {
            return JsErrorFatal;
        }

        (*content)->AddRef();

        return JsNoError;
    });
}

CHAKRA_API JsReleaseSharedArrayBufferContentHandle(_In_ JsSharedArrayBufferContentHandle sharedContents)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        ((Js::SharedContents*)sharedContents)->Release();
        return JsNoError;
    });
}

CHAKRA_API JsCreateCustomExternalObject(
    _In_opt_ void *data,
    _In_opt_ size_t inlineSlotSize,
    _In_opt_ JsTraceCallback traceCallback,
    _In_opt_ JsFinalizeCallback finalizeCallback,
    _Inout_opt_ JsGetterSetterInterceptor ** getterSetterInterceptor,
    _In_opt_ JsValueRef prototype,
    _Out_ JsValueRef * object)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateExternalObject, prototype);

        PARAM_NOT_NULL(object);

        Js::RecyclableObject * prototypeObject = nullptr;
        if (prototype != JS_INVALID_REFERENCE)
        {
            VALIDATE_INCOMING_OBJECT_OR_NULL(prototype, scriptContext);
            prototypeObject = Js::VarTo<Js::RecyclableObject>(prototype);
        }
        if (inlineSlotSize > UINT32_MAX)
        {
            return JsErrorInvalidArgument;
        }

        Js::JsGetterSetterInterceptor * interceptor = nullptr;
        *object = Js::CustomExternalWrapperObject::Create(data, (uint)inlineSlotSize, traceCallback, finalizeCallback, &interceptor, prototypeObject, scriptContext);
        Assert(interceptor);
        *getterSetterInterceptor = reinterpret_cast<JsGetterSetterInterceptor*>(interceptor);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, object);

        return JsNoError;
    });
}

CHAKRA_API JsCreateTracedExternalObject(
    _In_opt_ void *data,
    _In_opt_ size_t inlineSlotSize,
    _In_opt_ JsTraceCallback traceCallback,
    _In_opt_ JsFinalizeCallback finalizeCallback,
    _In_opt_ JsValueRef prototype,
    _Out_ JsValueRef *object)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateExternalObject, prototype);

        PARAM_NOT_NULL(object);

        Js::RecyclableObject * prototypeObject = nullptr;
        if (prototype != JS_INVALID_REFERENCE)
        {
            VALIDATE_INCOMING_OBJECT_OR_NULL(prototype, scriptContext);
            prototypeObject = Js::VarTo<Js::RecyclableObject>(prototype);
        }
        if (inlineSlotSize > UINT32_MAX)
        {
            return JsErrorInvalidArgument;
        }
        *object = JsrtExternalObject::Create(data, (uint)inlineSlotSize, traceCallback, finalizeCallback, prototypeObject, scriptContext, nullptr);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, object);

        return JsNoError;
    });
}

CHAKRA_API JsPrivateHasProperty(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _Out_ bool *hasProperty)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(key, scriptContext);
        PARAM_NOT_NULL(hasProperty);
        *hasProperty = false;

        Js::DynamicObject* dynObj = Js::VarTo<Js::DynamicObject>(object);
        if (!dynObj->HasObjectArray())
        {
            return JsNoError;
        }
        *hasProperty = Js::JavascriptOperators::OP_HasItem(dynObj->GetObjectArray(), key, scriptContext) != 0;

        return JsNoError;
    });
}

CHAKRA_API JsPrivateGetProperty(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _Out_ JsValueRef *value)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetIndex, key, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(key, scriptContext);
        PARAM_NOT_NULL(value);
        *value = nullptr;
        Js::DynamicObject* dynObj = Js::VarTo<Js::DynamicObject>(object);

        if (!dynObj->HasObjectArray())
        {
            *value = scriptContext->GetLibrary()->GetUndefined();
            PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, value);
            return JsNoError;
        }
        *value = (JsValueRef)Js::JavascriptOperators::OP_GetElementI(dynObj->GetObjectArray(), key, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, value);

        return JsNoError;
    });
}

CHAKRA_API JsPrivateSetProperty(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _In_ JsValueRef value)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTSetIndex, object, key, value);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(key, scriptContext);
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);

        Js::DynamicObject* dynObj = Js::VarTo<Js::DynamicObject>(object);

        if (!dynObj->HasObjectArray())
        {
            Js::ArrayObject* objArray = scriptContext->GetLibrary()->CreateArray();
            dynObj->SetObjectArray(objArray);
        }
        Js::JavascriptOperators::OP_SetElementI(dynObj->GetObjectArray(), key, value, scriptContext);

        return JsNoError;
    });
}

CHAKRA_API JsPrivateDeleteProperty(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(key, scriptContext);

        Js::DynamicObject* dynObj = Js::VarTo<Js::DynamicObject>(object);
        if (!dynObj->HasObjectArray())
        {
            *result = scriptContext->GetLibrary()->GetFalse();
            return JsNoError;
        }
        *result = Js::JavascriptOperators::OP_DeleteElementI(dynObj->GetObjectArray(), key, scriptContext);

        return JsNoError;
    });
}

CHAKRA_API JsCloneObject(_In_ JsValueRef source, _Out_ JsValueRef* newObject)
{
    VALIDATE_JSREF(source);

    return ContextAPINoScriptWrapper([&](Js::ScriptContext* scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {

        while (Js::VarIs<Js::JavascriptProxy>(source))
        {
            source = Js::UnsafeVarTo<Js::JavascriptProxy>(source)->GetTarget();
        }

        // We can currently only clone certain types of dynamic objects
        // TODO: support other object types
        if (Js::DynamicObject::IsBaseDynamicObject(source) ||
            Js::VarIs<JsrtExternalObject>(source) ||
            Js::VarIs<Js::CustomExternalWrapperObject>(source))
        {
            Js::DynamicObject* objSource = Js::UnsafeVarTo<Js::DynamicObject>(source);
            *newObject = objSource->Copy(true);
            return JsNoError;
        }

        return JsErrorInvalidArgument;
    });
}

template <class SrcChar, class DstChar>
static void CastCopy(const SrcChar* src, DstChar* dst, size_t count)
{
    const SrcChar* end = src + count;
    while (src < end)
    {
        *dst++ = static_cast<DstChar>(*src++);
    }
}

CHAKRA_API JsCreateString(
    _In_ const char *content,
    _In_ size_t length,
    _Out_ JsValueRef *value)
{
    PARAM_NOT_NULL(content);
    PARAM_NOT_NULL(value);
    *value = JS_INVALID_REFERENCE;

    if (length == static_cast<size_t>(-1))
    {
        length = strlen(content);
    }

    if (length > MaxCharCount)
    {
        return JsErrorOutOfMemory;
    }

    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {

        Js::JavascriptString *stringValue = Js::LiteralStringWithPropertyStringPtr::
            NewFromCString(content, (CharCount)length, scriptContext->GetLibrary());

        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateString, stringValue->GetSz(), stringValue->GetLength());

        *value = stringValue;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, value);

        return JsNoError;
    });
}

CHAKRA_API JsCreateStringUtf16(
    _In_ const uint16_t *content,
    _In_ size_t length,
    _Out_ JsValueRef *value)
{
    PARAM_NOT_NULL(content);
    PARAM_NOT_NULL(value);
    *value = JS_INVALID_REFERENCE;

    if (length == static_cast<size_t>(-1))
    {
        length = wcslen((const char16 *)content);
    }

    if (length > static_cast<CharCount>(-1))
    {
        return JsErrorOutOfMemory;
    }

    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {

        Js::JavascriptString *stringValue = Js::LiteralStringWithPropertyStringPtr::
            NewFromWideString((const char16 *)content, (CharCount)length, scriptContext->GetLibrary());

        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateString, stringValue->GetSz(), stringValue->GetLength());

        *value = stringValue;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, value);

        return JsNoError;
    });
}


CHAKRA_API JsCreatePropertyString(
    _In_z_ const char *name,
    _In_ size_t length,
    _Out_ JsValueRef *propertyString)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);
        Js::PropertyRecord* propertyRecord;
        JsErrorCode errorCode = JsCreatePropertyId(name, length, (JsPropertyIdRef *)&propertyRecord);

        if (errorCode != JsNoError)
        {
            return errorCode;
        }

        *propertyString = scriptContext->GetPropertyString(propertyRecord);
        return JsNoError;
    });
}

CHAKRA_API JsCreatePromise(_Out_ JsValueRef *promise, _Out_ JsValueRef *resolve, _Out_ JsValueRef *reject)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        PARAM_NOT_NULL(promise);
        PARAM_NOT_NULL(resolve);
        PARAM_NOT_NULL(reject);

        *promise = nullptr;
        *resolve = nullptr;
        *reject = nullptr;

        Js::JavascriptPromiseResolveOrRejectFunction *jsResolve = nullptr;
        Js::JavascriptPromiseResolveOrRejectFunction *jsReject = nullptr;
        Js::JavascriptPromise *jsPromise = scriptContext->GetLibrary()->CreatePromise();
        Js::JavascriptPromise::InitializePromise(jsPromise, &jsResolve, &jsReject, scriptContext);

        *promise = (JsValueRef)jsPromise;
        *resolve = (JsValueRef)jsResolve;
        *reject = (JsValueRef)jsReject;

        return JsNoError;
    });
}

CHAKRA_API JsGetPromiseState(_In_ JsValueRef promise, _Out_ JsPromiseState *state)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        VALIDATE_INCOMING_REFERENCE(promise, scriptContext);
        PARAM_NOT_NULL(state);

        *state = JsPromiseStatePending;

        if (!Js::VarIs<Js::JavascriptPromise>(promise))
        {
            return JsErrorInvalidArgument;
        }

        Js::JavascriptPromise *jsPromise = Js::VarTo<Js::JavascriptPromise>(promise);
        Js::JavascriptPromise::PromiseStatus status = jsPromise->GetStatus();

        switch (status)
        {
        case Js::JavascriptPromise::PromiseStatus::PromiseStatusCode_HasRejection:
            *state = JsPromiseStateRejected;
            break;

        case Js::JavascriptPromise::PromiseStatus::PromiseStatusCode_HasResolution:
            *state = JsPromiseStateFulfilled;
            break;
        }

        return JsNoError;
    });
}

CHAKRA_API JsGetPromiseResult(_In_ JsValueRef promise, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        VALIDATE_INCOMING_REFERENCE(promise, scriptContext);
        PARAM_NOT_NULL(result);

        *result = JS_INVALID_REFERENCE;

        if (!Js::VarIs<Js::JavascriptPromise>(promise))
        {
            return JsErrorInvalidArgument;
        }

        Js::JavascriptPromise *jsPromise = Js::VarTo<Js::JavascriptPromise>(promise);
        Js::Var jsResult = jsPromise->GetResult();

        if (jsResult == nullptr)
        {
            return JsErrorPromisePending;
        }

        *result = (JsValueRef)jsResult;
        return JsNoError;
    });
}

CHAKRA_API JsCreateWeakReference(
    _In_ JsValueRef value,
    _Out_ JsWeakRef* weakRef)
{
    VALIDATE_JSREF(value);
    PARAM_NOT_NULL(weakRef);
    *weakRef = nullptr;

    if (Js::TaggedNumber::Is(value))
    {
        return JsNoWeakRefRequired;
    }

    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();
        if (threadContext == nullptr)
        {
            return JsErrorNoCurrentContext;
        }

        Recycler* recycler = threadContext->GetRecycler();
        if (recycler->IsInObjectBeforeCollectCallback())
        {
            return JsErrorInObjectBeforeCollectCallback;
        }

        RecyclerHeapObjectInfo dummyObjectInfo;
        if (!recycler->FindHeapObject(value, Memory::FindHeapObjectFlags::FindHeapObjectFlags_NoFlags, dummyObjectInfo))
        {
            // value is not recyler-allocated
            return JsErrorInvalidArgument;
        }

        recycler->FindOrCreateWeakReferenceHandle<char>(
            reinterpret_cast<char*>(value),
            reinterpret_cast<Memory::RecyclerWeakReference<char>**>(weakRef));
        return JsNoError;
    });
}

CHAKRA_API JsGetWeakReferenceValue(
    _In_ JsWeakRef weakRef,
    _Out_ JsValueRef* value)
{
    VALIDATE_JSREF(weakRef);
    PARAM_NOT_NULL(value);
    *value = JS_INVALID_REFERENCE;

    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        Memory::RecyclerWeakReference<char>* recyclerWeakReference =
            reinterpret_cast<Memory::RecyclerWeakReference<char>*>(weakRef);
        *value = reinterpret_cast<JsValueRef>(recyclerWeakReference->Get());
        return JsNoError;
    });
}

CHAKRA_API JsGetAndClearExceptionWithMetadata(_Out_ JsValueRef *metadata)
{
    PARAM_NOT_NULL(metadata);
    *metadata = nullptr;

    JsrtContext *currentContext = JsrtContext::GetCurrent();

    if (currentContext == nullptr)
    {
        return JsErrorNoCurrentContext;
    }

    Js::ScriptContext *scriptContext = currentContext->GetScriptContext();
    Assert(scriptContext != nullptr);

    if (scriptContext->GetRecycler() && scriptContext->GetRecycler()->IsHeapEnumInProgress())
    {
        return JsErrorHeapEnumInProgress;
    }
    else if (scriptContext->GetThreadContext()->IsInThreadServiceCallback())
    {
        return JsErrorInThreadServiceCallback;
    }

    if (scriptContext->GetThreadContext()->IsExecutionDisabled())
    {
        return JsErrorInDisabledState;
    }

    HRESULT hr = S_OK;
    Js::JavascriptExceptionObject *recordedException = nullptr;

    BEGIN_TRANSLATE_OOM_TO_HRESULT
        if (scriptContext->HasRecordedException())
        {
            recordedException = scriptContext->GetAndClearRecordedException();
        }
    END_TRANSLATE_OOM_TO_HRESULT(hr)

        if (hr == E_OUTOFMEMORY)
        {
            recordedException = scriptContext->GetThreadContext()->GetRecordedException();
        }
    if (recordedException == nullptr)
    {
        return JsErrorInvalidArgument;
    }

    Js::Var exception = recordedException->GetThrownObject(nullptr);

    if (exception == nullptr)
    {
        // TODO: How does this early bailout impact TTD?
        return JsErrorInvalidArgument;
    }

    return ContextAPIWrapper<false>([&](Js::ScriptContext* scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        Js::Var exceptionMetadata = Js::JavascriptExceptionMetadata::CreateMetadataVar(scriptContext);
        Js::JavascriptOperators::OP_SetProperty(exceptionMetadata, Js::PropertyIds::exception, exception, scriptContext);

        Js::FunctionBody *functionBody = recordedException->GetFunctionBody();
        if (functionBody == nullptr)
        {
            // This is probably a parse error. We can get the error location metadata from the thrown object.
            Js::JavascriptExceptionMetadata::PopulateMetadataFromCompileException(exceptionMetadata, exception, scriptContext);
        }
        else
        {
            if (!Js::JavascriptExceptionMetadata::PopulateMetadataFromException(exceptionMetadata, recordedException, scriptContext))
            {
                return JsErrorInvalidArgument;
            }
        }

        *metadata = exceptionMetadata;

#if ENABLE_TTD
        if (hr != E_OUTOFMEMORY)
        {
            PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetAndClearExceptionWithMetadata);
            PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, metadata);
        }
#endif


        return JsNoError;
    });
}

CHAKRA_API JsGetDataViewInfo(
    _In_ JsValueRef dataView,
    _Out_opt_ JsValueRef *arrayBuffer,
    _Out_opt_ unsigned int *byteOffset,
    _Out_opt_ unsigned int *byteLength)
{
    VALIDATE_JSREF(dataView);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::VarIs<Js::DataView>(dataView))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

    Js::DataView* dv = Js::VarTo<Js::DataView>(dataView);
    if (arrayBuffer != nullptr) {
        *arrayBuffer = dv->GetArrayBuffer();
    }

    if (byteOffset != nullptr) {
        *byteOffset = dv->GetByteOffset();
    }

    if (byteLength != nullptr) {
        *byteLength = dv->GetLength();
    }
    }

#if ENABLE_TTD
    Js::ScriptContext* scriptContext = Js::VarTo<Js::RecyclableObject>(dataView)->GetScriptContext();
    if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext) && arrayBuffer != nullptr)
    {
        scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetDataViewInfo(dataView, *arrayBuffer);
    }
#endif

    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsSetHostPromiseRejectionTracker(_In_ JsHostPromiseRejectionTrackerCallback promiseRejectionTrackerCallback, _In_opt_ void *callbackState)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        scriptContext->GetLibrary()->SetNativeHostPromiseRejectionTrackerCallback((Js::JavascriptLibrary::HostPromiseRejectionTrackerCallback) promiseRejectionTrackerCallback, callbackState);
        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsGetProxyProperties(_In_ JsValueRef object, _Out_ bool* isProxy, _Out_opt_ JsValueRef* target, _Out_opt_ JsValueRef* handler)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(object, scriptContext);
        PARAM_NOT_NULL(isProxy);

        if (target != nullptr)
        {
            *target = JS_INVALID_REFERENCE;
        }

        if (handler != nullptr)
        {
            *handler = JS_INVALID_REFERENCE;
        }

        *isProxy = Js::VarIs<Js::JavascriptProxy>(object);

        if (!*isProxy)
        {
            return JsNoError;
        }

        Js::JavascriptProxy* proxy = Js::UnsafeVarTo<Js::JavascriptProxy>(object);
        bool revoked = proxy->IsRevoked();

        if (target != nullptr && !revoked)
        {
            *target = static_cast<JsValueRef>(proxy->GetTarget());
        }

        if (handler != nullptr && !revoked)
        {
            *handler = static_cast<JsValueRef>(proxy->GetHandler());
        }

        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsSetRuntimeBeforeSweepCallback(_In_ JsRuntimeHandle runtimeHandle, _In_opt_ void *callbackState, _In_ JsBeforeSweepCallback beforeSweepCallback)
{
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        JsrtRuntime::FromHandle(runtimeHandle)->SetBeforeSweepCallback(beforeSweepCallback, callbackState);
        return JsNoError;
    });
}

CHAKRA_API
JsSetRuntimeDomWrapperTracingCallbacks(
    _In_ JsRuntimeHandle runtimeHandle,
    _In_ JsRef wrapperTracingState,
    _In_ JsDOMWrapperTracingCallback wrapperTracingCallback,
    _In_ JsDOMWrapperTracingDoneCallback wrapperTracingDoneCallback,
    _In_ JsDOMWrapperTracingEnterFinalPauseCallback enterFinalPauseCallback)
{
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
        ThreadContextScope scope(threadContext);

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        Recycler * recycler = threadContext->GetRecycler();
        recycler->SetDOMWrapperTracingCallback(wrapperTracingState, reinterpret_cast<DOMWrapperTracingCallback>(wrapperTracingCallback), reinterpret_cast<DOMWrapperTracingDoneCallback>(wrapperTracingDoneCallback), reinterpret_cast<DOMWrapperTracingEnterFinalPauseCallback>(enterFinalPauseCallback));
        return JsNoError;
    });
}

CHAKRA_API
JsGetArrayForEachFunction(_Out_ JsValueRef * result)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        *result = scriptContext->GetLibrary()->EnsureArrayPrototypeForEachFunction();

        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API
JsGetArrayKeysFunction(_Out_ JsValueRef * result)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        *result = scriptContext->GetLibrary()->EnsureArrayPrototypeKeysFunction();

        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API
JsGetArrayValuesFunction(_Out_ JsValueRef * result)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        *result = scriptContext->GetLibrary()->EnsureArrayPrototypeValuesFunction();

        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API
JsGetArrayEntriesFunction(_Out_ JsValueRef * result)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        *result = scriptContext->GetLibrary()->EnsureArrayPrototypeEntriesFunction();

        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API
JsGetPropertyIdSymbolIterator(_Out_ JsPropertyIdRef * propertyId)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(propertyId);

        Js::PropertyId symbolIteratorPropertyId = scriptContext->GetLibrary()->GetPropertyIdSymbolIterator();
        *propertyId = Js::JavascriptNumber::ToVar(symbolIteratorPropertyId, scriptContext);

        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API
JsGetErrorPrototype(_Out_ JsValueRef * result)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        *result = scriptContext->GetLibrary()->GetErrorPrototype();
        if (*result == JS_INVALID_REFERENCE)
        {
            return JsErrorFatal;
        }

        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API
JsGetIteratorPrototype(_Out_ JsValueRef * result)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        *result = scriptContext->GetLibrary()->GetIteratorPrototype();
        if (*result == JS_INVALID_REFERENCE)
        {
            return JsErrorFatal;
        }

        return JsNoError;
    },
        /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsTraceExternalReference(_In_ JsRuntimeHandle runtimeHandle, _In_ JsValueRef value)
{
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
        ThreadContextScope scope(threadContext);

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        Recycler * recycler = threadContext->GetRecycler();
        recycler->TryExternalMarkNonInterior(value);
        return JsNoError;
    });
}

CHAKRA_API JsAllocRawData(_In_ JsRuntimeHandle runtimeHandle, _In_ size_t sizeInBytes, _In_ bool zeroed, _Out_ JsRef * buffer)
{
    PARAM_NOT_NULL(buffer);

    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
        ThreadContextScope scope(threadContext);

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        Recycler * recycler = threadContext->GetRecycler();
        *buffer = zeroed
            ? RecyclerNewArrayZ(recycler, char, sizeInBytes)
            : RecyclerNewArray(recycler, char, sizeInBytes);
        return JsNoError;
    });
}

CHAKRA_API JsIsCallable(_In_ JsValueRef object, _Out_ bool *isCallable)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(isCallable);

        *isCallable = Js::JavascriptConversion::IsCallable(object) && !Js::JavascriptOperators::IsClassConstructor(object);

        return JsNoError;
    });
}

CHAKRA_API JsIsConstructor(_In_ JsValueRef object, _Out_ bool *isConstructor)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(isConstructor);

        *isConstructor = Js::JavascriptOperators::IsConstructor(object);

        return JsNoError;
    });
}

CHAKRA_API
JsQueueBackgroundParse_Experimental(
    _In_ JsScriptContents* contents,
    _Out_ DWORD* dwBgParseCookie)
{
    HRESULT hr;
    if (Js::Configuration::Global.flags.BgParse && !CONFIG_FLAG(ForceDiagnosticsMode)
        // For now, only UTF8 buffers are supported for BGParse
        && contents->encodingType == JsScriptEncodingType::Utf8
        && contents->containerType == JsScriptContainerType::HeapAllocatedBuffer
        // SourceContext not needed for BGParse
        && contents->sourceContext == 0)
    {
        hr = BGParseManager::GetBGParseManager()->QueueBackgroundParse((LPUTF8)contents->container, contents->contentLengthInBytes, (char16*)contents->fullPath, dwBgParseCookie);
    }
    else
    {
        hr = E_NOTIMPL;
    }

    JsErrorCode res = (hr == S_OK) ? JsNoError : JsErrorFatal;

    return res;
}

CHAKRA_API
JsDiscardBackgroundParse_Experimental(
    _In_ DWORD dwBgParseCookie,
    _In_ void* buffer,
    _Out_ bool* callerOwnsBuffer)
{
    (*callerOwnsBuffer) = BGParseManager::GetBGParseManager()->DiscardParseResults(dwBgParseCookie, buffer);
    return JsNoError;
}

#ifdef _WIN32
CHAKRA_API
JsEnableOOPJIT()
{
#ifdef ENABLE_OOP_NATIVE_CODEGEN
    JITManager::GetJITManager()->EnableOOPJIT();
    return JsNoError;
#else
    return JsErrorNotImplemented;
#endif
}

CHAKRA_API
JsConnectJITProcess(_In_ HANDLE processHandle, _In_opt_ void* serverSecurityDescriptor, _In_ UUID connectionId)
{
#ifdef ENABLE_OOP_NATIVE_CODEGEN
    JITManager::GetJITManager()->EnableOOPJIT();
    ThreadContext::SetJITConnectionInfo(processHandle, serverSecurityDescriptor, connectionId);
    return JsNoError;
#else
    return JsErrorNotImplemented;
#endif
}
#endif

CHAKRA_API
JsGetArrayBufferFreeFunction(
    _In_ JsValueRef arrayBuffer,
    _Out_ ArrayBufferFreeFn* freeFn)
{
  VALIDATE_JSREF(arrayBuffer);
  PARAM_NOT_NULL(freeFn);
  return ContextAPINoScriptWrapper_NoRecord(
      [&](Js::ScriptContext* scriptContext) -> JsErrorCode {
        if (!Js::VarIs<Js::ArrayBuffer>(arrayBuffer))
        {
          return JsErrorInvalidArgument;
        }

        *freeFn = Js::VarTo<Js::ArrayBuffer>(arrayBuffer)->GetArrayBufferFreeFn();
        return JsNoError;
      });
}

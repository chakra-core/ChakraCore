//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JITServerPch.h"

__declspec(dllexport)
HRESULT JsInitializeRpcServer(
    __in UUID* connectionUuid)
{
    RPC_STATUS status;
    RPC_BINDING_VECTOR* bindingVector = NULL;
    UUID_VECTOR uuidVector;

    uuidVector.Count = 1;
    uuidVector.Uuid[0] = connectionUuid;

    status = RpcServerUseProtseqEpW(
        L"ncalrpc",
        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
        NULL,
        NULL);
    if (status != RPC_S_OK)
    {
        return status;
    }

    status = RpcServerRegisterIf2(
        ServerIChakraJIT_v0_0_s_ifspec,
        NULL,
        NULL,
        RPC_IF_AUTOLISTEN,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
        (ULONG)-1,
        NULL);

    if (status != RPC_S_OK)
    {
        return status;
    }
    status = RpcServerInqBindings(&bindingVector);
    if (status != RPC_S_OK)
    {
        return status;
    }

    status = RpcEpRegister(
        ServerIChakraJIT_v0_0_s_ifspec,
        bindingVector,
        &uuidVector,
        NULL);

    RpcBindingVectorFree(&bindingVector);

    if (status != RPC_S_OK)
    {
        return status;
    }
    JITManager::GetJITManager()->EnableOOPJIT();

    status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);

    return status;
}

HRESULT
ServerShutdown(/* [in] */ handle_t binding)
{
    RPC_STATUS status;

    status = RpcMgmtStopServerListening(NULL);
    if (status != RPC_S_OK)
    {
        TerminateProcess(GetCurrentProcess(), status);
    }

    status = RpcServerUnregisterIf(ServerIChakraJIT_v0_0_s_ifspec, NULL, FALSE);
    if (status != RPC_S_OK)
    {
        TerminateProcess(GetCurrentProcess(), status);
    }
    return S_OK;
}

HRESULT
ServerInitializeThreadContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in ThreadContextDataIDL * threadContextData,
    /* [out] */ __RPC__out __int3264 *threadContextRoot)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerThreadContext * contextInfo = HeapNew(ServerThreadContext, threadContextData);

    *threadContextRoot = (intptr_t)contextInfo;
    return S_OK;
}

HRESULT
ServerCleanupProcess(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 processHandle)
{
    CloseHandle((HANDLE)processHandle);
    return S_OK;
}

HRESULT
ServerCleanupThreadContext(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 threadContextRoot)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerThreadContext * threadContextInfo = reinterpret_cast<ServerThreadContext*>(threadContextRoot);
    if (threadContextInfo == nullptr)
    {
        return RPC_S_INVALID_ARG;
    }

    while (threadContextInfo->IsJITActive()) { Sleep(30); }
    HeapDelete(threadContextInfo);

    return S_OK;
}

HRESULT
ServerAddPropertyRecord(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 threadContextRoot,
    /* [in] */ PropertyRecordIDL * propertyRecord)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerThreadContext * threadContextInfo = reinterpret_cast<ServerThreadContext*>(threadContextRoot);

    if (threadContextInfo == nullptr) 
    {
        return RPC_S_INVALID_ARG;
    }

    threadContextInfo->AddToPropertyMap((Js::PropertyRecord *)propertyRecord);

    return S_OK;
}

HRESULT
ServerAddDOMFastPathHelper(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 scriptContextRoot,
    /* [in] */ __int3264 funcInfoAddr,
    /* [in] */ int helper)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerScriptContext * scriptContextInfo = reinterpret_cast<ServerScriptContext*>(scriptContextRoot);

    if (scriptContextInfo == nullptr)
    {
        return RPC_S_INVALID_ARG;
    }

    scriptContextInfo->AddToDOMFastPathHelperMap(funcInfoAddr, (IR::JnHelperMethod)helper);

    return S_OK;
}

HRESULT
ServerInitializeScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in ScriptContextDataIDL * scriptContextData,
    /* [out] */ __RPC__out __int3264 * scriptContextInfoAddress)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerScriptContext * contextInfo = HeapNew(ServerScriptContext, scriptContextData);
    *scriptContextInfoAddress = (intptr_t)contextInfo;
    return S_OK;
}

HRESULT
ServerCleanupScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 scriptContextRoot)
{
    ServerScriptContext * scriptContextInfo = reinterpret_cast<ServerScriptContext*>(scriptContextRoot);

    if (scriptContextInfo == nullptr)
    {
        return RPC_S_INVALID_ARG;
    }

    while (scriptContextInfo->IsJITActive()) { Sleep(30); }
    HeapDelete(scriptContextInfo);
    return S_OK;
}

HRESULT
ServerFreeAllocation(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 threadContextInfo,
    /* [in] */ __int3264 address)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
    ServerThreadContext * context = reinterpret_cast<ServerThreadContext*>(threadContextInfo);

    if (context == nullptr)
    {
        return RPC_S_INVALID_ARG;
    }

    bool succeeded = context->GetCodeGenAllocators()->emitBufferManager.FreeAllocation((void*)address);
    return succeeded ? S_OK : E_FAIL;
}

HRESULT
ServerIsNativeAddr(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 threadContextInfo,
    /* [in] */ __int3264 address,
    /* [out] */ boolean * result)
{
    ServerThreadContext * context = reinterpret_cast<ServerThreadContext*>(threadContextInfo);

    if (context == nullptr)
    {
        return RPC_S_INVALID_ARG;
    }

    PreReservedVirtualAllocWrapper *preReservedVirtualAllocWrapper = context->GetPreReservedVirtualAllocator();
    if (preReservedVirtualAllocWrapper->IsInRange((void*)address))
    {
        *result = true;
    }
    else if (!context->IsAllJITCodeInPreReservedRegion())
    {
        CustomHeap::CodePageAllocators::AutoLock autoLock(context->GetCodePageAllocators());
        *result = context->GetCodePageAllocators()->IsInNonPreReservedPageAllocator((void*)address);
    }
    else
    {
        *result = false;
    }

    return S_OK;
}

HRESULT
ServerRemoteCodeGen(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 threadContextInfoAddress,
    /* [in] */ __int3264 scriptContextInfoAddress,
    /* [in] */ CodeGenWorkItemIDL *workItemData,
    /* [out] */ JITOutputIDL *jitData)
{
    UNREFERENCED_PARAMETER(binding);
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerThreadContext * threadContextInfo = reinterpret_cast<ServerThreadContext*>(threadContextInfoAddress);
    ServerScriptContext * scriptContextInfo = reinterpret_cast<ServerScriptContext*>(scriptContextInfoAddress);

    if (threadContextInfo == nullptr || scriptContextInfo == nullptr)
    {
        return RPC_S_INVALID_ARG;
    }

    PageAllocator backgroundPageAllocator(threadContextInfo->GetAllocationPolicyManager(), Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
        (AutoSystemInfo::Data.IsLowMemoryProcess() ?
            PageAllocator::DefaultLowMaxFreePageCount :
            PageAllocator::DefaultMaxFreePageCount));

    NoRecoverMemoryJitArenaAllocator jitArena(L"JITArena", &backgroundPageAllocator, Js::Throw::OutOfMemory);    

    scriptContextInfo->BeginJIT(); // TODO: OOP JIT, improve how we do this
    threadContextInfo->BeginJIT();

    JITTimeWorkItem * jitWorkItem = Anew(&jitArena, JITTimeWorkItem, workItemData);

    jitData->numberPageSegments = (XProcNumberPageSegment*)midl_user_allocate(sizeof(XProcNumberPageSegment));
    memcpy_s(jitData->numberPageSegments, sizeof(XProcNumberPageSegment), &jitWorkItem->GetWorkItemData()->xProcNumberPageSegment, sizeof(XProcNumberPageSegment));

    Func::Codegen(&jitArena, jitWorkItem, threadContextInfo, scriptContextInfo, jitData, nullptr, jitWorkItem->GetPolymorphicInlineCacheInfo(), threadContextInfo->GetCodeGenAllocators(), nullptr, nullptr, true);

    scriptContextInfo->EndJIT();
    threadContextInfo->EndJIT();
    return S_OK;
}

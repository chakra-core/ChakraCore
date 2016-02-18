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
    /* [in] */ __RPC__in ThreadContextData * threadContextData,
    /* [out] */ __RPC__out __int3264 *threadContextRoot)
{
    AUTO_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ThreadContextInfo * contextInfo = HeapNew(ThreadContextInfo, threadContextData);
    *threadContextRoot = (intptr_t)contextInfo;
    return S_OK;
}

HRESULT
ServerCleanupThreadContext(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 threadContextRoot)
{
    ThreadContextInfo * contextInfo = reinterpret_cast<ThreadContextInfo*>(threadContextRoot);
    HeapDelete(contextInfo);
    return S_OK;
}

HRESULT
ServerInitializeScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in ScriptContextData * scriptContextData,
    /* [out] */ __RPC__out __int3264 * scriptContextInfoAddress)
{
    AUTO_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ScriptContextInfo * contextInfo = HeapNew(ScriptContextInfo, scriptContextData);
    *scriptContextInfoAddress = (intptr_t)contextInfo;
    return S_OK;
}

HRESULT
ServerCleanupScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 scriptContextRoot)
{
    ScriptContextInfo * contextInfo = reinterpret_cast<ScriptContextInfo*>(scriptContextRoot);
    HeapDelete(contextInfo);
    return S_OK;
}

HRESULT
ServerRemoteCodeGen(
    /* [in] */ handle_t binding,
    /* [in] */ __int3264 threadContextInfoAddress,
    /* [in] */ __int3264 scriptContextInfoAddress,
    /* [in] */ CodeGenWorkItemJITData *workItemData,
    /* [out] */ JITOutputData *jitData,
    /* [optional, in] */ ProfileData * profileData)
{
    UNREFERENCED_PARAMETER(binding);
    AUTO_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    AllocationPolicyManager * policyManager = HeapNew(AllocationPolicyManager, true);
    // TODO: store this
    PageAllocator backgroundPageAllocator(policyManager, Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
        (AutoSystemInfo::Data.IsLowMemoryProcess() ?
            PageAllocator::DefaultLowMaxFreePageCount :
            PageAllocator::DefaultMaxFreePageCount));
    JitArenaAllocator jitArena(L"JITArena", &backgroundPageAllocator, Js::Throw::OutOfMemory);

    JITTimeWorkItem * jitWorkItem = Anew(&jitArena, JITTimeWorkItem, workItemData);
    ThreadContextInfo * threadContextInfo = reinterpret_cast<ThreadContextInfo*>(threadContextInfoAddress);
    ScriptContextInfo * scriptContextInfo = reinterpret_cast<ScriptContextInfo*>(scriptContextInfoAddress);

    JITTimeProfileInfo profileInfo(profileData);
    Func func(&jitArena, jitWorkItem, threadContextInfo, scriptContextInfo, nullptr, nullptr, nullptr, nullptr, nullptr, &profileInfo, nullptr, true);
    func.m_symTable->SetStartingID(static_cast<SymID>(jitWorkItem->GetJITFunctionBody()->GetLocalsCount() + 1));
    func.Codegen();
    return S_OK;
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JITServerPch.h"

__declspec(dllexport)
HRESULT JsInitializeJITServer(
    __in UUID* connectionUuid,
    __in_opt void* securityDescriptor,
    __in_opt void* alpcSecurityDescriptor)
{
    RPC_STATUS status;
    RPC_BINDING_VECTOR* bindingVector = NULL;
    UUID_VECTOR uuidVector;

    uuidVector.Count = 1;
    uuidVector.Uuid[0] = connectionUuid;

    status = RpcServerUseProtseqW(
        (RPC_WSTR)L"ncalrpc",
        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
        alpcSecurityDescriptor);
    if (status != RPC_S_OK)
    {
        return status;
    }

#ifndef NTBUILD
    status = RpcServerRegisterIf2(
        ServerIChakraJIT_v0_0_s_ifspec,
        NULL,
        NULL,
        RPC_IF_AUTOLISTEN,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
        (ULONG)-1,
        NULL);
#else
    status = RpcServerRegisterIf3(
        ServerIChakraJIT_v0_0_s_ifspec,
        NULL,
        NULL,
        RPC_IF_AUTOLISTEN,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
        (ULONG)-1,
        NULL,
        securityDescriptor);
#endif
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
    JITManager::GetJITManager()->SetIsJITServer();

    status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);

    return status;
}

HRESULT
ShutdownCommon()
{
    HRESULT status = RpcMgmtStopServerListening(NULL);
    if (status != RPC_S_OK)
    {
        return status;
    }

    status = RpcServerUnregisterIf(ServerIChakraJIT_v0_0_s_ifspec, NULL, FALSE);
    return status;
}

__declspec(dllexport)
HRESULT
JsShutdownJITServer()
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());

    if (JITManager::GetJITManager()->IsConnected())
    {
        // if client is hosting jit process directly, call to remotely shutdown
        return JITManager::GetJITManager()->Shutdown();
    }
    else
    {
        return ShutdownCommon();
    }
}

HRESULT
ServerShutdown(
    /* [in] */ handle_t binding)
{
    return ShutdownCommon();
}

HRESULT
ServerCleanupProcess(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t processHandle)
{
    CloseHandle((HANDLE)processHandle);
    return S_OK;
}

HRESULT
ServerInitializeThreadContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in ThreadContextDataIDL * threadContextData,
    /* [out] */ __RPC__out intptr_t *threadContextRoot,
    /* [out] */ __RPC__out intptr_t *prereservedRegionAddr)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerThreadContext * contextInfo = HeapNewNoThrow(ServerThreadContext, threadContextData);
    if (contextInfo == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    AutoReleaseContext<ServerThreadContext> autoThreadContext(contextInfo);
    return ServerCallWrapper(contextInfo, [&]()->HRESULT
    {
        ServerContextManager::RegisterThreadContext(contextInfo);

        *threadContextRoot = (intptr_t)EncodePointer(contextInfo);
        *prereservedRegionAddr = (intptr_t)contextInfo->GetPreReservedVirtualAllocator()->EnsurePreReservedRegion();

        return S_OK;
    });
}

HRESULT
ServerCleanupThreadContext(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t threadContextRoot)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerThreadContext * threadContextInfo = (ServerThreadContext*)DecodePointer((void*)threadContextRoot);
    if (threadContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (ServerContextManager::IsThreadContextAlive(threadContextInfo))
    {
        AutoReleaseContext<ServerThreadContext> autoThreadContext(threadContextInfo);
        threadContextInfo->Close();
        ServerContextManager::UnRegisterThreadContext(threadContextInfo);
    }
    return S_OK;
}

HRESULT
ServerUpdatePropertyRecordMap(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t threadContextInfoAddress,
    /* [in] */ __RPC__in UpdatedPropertysIDL * updatedProps)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerThreadContext * threadContextInfo = (ServerThreadContext*)DecodePointer((void*)threadContextInfoAddress);

    if (threadContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsThreadContextAlive(threadContextInfo))
    {
        Assert(false);
        return E_ACCESSDENIED;
    }

    AutoReleaseContext<ServerThreadContext> autoThreadContext(threadContextInfo);
    return ServerCallWrapper(threadContextInfo, [&]()->HRESULT
    {
        for (uint i = 0; i < updatedProps->reclaimedPropertyCount; ++i)
        {
            threadContextInfo->RemoveFromPropertyMap((Js::PropertyId)updatedProps->reclaimedPropertyIdArray[i]);
        }

        for (uint i = 0; i < updatedProps->newRecordCount; ++i)
        {
            threadContextInfo->AddToPropertyMap((Js::PropertyRecord *)updatedProps->newRecordArray[i]);
        }

        return S_OK;
    });
}

HRESULT
ServerAddDOMFastPathHelper(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t scriptContextRoot,
    /* [in] */ intptr_t funcInfoAddr,
    /* [in] */ int helper)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer((void*)scriptContextRoot);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsScriptContextAlive(scriptContextInfo))
    {
        Assert(false);
        return E_ACCESSDENIED;
    }

    AutoReleaseContext<ServerScriptContext> autoScriptContext(scriptContextInfo);
    return ServerCallWrapper(scriptContextInfo, [&]()->HRESULT
    {
        scriptContextInfo->AddToDOMFastPathHelperMap(funcInfoAddr, (IR::JnHelperMethod)helper);
        return S_OK;
    });
}

HRESULT
ServerAddModuleRecordInfo(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t scriptContextInfoAddress,
    /* [in] */ unsigned int moduleId,
    /* [in] */ intptr_t localExportSlotsAddr)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerScriptContext * serverScriptContext = (ServerScriptContext*)DecodePointer((void*)scriptContextInfoAddress);
    if (serverScriptContext == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsScriptContextAlive(serverScriptContext))
    {
        Assert(false);
        return E_ACCESSDENIED;
    }

    AutoReleaseContext<ServerScriptContext> autoScriptContext(serverScriptContext);
    return ServerCallWrapper(serverScriptContext, [&]()->HRESULT
    {
        serverScriptContext->AddModuleRecordInfo(moduleId, localExportSlotsAddr);
        return S_OK;
    });

}

HRESULT 
ServerSetWellKnownHostTypeId(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t threadContextRoot,
    /* [in] */ int typeId)
{
    ServerThreadContext * threadContextInfo = (ServerThreadContext*)DecodePointer((void*)threadContextRoot);

    if (threadContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsThreadContextAlive(threadContextInfo))
    {
        Assert(false);
        return E_ACCESSDENIED;
    }

    AutoReleaseContext<ServerThreadContext> autoThreadContext(threadContextInfo);
    threadContextInfo->SetWellKnownHostTypeId((Js::TypeId)typeId);

    return S_OK;
}

HRESULT
ServerInitializeScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in ScriptContextDataIDL * scriptContextData,
    /* [in] */ intptr_t threadContextInfoAddress,
    /* [out] */ __RPC__out intptr_t * scriptContextInfoAddress)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

    ServerThreadContext * threadContextInfo = (ServerThreadContext*)DecodePointer((void*)threadContextInfoAddress);

    if (threadContextInfo == nullptr)
    {
        Assert(false);
        *scriptContextInfoAddress = 0;
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsThreadContextAlive(threadContextInfo))
    {
        Assert(false);
        *scriptContextInfoAddress = 0;
        return E_ACCESSDENIED;
    }

    return ServerCallWrapper(threadContextInfo, [&]()->HRESULT
    {
        ServerScriptContext * contextInfo = HeapNew(ServerScriptContext, scriptContextData, threadContextInfo);

        ServerContextManager::RegisterScriptContext(contextInfo);

        *scriptContextInfoAddress = (intptr_t)EncodePointer(contextInfo);

#if !FLOATVAR
        // TODO: should move this to ServerInitializeThreadContext, also for the fields in IDL
        XProcNumberPageSegmentImpl::Initialize(contextInfo->IsRecyclerVerifyEnabled(), contextInfo->GetRecyclerVerifyPad());
#endif
        return S_OK;
    });
}

HRESULT
ServerCloseScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t scriptContextRoot)
{
    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer((void*)scriptContextRoot);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (ServerContextManager::IsScriptContextAlive(scriptContextInfo))
    {
        AutoReleaseContext<ServerScriptContext> autoScriptContext(scriptContextInfo);

#ifdef PROFILE_EXEC
        auto profiler = scriptContextInfo->GetCodeGenProfiler();
        if (profiler && profiler->IsInitialized())
        {
            profiler->ProfilePrint(Js::Configuration::Global.flags.Profile.GetFirstPhase());
        }
#endif

        scriptContextInfo->Close();
        ServerContextManager::UnRegisterScriptContext(scriptContextInfo);
    }

    return S_OK;
}

HRESULT
ServerCleanupScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t scriptContextRoot)
{
    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer((void*)scriptContextRoot);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (ServerContextManager::IsScriptContextAlive(scriptContextInfo))
    {
        AutoReleaseContext<ServerScriptContext> autoScriptContext(scriptContextInfo);        
        scriptContextInfo->Close();
        ServerContextManager::UnRegisterScriptContext(scriptContextInfo);
    }

    return S_OK;
}

HRESULT
ServerFreeAllocation(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t threadContextInfo,
    /* [in] */ intptr_t address)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
    ServerThreadContext * context = (ServerThreadContext*)DecodePointer((void*)threadContextInfo);

    if (context == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsThreadContextAlive(context))
    {
        Assert(false);
        return E_ACCESSDENIED;
    }

    return ServerCallWrapper(context, [&]()->HRESULT 
    {
        AutoReleaseContext<ServerThreadContext> autoThreadContext(context);
        context->SetValidCallTargetForCFG((PVOID)address, false);
        context->GetCodeGenAllocators()->emitBufferManager.FreeAllocation((void*)address);
        return S_OK;
    });
}

HRESULT
ServerIsNativeAddr(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t threadContextInfo,
    /* [in] */ intptr_t address,
    /* [out] */ __RPC__out boolean * result)
{
    ServerThreadContext * context = (ServerThreadContext*)DecodePointer((void*)threadContextInfo);

    if (context == nullptr)
    {
        *result = false;
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsThreadContextAlive(context))
    {
        *result = false;
        Assert(false);
        return E_ACCESSDENIED;
    }

    AutoReleaseContext<ServerThreadContext> autoThreadContext(context);
    return ServerCallWrapper(context, [&]()->HRESULT
    {
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
    });
}

HRESULT
ServerSetIsPRNGSeeded(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t scriptContextInfoAddress,
    /* [in] */ boolean value)
{
    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer((void*)scriptContextInfoAddress);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsScriptContextAlive(scriptContextInfo))
    {
        Assert(false);
        return E_ACCESSDENIED;
    }

    AutoReleaseContext<ServerScriptContext> autoScriptContext(scriptContextInfo);
    scriptContextInfo->SetIsPRNGSeeded(value != FALSE);
    return S_OK;
}

HRESULT
ServerRemoteCodeGen(
    /* [in] */ handle_t binding,
    /* [in] */ intptr_t scriptContextInfoAddress,
    /* [in] */ __RPC__in CodeGenWorkItemIDL *workItemData,
    /* [out] */ __RPC__out JITOutputIDL *jitData)
{
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
    LARGE_INTEGER start_time = { 0 };
    if (PHASE_TRACE1(Js::BackEndPhase))
    {
        QueryPerformanceCounter(&start_time);
    }
    memset(jitData, 0, sizeof(JITOutputIDL));

    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer((void*)scriptContextInfoAddress);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!ServerContextManager::IsScriptContextAlive(scriptContextInfo))
    {
        Assert(false);
        return E_ACCESSDENIED;
    }

    AutoReleaseContext<ServerScriptContext> autoScriptContext(scriptContextInfo);

    return ServerCallWrapper(scriptContextInfo, [&]() ->HRESULT
    {
        scriptContextInfo->UpdateGlobalObjectThisAddr(workItemData->globalThisAddr);
        ServerThreadContext * threadContextInfo = scriptContextInfo->GetThreadContext();

        NoRecoverMemoryJitArenaAllocator jitArena(L"JITArena", threadContextInfo->GetPageAllocator(), Js::Throw::OutOfMemory);
        JITTimeWorkItem * jitWorkItem = Anew(&jitArena, JITTimeWorkItem, workItemData);

        if (PHASE_VERBOSE_TRACE_RAW(Js::BackEndPhase, jitWorkItem->GetJITTimeInfo()->GetSourceContextId(), jitWorkItem->GetJITTimeInfo()->GetLocalFunctionId()))
        {
            LARGE_INTEGER freq;
            LARGE_INTEGER end_time;
            QueryPerformanceCounter(&end_time);
            QueryPerformanceFrequency(&freq);

            Output::Print(
                L"BackendMarshalIn - function: %s time:%8.6f mSec\r\n",
                jitWorkItem->GetJITFunctionBody()->GetDisplayName(),
                (((double)((end_time.QuadPart - workItemData->startTime)* (double)1000.0 / (double)freq.QuadPart))) / (1));
            Output::Flush();
        }

        auto profiler = scriptContextInfo->GetCodeGenProfiler();
#ifdef PROFILE_EXEC
        if (profiler && !profiler->IsInitialized())
        {
            profiler->Initialize(threadContextInfo->GetPageAllocator(), nullptr);
        }
#endif
        if (jitWorkItem->GetWorkItemData()->xProcNumberPageSegment)
        {
            jitData->numberPageSegments = (XProcNumberPageSegment*)midl_user_allocate(sizeof(XProcNumberPageSegment));
            if (!jitData->numberPageSegments)
            {
                return E_OUTOFMEMORY;
            }
            __analysis_assume(jitData->numberPageSegments);

            memcpy_s(jitData->numberPageSegments, sizeof(XProcNumberPageSegment), jitWorkItem->GetWorkItemData()->xProcNumberPageSegment, sizeof(XProcNumberPageSegment));
        }

        Func::Codegen(
            &jitArena,
            jitWorkItem,
            threadContextInfo,
            scriptContextInfo,
            jitData,
            nullptr,
            nullptr,
            jitWorkItem->GetPolymorphicInlineCacheInfo(),
            threadContextInfo->GetCodeGenAllocators(),
#if !FLOATVAR
            nullptr, // number allocator
#endif
            profiler,
            true);


#ifdef PROFILE_EXEC
        if (profiler && profiler->IsInitialized())
        {
            profiler->ProfilePrint(Js::Configuration::Global.flags.Profile.GetFirstPhase());
        }
#endif

        if (PHASE_VERBOSE_TRACE_RAW(Js::BackEndPhase, jitWorkItem->GetJITTimeInfo()->GetSourceContextId(), jitWorkItem->GetJITTimeInfo()->GetLocalFunctionId()))
        {
            LARGE_INTEGER freq;
            LARGE_INTEGER end_time;
            QueryPerformanceCounter(&end_time);
            QueryPerformanceFrequency(&freq);

            Output::Print(
                L"EndBackEndInner - function: %s time:%8.6f mSec\r\n",
                jitWorkItem->GetJITFunctionBody()->GetDisplayName(),
                (((double)((end_time.QuadPart - start_time.QuadPart)* (double)1000.0 / (double)freq.QuadPart))) / (1));
            Output::Flush();

        }
        LARGE_INTEGER out_time = { 0 };
        if (PHASE_TRACE1(Js::BackEndPhase))
        {
            QueryPerformanceCounter(&out_time);
            jitData->startTime = out_time.QuadPart;
        }

        return S_OK;
    });
}

JsUtil::BaseHashSet<ServerThreadContext*, HeapAllocator> ServerContextManager::threadContexts(&HeapAllocator::Instance);
JsUtil::BaseHashSet<ServerScriptContext*, HeapAllocator> ServerContextManager::scriptContexts(&HeapAllocator::Instance);
CriticalSection ServerContextManager::cs;

void ServerContextManager::RegisterThreadContext(ServerThreadContext* threadContext)
{
    AutoCriticalSection autoCS(&cs);
    threadContexts.Add(threadContext);
}

void ServerContextManager::UnRegisterThreadContext(ServerThreadContext* threadContext)
{
    AutoCriticalSection autoCS(&cs);
    threadContexts.Remove(threadContext);
    auto iter = scriptContexts.GetIteratorWithRemovalSupport();
    while (iter.IsValid())
    {
        ServerScriptContext* scriptContext = iter.Current().Key();
        if (scriptContext->GetThreadContext() == threadContext)
        {   
            scriptContext->Close();
            iter.RemoveCurrent();
        }
        iter.MoveNext();
    }
}

bool ServerContextManager::IsThreadContextAlive(ServerThreadContext* threadContext)
{
    AutoCriticalSection autoCS(&cs);
    if (threadContexts.LookupWithKey(threadContext)) 
    {
        return !threadContext->IsClosed();
    }
    return false;
}

void ServerContextManager::RegisterScriptContext(ServerScriptContext* scriptContext)
{
    AutoCriticalSection autoCS(&cs);
    scriptContexts.Add(scriptContext);
}

void ServerContextManager::UnRegisterScriptContext(ServerScriptContext* scriptContext)
{
    AutoCriticalSection autoCS(&cs);
    scriptContexts.Remove(scriptContext);
}

bool ServerContextManager::IsScriptContextAlive(ServerScriptContext* scriptContext)
{
    AutoCriticalSection autoCS(&cs);
    if (scriptContexts.LookupWithKey(scriptContext))
    {
        return !scriptContext->IsClosed();
    }
    return false;
}

template<typename Fn>
HRESULT ServerCallWrapper(ServerThreadContext* threadContextInfo, Fn fn)
{
    MemoryOperationLastError::ClearLastError();
    HRESULT hr = S_OK;
    try
    {
        hr = fn();
    }
    catch (Js::OutOfMemoryException)
    {
        hr = E_OUTOFMEMORY;
    }
    catch (Js::StackOverflowException)
    {
        hr = VBSERR_OutOfStack;
    }
    catch (Js::OperationAbortedException)
    {
        hr = E_ABORT;
    }
    catch (Js::InternalErrorException)
    {
        hr = E_FAIL;
    }
    catch (...)
    {
        AssertMsg(false, "Unknown exception caught in JIT server call.");
        hr = E_FAIL;
    }

    DWORD exitCode = STILL_ACTIVE;
    if (!GetExitCodeProcess(threadContextInfo->GetProcessHandle(), &exitCode))
    {
        Assert(false); // fail to check target process
        hr = E_FAIL;
    }

    if (exitCode != STILL_ACTIVE)
    {
        threadContextInfo->Close();
        ServerContextManager::UnRegisterThreadContext(threadContextInfo);
        hr = E_FAIL;
    }

    if (hr == E_OUTOFMEMORY || hr == E_FAIL)
    {
        if (HRESULT_FROM_WIN32(MemoryOperationLastError::GetLastError()) != S_OK)
        {
            hr = HRESULT_FROM_WIN32(MemoryOperationLastError::GetLastError());
        }
    }

    return hr;
}

template<typename Fn>
HRESULT ServerCallWrapper(ServerScriptContext* scriptContextInfo, Fn fn)
{
    ServerThreadContext* threadContextInfo = scriptContextInfo->GetThreadContext();
    if (!ServerContextManager::IsThreadContextAlive(threadContextInfo))
    {
        Assert(false);
        return E_ACCESSDENIED;
    }
    AutoReleaseContext<ServerThreadContext> autoThreadContext(threadContextInfo);
    return ServerCallWrapper(threadContextInfo, fn);
}
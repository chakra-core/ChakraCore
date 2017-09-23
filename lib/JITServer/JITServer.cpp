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

    JITManager::GetJITManager()->SetIsJITServer();
    PageAllocatorPool::Initialize();

    status = RpcEpRegister(
        ServerIChakraJIT_v0_0_s_ifspec,
        bindingVector,
        &uuidVector,
        NULL);
    if (status != RPC_S_OK)
    {
        return status;
    }

    status = RpcBindingVectorFree(&bindingVector);

    if (status != RPC_S_OK)
    {
        return status;
    }

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

    ServerContextManager::Shutdown();
    PageAllocatorPool::Shutdown();
    return status;
}

HRESULT
ServerShutdown(
    /* [in] */ handle_t binding)
{
    return ShutdownCommon();
}

void
__RPC_USER PTHREADCONTEXT_HANDLE_rundown(__RPC__in PTHREADCONTEXT_HANDLE phContext)
{
    ServerCleanupThreadContext(nullptr, &phContext);
}

void
__RPC_USER PSCRIPTCONTEXT_HANDLE_rundown(__RPC__in PSCRIPTCONTEXT_HANDLE phContext)
{
    ServerCloseScriptContext(nullptr, phContext);
    ServerCleanupScriptContext(nullptr, &phContext);
}

HRESULT CheckModuleAddress(HANDLE process, LPCVOID remoteImageBase, LPCVOID localImageBase)
{
    byte remoteImageHeader[0x1000];
    MEMORY_BASIC_INFORMATION remoteImageInfo;
    SIZE_T resultBytes = VirtualQueryEx(process, (LPCVOID)remoteImageBase, &remoteImageInfo, sizeof(remoteImageInfo));
    if (resultBytes != sizeof(remoteImageInfo))
    {
        return E_ACCESSDENIED;
    }
    if (remoteImageInfo.BaseAddress != (PVOID)remoteImageBase)
    {
        return E_ACCESSDENIED;
    }
    if (remoteImageInfo.Type != MEM_IMAGE)
    {
        return E_ACCESSDENIED;
    }
    if (remoteImageInfo.State != MEM_COMMIT)
    {
        return E_ACCESSDENIED;
    }

    if (remoteImageInfo.RegionSize < sizeof(remoteImageHeader))
    {
        return E_ACCESSDENIED;
    }

    if (!ReadProcessMemory(process, remoteImageBase, remoteImageHeader, sizeof(remoteImageHeader), &resultBytes))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (resultBytes < sizeof(remoteImageHeader))
    {
        return E_ACCESSDENIED;
    }
    PIMAGE_DOS_HEADER localDosHeader = (PIMAGE_DOS_HEADER)localImageBase;
    PIMAGE_NT_HEADERS localNtHeader = (PIMAGE_NT_HEADERS)((BYTE*)localDosHeader + localDosHeader->e_lfanew);

    PIMAGE_DOS_HEADER remoteDosHeader = (PIMAGE_DOS_HEADER)remoteImageHeader;
    PIMAGE_NT_HEADERS remoteNtHeader = (PIMAGE_NT_HEADERS)((BYTE*)remoteDosHeader + remoteDosHeader->e_lfanew);

    uintptr_t remoteHeaderMax = (uintptr_t)remoteImageHeader + sizeof(remoteImageHeader);
    uintptr_t remoteMaxRead = (uintptr_t)remoteNtHeader + sizeof(IMAGE_NT_HEADERS);
    if (remoteMaxRead >= remoteHeaderMax || remoteMaxRead < (uintptr_t)remoteImageHeader)
    {
        return E_ACCESSDENIED;
    }

    if (localNtHeader->FileHeader.NumberOfSections != remoteNtHeader->FileHeader.NumberOfSections)
    {
        return E_ACCESSDENIED;
    }
    if (localNtHeader->FileHeader.NumberOfSymbols != remoteNtHeader->FileHeader.NumberOfSymbols)
    {
        return E_ACCESSDENIED;
    }
    if (localNtHeader->OptionalHeader.CheckSum != remoteNtHeader->OptionalHeader.CheckSum)
    {
        return E_ACCESSDENIED;
    }
    if (localNtHeader->OptionalHeader.SizeOfImage != remoteNtHeader->OptionalHeader.SizeOfImage)
    {
        return E_ACCESSDENIED;
    }

    return S_OK;
}

#pragma warning(push)
#pragma warning(disable:6387 28196) // PREFast does not understand the out context can be null here
HRESULT
ServerInitializeThreadContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in ThreadContextDataIDL * threadContextData,
#ifdef USE_RPC_HANDLE_MARSHALLING
    /* [in] */ __RPC__in HANDLE processHandle,
#endif
    /* [out] */ __RPC__deref_out_opt PPTHREADCONTEXT_HANDLE threadContextInfoAddress,
    /* [out] */ __RPC__out intptr_t *prereservedRegionAddr,
    /* [out] */ __RPC__out intptr_t *jitThunkAddr)
{
    if (threadContextInfoAddress == nullptr || prereservedRegionAddr == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    *threadContextInfoAddress = nullptr;
    *prereservedRegionAddr = 0;
    *jitThunkAddr = 0;

    ServerThreadContext * contextInfo = nullptr;

    DWORD clientPid;
    HRESULT hr = HRESULT_FROM_WIN32(I_RpcBindingInqLocalClientPID(binding, &clientPid));
    if (FAILED(hr))
    {
        return hr;
    }
#ifdef USE_RPC_HANDLE_MARSHALLING
    HANDLE targetHandle;
    if (!DuplicateHandle(GetCurrentProcess(), processHandle, GetCurrentProcess(), &targetHandle, 0, false, DUPLICATE_SAME_ACCESS))
    {
        return E_ACCESSDENIED;
    }
#else
    HANDLE targetHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_LIMITED_INFORMATION, false, clientPid);
    if (!targetHandle)
    {
        return E_ACCESSDENIED;
    }
#endif
    try
    {
        AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory));
        contextInfo = HeapNew(ServerThreadContext, threadContextData, targetHandle);
        ServerContextManager::RegisterThreadContext(contextInfo);
    }
    catch (Js::OutOfMemoryException)
    {
        CloseHandle(targetHandle);
        return E_OUTOFMEMORY;
    }

    return ServerCallWrapper(contextInfo, [&]()->HRESULT
    {
        if (clientPid != contextInfo->GetRuntimePid())
        {
            return E_ACCESSDENIED;
        }
        hr = CheckModuleAddress(targetHandle, (LPCVOID)contextInfo->GetRuntimeChakraBaseAddress(), (LPCVOID)AutoSystemInfo::Data.dllLoadAddress);
        if (FAILED(hr))
        {
            return hr;
        }
        hr = CheckModuleAddress(targetHandle, (LPCVOID)contextInfo->GetRuntimeCRTBaseAddress(), (LPCVOID)contextInfo->GetJITCRTBaseAddress());
        if (FAILED(hr))
        {
            return hr;
        }

        *threadContextInfoAddress = (PTHREADCONTEXT_HANDLE)EncodePointer(contextInfo);

#if defined(_CONTROL_FLOW_GUARD)
        if (!PHASE_OFF1(Js::PreReservedHeapAllocPhase))
        {
            *prereservedRegionAddr = (intptr_t)contextInfo->GetPreReservedSectionAllocator()->EnsurePreReservedRegion();
        }
#if _M_IX86 || _M_X64
        *jitThunkAddr = (intptr_t)contextInfo->GetJITThunkEmitter()->EnsureInitialized();
#endif
#endif

        return hr;
    });
}

HRESULT
ServerInitializeScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in ScriptContextDataIDL * scriptContextData,
    /* [in] */ __RPC__in PTHREADCONTEXT_HANDLE threadContextInfoAddress,
    /* [out] */ __RPC__deref_out_opt PPSCRIPTCONTEXT_HANDLE scriptContextInfoAddress)
{
    if (scriptContextInfoAddress == nullptr || threadContextInfoAddress == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    *scriptContextInfoAddress = nullptr;
    ServerThreadContext * threadContextInfo = (ServerThreadContext*)DecodePointer(threadContextInfoAddress);

    return ServerCallWrapper(threadContextInfo, [&]()->HRESULT
    {
        ServerScriptContext * contextInfo = HeapNew(ServerScriptContext, scriptContextData, threadContextInfo);
        ServerContextManager::RegisterScriptContext(contextInfo);
        *scriptContextInfoAddress = (PSCRIPTCONTEXT_HANDLE)EncodePointer(contextInfo);

#if !FLOATVAR
        // TODO: should move this to ServerInitializeThreadContext, also for the fields in IDL
        XProcNumberPageSegmentImpl::Initialize(contextInfo->IsRecyclerVerifyEnabled(), contextInfo->GetRecyclerVerifyPad());
#endif
        return S_OK;
    });
}
#pragma warning(pop)

HRESULT
ServerCleanupThreadContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__deref_inout_opt PPTHREADCONTEXT_HANDLE threadContextInfoAddress)
{
    if (threadContextInfoAddress == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }
    ServerThreadContext * threadContextInfo = (ServerThreadContext*)DecodePointer(*threadContextInfoAddress);
    if (threadContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    // This tells the run-time, when it is marshalling the out
    // parameters, that the context handle has been closed normally.
    *threadContextInfoAddress = nullptr;

    return ServerCallWrapper(threadContextInfo, [&]()->HRESULT
    {
        threadContextInfo->Close();
        ServerContextManager::UnRegisterThreadContext(threadContextInfo);
        return S_OK;
    });
}

HRESULT
ServerUpdatePropertyRecordMap(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PTHREADCONTEXT_HANDLE threadContextInfoAddress,
    /* [in] */ __RPC__in_opt BVSparseNodeIDL * updatedPropsBVHead)
{
    ServerThreadContext * threadContextInfo = (ServerThreadContext*)DecodePointer(threadContextInfoAddress);

    if (threadContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(threadContextInfo, [&]()->HRESULT
    {
        typedef ServerThreadContext::BVSparseNode BVSparseNode;
        CompileAssert(sizeof(BVSparseNode) == sizeof(BVSparseNodeIDL));
        threadContextInfo->UpdateNumericPropertyBV((BVSparseNode*)updatedPropsBVHead);

        return S_OK;
    });
}

HRESULT
ServerAddDOMFastPathHelper(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    /* [in] */ intptr_t funcInfoAddr,
    /* [in] */ int helper)
{
    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer(scriptContextInfoAddress);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(scriptContextInfo, [&]()->HRESULT
    {
        scriptContextInfo->AddToDOMFastPathHelperMap(funcInfoAddr, (IR::JnHelperMethod)helper);
        return S_OK;
    });
}

HRESULT
ServerAddModuleRecordInfo(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    /* [in] */ unsigned int moduleId,
    /* [in] */ intptr_t localExportSlotsAddr)
{
    ServerScriptContext * serverScriptContext = (ServerScriptContext*)DecodePointer(scriptContextInfoAddress);
    if (serverScriptContext == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(serverScriptContext, [&]()->HRESULT
    {
        serverScriptContext->AddModuleRecordInfo(moduleId, localExportSlotsAddr);
        return S_OK;
    });

}

HRESULT
ServerSetWellKnownHostTypeId(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PTHREADCONTEXT_HANDLE threadContextInfoAddress,
    /* [in] */ int typeId)
{
    ServerThreadContext * threadContextInfo = (ServerThreadContext*)DecodePointer(threadContextInfoAddress);

    if (threadContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(threadContextInfo, [&]()->HRESULT
    {
        threadContextInfo->SetWellKnownHostTypeId((Js::TypeId)typeId);
        return S_OK;
    });
}

HRESULT
ServerCleanupScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__deref_inout_opt PPSCRIPTCONTEXT_HANDLE scriptContextInfoAddress)
{
    if (scriptContextInfoAddress == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer(*scriptContextInfoAddress);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    if (!scriptContextInfo->IsClosed())
    {
        scriptContextInfo->Close();
        ServerContextManager::UnRegisterScriptContext(scriptContextInfo);
    }
    // This tells the run-time, when it is marshalling the out
    // parameters, that the context handle has been closed normally.
    *scriptContextInfoAddress = nullptr;

    HeapDelete(scriptContextInfo);

    return S_OK;
}

HRESULT
ServerCloseScriptContext(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress)
{
    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer(scriptContextInfoAddress);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(scriptContextInfo, [&]()->HRESULT
    {
#ifdef PROFILE_EXEC
        auto profiler = scriptContextInfo->GetCodeGenProfiler();
        if (profiler && profiler->IsInitialized())
        {
            profiler->ProfilePrint(Js::Configuration::Global.flags.Profile.GetFirstPhase());
        }
#endif
        scriptContextInfo->Close();
        ServerContextManager::UnRegisterScriptContext(scriptContextInfo);

        return S_OK;
    });
}

HRESULT
ServerDecommitInterpreterBufferManager(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    /* [in] */ boolean asmJsManager)
{
    ServerScriptContext * scriptContext = (ServerScriptContext *)DecodePointer((void*)scriptContextInfoAddress);

    if (scriptContext == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(scriptContext, [&]()->HRESULT
    {
        scriptContext->DecommitEmitBufferManager(asmJsManager != FALSE);
        return S_OK;
    });
}

HRESULT
ServerNewInterpreterThunkBlock(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PSCRIPTCONTEXT_HANDLE scriptContextInfo,
    /* [in] */ __RPC__in InterpreterThunkInputIDL * thunkInput,
    /* [out] */ __RPC__out InterpreterThunkOutputIDL * thunkOutput)
{
    memset(thunkOutput, 0, sizeof(InterpreterThunkOutputIDL));

    ServerScriptContext * scriptContext = (ServerScriptContext *)DecodePointer(scriptContextInfo);
    if (scriptContext == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(scriptContext, [&]()->HRESULT
    {
        ServerThreadContext * threadContext = scriptContext->GetThreadContext();

        class AutoLocalAlloc
        {
        public:
            AutoLocalAlloc(ServerThreadContext * threadContext) : localAddress(nullptr), threadContext(threadContext) { }
            ~AutoLocalAlloc()
            {
                if (localAddress)
                {
                    threadContext->GetCodePageAllocators()->FreeLocal(this->localAddress, this->segment);
                }
            }
            char * localAddress;
            void * segment;
            ServerThreadContext * threadContext;
        } localAlloc(threadContext);

        OOPEmitBufferManager * emitBufferManager = scriptContext->GetEmitBufferManager(thunkInput->asmJsThunk != FALSE);

        BYTE* runtimeAddress;
        EmitBufferAllocation<SectionAllocWrapper, PreReservedSectionAllocWrapper> * alloc = emitBufferManager->AllocateBuffer(InterpreterThunkEmitter::BlockSize, &runtimeAddress);

        CompileAssert(InterpreterThunkEmitter::BlockSize <= CustomHeap::Page::MaxAllocationSize);
        localAlloc.segment = alloc->allocation->page->segment;

        localAlloc.localAddress = threadContext->GetCodePageAllocators()->AllocLocal((char*)runtimeAddress, InterpreterThunkEmitter::BlockSize, localAlloc.segment);
        if (!localAlloc.localAddress)
        {
            Js::Throw::OutOfMemory();
        }

#if PDATA_ENABLED
        PRUNTIME_FUNCTION pdataStart = {0};
        intptr_t epilogEnd = 0;
#endif
        DWORD thunkCount = 0;

        InterpreterThunkEmitter::FillBuffer(
            threadContext,
            thunkInput->asmJsThunk != FALSE,
            (intptr_t)runtimeAddress,
            InterpreterThunkEmitter::BlockSize,
            (BYTE*)localAlloc.localAddress,
#if PDATA_ENABLED
            &pdataStart,
            &epilogEnd,
#endif
            &thunkCount
        );

        emitBufferManager->CommitBufferForInterpreter(alloc, runtimeAddress, InterpreterThunkEmitter::BlockSize);
        // Call to set VALID flag for CFG check
        if (CONFIG_FLAG(OOPCFGRegistration))
        {
            threadContext->SetValidCallTargetForCFG(runtimeAddress);
        }

        thunkOutput->thunkCount = thunkCount;
        thunkOutput->mappedBaseAddr = (intptr_t)runtimeAddress;
#if PDATA_ENABLED
        thunkOutput->pdataTableStart = (intptr_t)pdataStart;
        thunkOutput->epilogEndAddr = epilogEnd;
#endif

        return S_OK;
    });
}

#if DBG
HRESULT
ServerIsInterpreterThunkAddr(
    /* [in] */ handle_t binding,
    /* [in] */ PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    /* [in] */ intptr_t address,
    /* [in] */ boolean asmjsThunk,
    /* [out] */ __RPC__out boolean * result)
{
    ServerScriptContext * context = (ServerScriptContext*)DecodePointer((void*)scriptContextInfoAddress);

    if (context == nullptr)
    {
        *result = false;
        return RPC_S_INVALID_ARG;
    }
    OOPEmitBufferManager * manager = context->GetEmitBufferManager(asmjsThunk != FALSE);
    if (manager == nullptr)
    {
        *result = false;
        return S_OK;
    }

    *result = manager->IsInHeap((void*)address);

    return S_OK;
}
#endif

HRESULT
ServerFreeAllocation(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PTHREADCONTEXT_HANDLE threadContextInfo,
    /* [in] */ intptr_t codeAddress,
    /* [in] */ intptr_t thunkAddress)
{
    ServerThreadContext * context = (ServerThreadContext*)DecodePointer(threadContextInfo);

    if (context == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(context, [&]()->HRESULT
    {
        if (CONFIG_FLAG(OOPCFGRegistration) && !thunkAddress)
        {
            context->SetValidCallTargetForCFG((PVOID)codeAddress, false);
        }
        context->GetCodeGenAllocators()->emitBufferManager.FreeAllocation((void*)codeAddress);
#if defined(_CONTROL_FLOW_GUARD) && (_M_IX86 || _M_X64)
        if (thunkAddress)
        {
            context->GetJITThunkEmitter()->FreeThunk(thunkAddress);
        }
#endif
        return S_OK;
    });
}

HRESULT
ServerIsNativeAddr(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PTHREADCONTEXT_HANDLE threadContextInfo,
    /* [in] */ intptr_t address,
    /* [out] */ __RPC__out boolean * result)
{
    if (result == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    *result = false;

    ServerThreadContext * context = (ServerThreadContext*)DecodePointer(threadContextInfo);
    if (context == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(context, [&]()->HRESULT
    {
        PreReservedSectionAllocWrapper *preReservedAllocWrapper = context->GetPreReservedSectionAllocator();
        if (preReservedAllocWrapper->IsInRange((void*)address))
        {
            *result = true;
        }
        else if (!context->IsAllJITCodeInPreReservedRegion())
        {
            AutoCriticalSection autoLock(&context->GetCodePageAllocators()->cs);
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
    /* [in] */ __RPC__in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    /* [in] */ boolean value)
{
    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer(scriptContextInfoAddress);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(scriptContextInfo, [&]()->HRESULT
    {
        scriptContextInfo->SetIsPRNGSeeded(value != FALSE);
        return S_OK;
    });
}

HRESULT
ServerRemoteCodeGen(
    /* [in] */ handle_t binding,
    /* [in] */ __RPC__in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    /* [in] */ __RPC__in CodeGenWorkItemIDL *workItemData,
    /* [out] */ __RPC__out JITOutputIDL *jitData)
{
    memset(jitData, 0, sizeof(JITOutputIDL));

    ServerScriptContext * scriptContextInfo = (ServerScriptContext*)DecodePointer(scriptContextInfoAddress);

    if (scriptContextInfo == nullptr)
    {
        Assert(false);
        return RPC_S_INVALID_ARG;
    }

    return ServerCallWrapper(scriptContextInfo, [&]() ->HRESULT
    {
        LARGE_INTEGER start_time = { 0 };
        if (PHASE_TRACE1(Js::BackEndPhase))
        {
            QueryPerformanceCounter(&start_time);
        }

        scriptContextInfo->UpdateGlobalObjectThisAddr(workItemData->globalThisAddr);
        ServerThreadContext * threadContextInfo = scriptContextInfo->GetThreadContext();

        AutoReturnPageAllocator autoReturnPageAllocator;
        PageAllocator* pageAllocator = autoReturnPageAllocator.GetPageAllocator();

        NoRecoverMemoryJitArenaAllocator jitArena(L"JITArena", pageAllocator, Js::Throw::OutOfMemory);
#if DBG
        jitArena.SetNeedsDelayFreeList();
#endif
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
            profiler->Initialize(pageAllocator, nullptr);
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

        Assert(jitData->codeAddress);
        Assert(jitData->codeSize);
        return S_OK;
    });
}

JsUtil::BaseHashSet<ServerThreadContext*, HeapAllocator> ServerContextManager::threadContexts(&HeapAllocator::Instance);
JsUtil::BaseHashSet<ServerScriptContext*, HeapAllocator> ServerContextManager::scriptContexts(&HeapAllocator::Instance);
CriticalSection ServerContextManager::cs;

#ifdef STACK_BACK_TRACE
SList<ServerContextManager::ClosedContextEntry<ServerThreadContext>*, NoThrowHeapAllocator> ServerContextManager::ClosedThreadContextList(&NoThrowHeapAllocator::Instance);
SList<ServerContextManager::ClosedContextEntry<ServerScriptContext>*, NoThrowHeapAllocator> ServerContextManager::ClosedScriptContextList(&NoThrowHeapAllocator::Instance);
#endif

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
            if (!scriptContext->IsClosed())
            {
                scriptContext->Close();
            }
            iter.RemoveCurrent();
        }
        iter.MoveNext();
    }
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

bool ServerContextManager::CheckLivenessAndAddref(ServerScriptContext* context)
{
    AutoCriticalSection autoCS(&cs);
    if (scriptContexts.LookupWithKey(context))
    {
        if (!context->IsClosed() && !context->GetThreadContext()->IsClosed())
        {
            context->AddRef();
            context->GetThreadContext()->AddRef();
            return true;
        }
    }
    return false;
}
bool ServerContextManager::CheckLivenessAndAddref(ServerThreadContext* context)
{
    AutoCriticalSection autoCS(&cs);
    if (threadContexts.LookupWithKey(context))
    {
        if (!context->IsClosed())
        {
            context->AddRef();
            return true;
        }
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
        AUTO_NESTED_HANDLED_EXCEPTION_TYPE(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
        AutoReleaseThreadContext autoThreadContext(threadContextInfo);
        hr = fn();
    }
    catch (ContextClosedException&)
    {
        hr = E_ACCESSDENIED;
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
    catch (...)
    {
        AssertOrFailFastMsg(false, "Unknown exception caught in JIT server call.");
    }

    if (hr == S_OK)
    {
        return MemoryOperationLastError::GetLastError();
    }
    return hr;
}

template<typename Fn>
HRESULT ServerCallWrapper(ServerScriptContext* scriptContextInfo, Fn fn)
{
    try
    {
        AutoReleaseScriptContext autoScriptContext(scriptContextInfo);
        ServerThreadContext* threadContextInfo = scriptContextInfo->GetThreadContext();
        return ServerCallWrapper(threadContextInfo, fn);
    }
    catch (ContextClosedException&)
    {
        return E_ACCESSDENIED;
    }
}
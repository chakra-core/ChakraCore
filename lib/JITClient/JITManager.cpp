//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JITClientPch.h"

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(size)
void * __RPC_USER midl_user_allocate(
#if defined(_WIN32_WINNT_WIN10)
    _In_ // starting win10, _In_ is in the signature
#endif
    size_t size)
{
    return (HeapAlloc(GetProcessHeap(), 0, size));
}

void __RPC_USER midl_user_free(_Pre_maybenull_ _Post_invalid_ void * ptr)
{
    if (ptr != NULL)
    {
        HeapFree(GetProcessHeap(), NULL, ptr);
    }
}

JITManager JITManager::s_jitManager = JITManager();

JITManager::JITManager() :
    m_rpcBindingHandle(nullptr),
    m_oopJitEnabled(false),
    m_isJITServer(false),
    m_failingHRESULT(S_OK),
    m_jitConnectionId()
{
}

JITManager::~JITManager()
{
    if (m_rpcBindingHandle)
    {
        RpcBindingFree(&m_rpcBindingHandle);
    }
}

/* static */
JITManager * 
JITManager::GetJITManager()
{
    return &s_jitManager;
}

// This routine creates a binding with the server.
HRESULT
JITManager::CreateBinding(
    __in HANDLE serverProcessHandle,
    __in_opt void * serverSecurityDescriptor,
    __in UUID * connectionUuid,
    __out RPC_BINDING_HANDLE * bindingHandle)
{
    Assert(IsOOPJITEnabled());

    RPC_STATUS status;
    DWORD attemptCount = 0;
    DWORD sleepInterval = 100; // in milliseconds
    RPC_BINDING_HANDLE localBindingHandle;
    RPC_BINDING_HANDLE_TEMPLATE_V1 bindingTemplate;
    RPC_BINDING_HANDLE_SECURITY_V1_W bindingSecurity;

#ifndef NTBUILD
    RPC_SECURITY_QOS_V4 securityQOS;
    ZeroMemory(&securityQOS, sizeof(RPC_SECURITY_QOS_V4));
    securityQOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    securityQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    securityQOS.ImpersonationType = RPC_C_IMP_LEVEL_IDENTIFY;
    securityQOS.Version = 4;
#else
    RPC_SECURITY_QOS_V5 securityQOS;
    ZeroMemory(&securityQOS, sizeof(RPC_SECURITY_QOS_V5));
    securityQOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    securityQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    securityQOS.ImpersonationType = RPC_C_IMP_LEVEL_IDENTIFY;
    securityQOS.Version = 5;
    securityQOS.ServerSecurityDescriptor = serverSecurityDescriptor;
#endif // NTBUILD

    ZeroMemory(&bindingTemplate, sizeof(bindingTemplate));
    bindingTemplate.Version = 1;
    bindingTemplate.ProtocolSequence = RPC_PROTSEQ_LRPC;
    bindingTemplate.StringEndpoint = NULL;
    memcpy_s(&bindingTemplate.ObjectUuid, sizeof(UUID), connectionUuid, sizeof(UUID));
    bindingTemplate.Flags |= RPC_BHT_OBJECT_UUID_VALID;

    ZeroMemory(&bindingSecurity, sizeof(bindingSecurity));
    bindingSecurity.Version = 1;
    bindingSecurity.AuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    bindingSecurity.AuthnSvc = RPC_C_AUTHN_KERNEL;
    bindingSecurity.SecurityQos = (RPC_SECURITY_QOS*)&securityQOS;

    status = RpcBindingCreate(&bindingTemplate, &bindingSecurity, NULL, &localBindingHandle);
    if (status != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(status);
    }

    // We keep attempting to connect to the server with increasing wait intervals in between.
    // This will wait close to 5 minutes before it finally gives up.
    do
    {
        DWORD waitStatus;

        status = RpcBindingBind(NULL, localBindingHandle, ClientIChakraJIT_v0_0_c_ifspec);
        if (status == RPC_S_OK)
        {
            break;
        }
        else if (status == EPT_S_NOT_REGISTERED)
        {
            // The Server side has not finished registering the RPC Server yet.
            // We should only breakout if we have reached the max attempt count.
            if (attemptCount > 600)
            {
                break;
            }
        }
        else
        {
            // Some unknown error occurred. We are not going to retry for arbitrary errors.
            break;
        }

        // When we come to this point, it means the server has not finished registration yet.
        // We should wait for a while and then reattempt to bind.
        waitStatus = WaitForSingleObject(serverProcessHandle, sleepInterval);
        if (waitStatus == WAIT_OBJECT_0)
        {
            // The server process died for some reason. No need to reattempt.
            status = RPC_S_SERVER_UNAVAILABLE;
            break;
        }
        else if (waitStatus == WAIT_TIMEOUT)
        {
            // Not an error. the server is still alive and we should reattempt.
        }
        else
        {
            // wait operation failed for an unknown reason.
            Assert(false);
            status = HRESULT_FROM_WIN32(waitStatus);
            break;
        }

        attemptCount++;
        if (sleepInterval < 500)
        {
            sleepInterval += 100;
        }
    } while (status != RPC_S_OK); // redundant check, but compiler would not allow true here.

    *bindingHandle = localBindingHandle;

    return HRESULT_FROM_WIN32(status);
}

bool
JITManager::IsJITServer() const
{
    return m_isJITServer;
}

void
JITManager::SetIsJITServer()
{
    m_isJITServer = true;
    m_oopJitEnabled = true;
}

bool
JITManager::IsConnected() const
{
    Assert(IsOOPJITEnabled());
    return m_rpcBindingHandle != nullptr && !HasJITFailed();
}

void
JITManager::EnableOOPJIT()
{
    m_oopJitEnabled = true;

    if (CONFIG_FLAG(OOPCFGRegistration))
    {
        // Since this client has enabled OOPJIT, perform the one-way policy update
        // that will disable SetProcessValidCallTargets from being invoked.
        GlobalSecurityPolicy::DisableSetProcessValidCallTargets();
    }
}

void
JITManager::SetJITFailed(HRESULT hr)
{
    Assert(hr != S_OK);
    m_failingHRESULT = hr;
}

bool
JITManager::HasJITFailed() const
{
    return m_failingHRESULT != S_OK;
}

bool
JITManager::IsOOPJITEnabled() const
{
    return m_oopJitEnabled;
}

HRESULT
JITManager::ConnectRpcServer(__in HANDLE jitProcessHandle, __in_opt void* serverSecurityDescriptor, __in UUID connectionUuid)
{
    Assert(IsOOPJITEnabled());
    Assert(m_rpcBindingHandle == nullptr);

    HRESULT hr = E_FAIL;

    if (IsConnected())
    {
        Assert(UNREACHED);
        return E_FAIL;
    }

    hr = CreateBinding(jitProcessHandle, serverSecurityDescriptor, &connectionUuid, &m_rpcBindingHandle);
    if (FAILED(hr))
    {
        goto FailureCleanup;
    }

    m_jitConnectionId = connectionUuid;

    return hr;

FailureCleanup:
    if (m_rpcBindingHandle)
    {
        RpcBindingFree(&m_rpcBindingHandle);
        m_rpcBindingHandle = nullptr;
    }

    return hr;
}

HRESULT
JITManager::Shutdown()
{
    // this is special case of shutdown called when runtime process is a parent of the server process
    // used for console host type scenarios
    HRESULT hr = S_OK;
    Assert(IsOOPJITEnabled());
    Assert(m_rpcBindingHandle != nullptr);

    RpcTryExcept
    {
        ClientShutdown(m_rpcBindingHandle);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    m_rpcBindingHandle = nullptr;

    return hr;
}

HRESULT
JITManager::InitializeThreadContext(
    __in ThreadContextDataIDL * data,
#ifdef USE_RPC_HANDLE_MARSHALLING
    __in HANDLE processHandle,
#endif
    __out PPTHREADCONTEXT_HANDLE threadContextInfoAddress,
    __out intptr_t * prereservedRegionAddr,
    __out intptr_t * jitThunkAddr)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientInitializeThreadContext(
            m_rpcBindingHandle,
            data,
#ifdef USE_RPC_HANDLE_MARSHALLING
            processHandle,
#endif
            threadContextInfoAddress,
            prereservedRegionAddr,
            jitThunkAddr);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::CleanupThreadContext(
    __inout PPTHREADCONTEXT_HANDLE threadContextInfoAddress)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientCleanupThreadContext(m_rpcBindingHandle, threadContextInfoAddress);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::AddDOMFastPathHelper(
    __in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    __in intptr_t funcInfoAddr,
    __in int helper)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientAddDOMFastPathHelper(m_rpcBindingHandle, scriptContextInfoAddress, funcInfoAddr, helper);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::SetIsPRNGSeeded(
    __in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    __in boolean value)
{
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientSetIsPRNGSeeded(m_rpcBindingHandle, scriptContextInfoAddress, value);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;

}

HRESULT
JITManager::DecommitInterpreterBufferManager(
    __in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    __in boolean asmJsThunk)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientDecommitInterpreterBufferManager(m_rpcBindingHandle, scriptContextInfoAddress, asmJsThunk);
    }
        RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::NewInterpreterThunkBlock(
    __in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    __in InterpreterThunkInputIDL * thunkInput,
    __out InterpreterThunkOutputIDL * thunkOutput)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientNewInterpreterThunkBlock(m_rpcBindingHandle, scriptContextInfoAddress, thunkInput, thunkOutput);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT 
JITManager::AddModuleRecordInfo(
    /* [in] */ PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    /* [in] */ unsigned int moduleId,
    /* [in] */ intptr_t localExportSlotsAddr)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientAddModuleRecordInfo(m_rpcBindingHandle, scriptContextInfoAddress, moduleId, localExportSlotsAddr);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}


HRESULT
JITManager::SetWellKnownHostTypeId(
    __in  PTHREADCONTEXT_HANDLE threadContextRoot,
    __in  int typeId)
{

    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientSetWellKnownHostTypeId(m_rpcBindingHandle, threadContextRoot, typeId);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;

}

HRESULT
JITManager::UpdatePropertyRecordMap(
    __in PTHREADCONTEXT_HANDLE threadContextInfoAddress,
    __in_opt BVSparseNodeIDL * updatedPropsBVHead)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientUpdatePropertyRecordMap(m_rpcBindingHandle, threadContextInfoAddress, updatedPropsBVHead);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::InitializeScriptContext(
    __in ScriptContextDataIDL * data,
    __in PTHREADCONTEXT_HANDLE threadContextInfoAddress,
    __out PPSCRIPTCONTEXT_HANDLE scriptContextInfoAddress)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientInitializeScriptContext(m_rpcBindingHandle, data, threadContextInfoAddress, scriptContextInfoAddress);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::CleanupScriptContext(
    __inout PPSCRIPTCONTEXT_HANDLE scriptContextInfoAddress)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientCleanupScriptContext(m_rpcBindingHandle, scriptContextInfoAddress);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::CloseScriptContext(
    __in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientCloseScriptContext(m_rpcBindingHandle, scriptContextInfoAddress);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::FreeAllocation(
    __in PTHREADCONTEXT_HANDLE threadContextInfoAddress,
    __in intptr_t codeAddress,
    __in intptr_t thunkAddress)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientFreeAllocation(m_rpcBindingHandle, threadContextInfoAddress, codeAddress, thunkAddress);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::IsNativeAddr(
    __in PTHREADCONTEXT_HANDLE threadContextInfoAddress,
    __in intptr_t address,
    __out boolean * result)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientIsNativeAddr(m_rpcBindingHandle, threadContextInfoAddress, address, result);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::RemoteCodeGenCall(
    __in CodeGenWorkItemIDL *workItemData,
    __in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    __out JITOutputIDL *jitData)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientRemoteCodeGen(m_rpcBindingHandle, scriptContextInfoAddress, workItemData, jitData);
    }
    RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

#if DBG
HRESULT
JITManager::IsInterpreterThunkAddr(
    __in PSCRIPTCONTEXT_HANDLE scriptContextInfoAddress,
    __in intptr_t address,
    __in boolean asmjsThunk,
    __out boolean * result)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientIsInterpreterThunkAddr(m_rpcBindingHandle, scriptContextInfoAddress, address, asmjsThunk, result);
    }
        RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}
#endif

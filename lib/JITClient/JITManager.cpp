//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JITClientPch.h"

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(size)
void * __RPC_USER midl_user_allocate(
#if defined(NTBUILD) || defined(_M_ARM)
    _In_ // starting win8, _In_ is in the signature
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
    m_targetHandle(nullptr),
    m_jitConnectionId()
{
}

JITManager::~JITManager()
{
    if(m_targetHandle)
    {
        CleanupProcess();
    }
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
            DWORD exitCode = (DWORD)-1;

            // The server process died for some reason. No need to reattempt.
            // We use -1 as the exit code if GetExitCodeProcess fails.
            Assert(GetExitCodeProcess(serverProcessHandle, &exitCode));
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

    if (status != RPC_S_OK)
    {
        RpcBindingFree(&localBindingHandle);
        return HRESULT_FROM_WIN32(status);
    }

    *bindingHandle = localBindingHandle;
    return S_OK;
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
    return m_rpcBindingHandle != nullptr && m_targetHandle != nullptr;
}

void
JITManager::EnableOOPJIT()
{
    m_oopJitEnabled = true;
}

bool
JITManager::IsOOPJITEnabled() const
{
    return m_oopJitEnabled;
}

HANDLE
JITManager::GetJITTargetHandle() const
{
    if (!IsOOPJITEnabled())
    {
        return GetCurrentProcess();
    }
    Assert(m_targetHandle != nullptr);
    return m_targetHandle;
}

HRESULT
JITManager::ConnectRpcServer(__in HANDLE jitProcessHandle, __in_opt void* serverSecurityDescriptor, __in UUID connectionUuid)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr;
    RPC_BINDING_HANDLE localBindingHandle;

    if (IsConnected() && (connectionUuid != m_jitConnectionId))
    {
        return E_FAIL;
    }

    hr = CreateBinding(jitProcessHandle, serverSecurityDescriptor, &connectionUuid, &localBindingHandle);

    HANDLE targetHandle;
    BOOL succeeded = DuplicateHandle(
        GetCurrentProcess(), GetCurrentProcess(),
        jitProcessHandle, &targetHandle,
        NULL, FALSE, DUPLICATE_SAME_ACCESS);

    if (!succeeded)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_targetHandle = targetHandle;
    m_rpcBindingHandle = localBindingHandle;
    m_jitConnectionId = connectionUuid;

    return hr;
}

HRESULT
JITManager::CleanupProcess()
{
    Assert(JITManager::IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientCleanupProcess(m_rpcBindingHandle, (intptr_t)m_targetHandle);
    }
        RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::Shutdown()
{
    HRESULT hr = S_OK;
    Assert(IsOOPJITEnabled());

    RpcTryExcept
    {
        ClientShutdown(m_rpcBindingHandle);
    }
        RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::InitializeThreadContext(
    __in ThreadContextDataIDL * data,
    __out intptr_t * threadContextInfoAddress,
    __out intptr_t * prereservedRegionAddr)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientInitializeThreadContext(m_rpcBindingHandle, data, threadContextInfoAddress, prereservedRegionAddr);
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
    __in intptr_t threadContextInfoAddress)
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
    __in intptr_t scriptContextInfoAddress,
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
    __in intptr_t scriptContextInfoAddress,
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
JITManager::AddModuleRecordInfo(
    /* [in] */ intptr_t scriptContextInfoAddress,
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
    __in  intptr_t threadContextRoot,
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
    __in intptr_t threadContextInfoAddress,
    __in UpdatedPropertysIDL * updatedProps)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientUpdatePropertyRecordMap(m_rpcBindingHandle, threadContextInfoAddress, updatedProps);
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
    __out intptr_t * scriptContextInfoAddress)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientInitializeScriptContext(m_rpcBindingHandle, data, scriptContextInfoAddress);
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
    __in intptr_t scriptContextInfoAddress)
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
    __in intptr_t scriptContextInfoAddress)
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
    __in intptr_t threadContextInfoAddress,
    __in intptr_t address)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientFreeAllocation(m_rpcBindingHandle, threadContextInfoAddress, address);
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
    __in intptr_t threadContextInfoAddress,
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
    __in intptr_t threadContextInfoAddress,
    __in intptr_t scriptContextInfoAddress,
    __out JITOutputIDL *jitData)
{
    Assert(IsOOPJITEnabled());

    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientRemoteCodeGen(m_rpcBindingHandle, threadContextInfoAddress, scriptContextInfoAddress, workItemData, jitData);
    }
        RpcExcept(RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

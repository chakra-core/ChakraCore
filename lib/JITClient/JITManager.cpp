//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JITClientPch.h"

__bcount_opt(size) void * __RPC_USER midl_user_allocate(size_t size)
{
    return (HeapAlloc(GetProcessHeap(), 0, size));
}

void __RPC_USER midl_user_free(__inout void * ptr)
{
    if (ptr != NULL)
    {
        HeapFree(GetProcessHeap(), NULL, ptr);
    }
}

JITManager JITManager::s_jitManager = JITManager();

JITManager::JITManager() :
    m_rpcBindingHandle(nullptr),
    m_rpcServerProcessHandle(nullptr),
    m_jitProcessId(0),
    m_jitConnectionId()
{
}

JITManager::~JITManager()
{
    // TODO: OOP JIT, what to do in case we fail to disconnect?
    DisconnectRpcServer();
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
    __in UUID * connectionUuid,
    __out RPC_BINDING_HANDLE * bindingHandle)
{
    RPC_STATUS status;
    RPC_SECURITY_QOS_V4 securityQOS; // TODO: V5???
    DWORD attemptCount = 0;
    DWORD sleepInterval = 100; // in milliseconds
    RPC_BINDING_HANDLE localBindingHandle;
    RPC_BINDING_HANDLE_TEMPLATE_V1 bindingTemplate;
    RPC_BINDING_HANDLE_SECURITY_V1_W bindingSecurity;

    ZeroMemory(&securityQOS, sizeof(RPC_SECURITY_QOS_V4));
    securityQOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    securityQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    securityQOS.ImpersonationType = RPC_C_IMP_LEVEL_IDENTIFY;
    securityQOS.Version = 4;

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
            NT_VERIFY(GetExitCodeProcess(serverProcessHandle, &exitCode));
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
            NT_ASSERT(false);
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
JITManager::IsConnected() const
{
    return m_rpcBindingHandle != nullptr && m_rpcServerProcessHandle != nullptr && m_targetHandle != nullptr;
}

HANDLE
JITManager::GetJITTargetHandle() const
{
    Assert(m_targetHandle != nullptr);
    return m_targetHandle;
}

HRESULT
JITManager::ConnectRpcServer(DWORD proccessId, UUID connectionUuid)
{
    HRESULT hr;
    HANDLE localServerProcessHandle = nullptr;
    WCHAR* connectionUuidString = nullptr;
    RPC_BINDING_HANDLE localBindingHandle;

    if (IsConnected() && (proccessId != m_jitProcessId || connectionUuid != m_jitConnectionId))
    {
        return E_FAIL;
    }

    hr = HRESULT_FROM_WIN32(UuidToStringW(&connectionUuid, &connectionUuidString));
    if (FAILED(hr))
    {
        return hr;
    }

    localServerProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proccessId);

    hr = CreateBinding(localServerProcessHandle, &connectionUuid, &localBindingHandle);
    if (FAILED(hr))
    {
        CloseHandle(localServerProcessHandle);
        return hr;
    }

    HANDLE targetHandle;
    HANDLE jitProcHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, proccessId);
    BOOL succeeded = DuplicateHandle(
        GetCurrentProcess(), GetCurrentProcess(),
        jitProcHandle, &targetHandle,
        NULL, FALSE, DUPLICATE_SAME_ACCESS);

    if (!succeeded)
    {
        CloseHandle(localServerProcessHandle);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!CloseHandle(jitProcHandle))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    m_targetHandle = targetHandle;
    m_rpcBindingHandle = localBindingHandle;
    m_rpcServerProcessHandle = localServerProcessHandle;
    m_jitProcessId = proccessId;
    m_jitConnectionId = connectionUuid;

    return hr;
}

HRESULT
JITManager::DisconnectRpcServer()
{
    HRESULT hr = S_OK;

    if (m_rpcBindingHandle == nullptr)
    {
        Assert(m_rpcBindingHandle == nullptr);
        Assert(m_rpcServerProcessHandle == nullptr);
        Assert(m_targetHandle == nullptr);
        return hr;
    }

    CleanupProcess((intptr_t)m_targetHandle);

    RpcTryExcept
    {
        ClientShutdown(m_rpcBindingHandle);
    }
        RpcExcept(1)
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    if (FAILED(hr))
    {
        return hr;
    }

    hr = HRESULT_FROM_WIN32(RpcBindingFree(&m_rpcBindingHandle));
    if (FAILED(hr))
    {
        return hr;
    }

    if (!CloseHandle(m_rpcServerProcessHandle))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!CloseHandle(m_targetHandle))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_targetHandle = nullptr;
    m_rpcBindingHandle = nullptr;
    m_rpcServerProcessHandle = nullptr;
    m_jitProcessId = 0;
    m_jitConnectionId = {0};

    return hr;
}

HRESULT
JITManager::InitializeThreadContext(
    __in ThreadContextDataIDL * data,
    __out intptr_t * threadContextInfoAddress)
{
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientInitializeThreadContext(m_rpcBindingHandle, data, threadContextInfoAddress);
    }
        RpcExcept(1)
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::CleanupProcess(
    __in intptr_t processHandle)
{
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientCleanupProcess(m_rpcBindingHandle, processHandle);
    }
        RpcExcept(1)
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
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientCleanupThreadContext(m_rpcBindingHandle, threadContextInfoAddress);
    }
        RpcExcept(1)
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
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientAddDOMFastPathHelper(m_rpcBindingHandle, scriptContextInfoAddress, funcInfoAddr, helper);
    }
        RpcExcept(1)
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

HRESULT
JITManager::AddPropertyRecord(
    __in intptr_t threadContextInfoAddress,
    __in PropertyRecordIDL * propertyRecord)
{
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientAddPropertyRecord(m_rpcBindingHandle, threadContextInfoAddress, propertyRecord);
    }
        RpcExcept(1)
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
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientInitializeScriptContext(m_rpcBindingHandle, data, scriptContextInfoAddress);
    }
        RpcExcept(1)
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
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientCleanupScriptContext(m_rpcBindingHandle, scriptContextInfoAddress);
    }
        RpcExcept(1)
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
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientFreeAllocation(m_rpcBindingHandle, threadContextInfoAddress, address);
    }
        RpcExcept(1)
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
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientIsNativeAddr(m_rpcBindingHandle, threadContextInfoAddress, address, result);
    }
        RpcExcept(1)
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
    HRESULT hr = E_FAIL;
    RpcTryExcept
    {
        hr = ClientRemoteCodeGen(m_rpcBindingHandle, threadContextInfoAddress, scriptContextInfoAddress, workItemData, jitData);
    }
        RpcExcept(1)
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return hr;
}

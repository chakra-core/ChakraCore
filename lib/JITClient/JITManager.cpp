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

HRESULT
JITManager::ConnectRpcServer(DWORD proccessId, UUID connectionUuid)
{
    HRESULT hr;
    RPC_STATUS status;
    HANDLE localServerProcessHandle = NULL;
    WCHAR* connectionUuidString = NULL;
    RPC_BINDING_HANDLE localBindingHandle;

    status = UuidToStringW(&connectionUuid, &connectionUuidString);
    if (status != S_OK)
    {
        return HRESULT_FROM_WIN32(status);
    }

    localServerProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proccessId);

    hr = CreateBinding(localServerProcessHandle, &connectionUuid, &localBindingHandle);
    if (SUCCEEDED(hr))
    {
        m_rpcBindingHandle = localBindingHandle;
        m_rpcServerProcessHandle = localServerProcessHandle;
    }
    else
    {
        CloseHandle(localServerProcessHandle);
    }


    // log an event

    return hr;
}

void
JITManager::DisconnectRpcServer()
{
    HRESULT hr = S_OK;

    if (m_rpcBindingHandle == NULL)
    {
        NT_ASSERT(m_rpcBindingHandle == NULL);
        NT_ASSERT(m_rpcServerProcessHandle == NULL);
        return;
    }

    RpcTryExcept
    {
        ClientShutdown(m_rpcBindingHandle);
    }
        RpcExcept(1)
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    RpcBindingFree(&m_rpcBindingHandle);
    m_rpcBindingHandle = NULL;

    // log an event

    CloseHandle(m_rpcServerProcessHandle);
    m_rpcServerProcessHandle = NULL;

}

HRESULT
JITManager::InitializeThreadContext(
    __in ThreadContextData * data,
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
JITManager::InitializeScriptContext(
    __in ScriptContextData * data,
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
JITManager::RemoteCodeGenCall(
    __in CodeGenWorkItemJITData *workItemData,
    __in intptr_t threadContextInfoAddress,
    __in intptr_t scriptContextInfoAddress,
    __out JITOutputData *jitData)
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

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITManager
{
public:
    HRESULT ConnectRpcServer(__in DWORD processId, __in UUID connectionUuid);

    void DisconnectRpcServer();

    HRESULT InitializeThreadContext(
        __in ThreadContextDataIDL * data,
        __out intptr_t *threadContextInfoAddress);

    HRESULT CleanupThreadContext(
        __in intptr_t threadContextInfoAddress);

    HRESULT AddPropertyRecord(
        __in intptr_t threadContextInfoAddress,
        __in PropertyRecordIDL * propertyRecord);

    HRESULT InitializeScriptContext(
        __in ScriptContextDataIDL * data,
        __out intptr_t *scriptContextInfoAddress);

    HRESULT CleanupScriptContext(
        __in intptr_t scriptContextInfoAddress);

    HRESULT FreeAllocation(
        __in intptr_t threadContextInfoAddress,
        __in intptr_t address);

    HRESULT IsNativeAddr(
        __in intptr_t threadContextInfoAddress,
        __in intptr_t address,
        __out boolean * result);

    HRESULT RemoteCodeGenCall(
        __in CodeGenWorkItemIDL *workItemData,
        __in intptr_t threadContextInfoAddress,
        __in intptr_t scriptContextInfoAddress,
        __out JITOutputIDL *jitData);

private:

    HRESULT JITManager::CreateBinding(
        __in HANDLE serverProcessHandle,
        __in UUID* connectionUuid,
        __out RPC_BINDING_HANDLE* bindingHandle);

    RPC_BINDING_HANDLE m_rpcBindingHandle;
    HANDLE m_rpcServerProcessHandle;
};

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITManager
{
public:
    HRESULT ConnectRpcServer(__in HANDLE jitProcessHandle, __in_opt void* serverSecurityDescriptor, __in UUID connectionUuid);

    bool IsConnected() const;
    bool IsJITServer() const;
    void SetIsJITServer();
    bool IsOOPJITEnabled() const;
    void EnableOOPJIT();

    HANDLE GetJITTargetHandle() const;

    HRESULT InitializeThreadContext(
        __in ThreadContextDataIDL * data,
        __out intptr_t *threadContextInfoAddress,
        __out intptr_t *prereservedRegionAddr);

    HRESULT CleanupThreadContext(
        __in intptr_t threadContextInfoAddress);

    HRESULT UpdatePropertyRecordMap(
        __in intptr_t threadContextInfoAddress,
        __in UpdatedPropertysIDL * updatedProps);

    HRESULT AddDOMFastPathHelper(
        __in intptr_t scriptContextInfoAddress,
        __in intptr_t funcInfoAddr,
        __in int helper);

    HRESULT AddModuleRecordInfo(
            /* [in] */ intptr_t scriptContextInfoAddress,
            /* [in] */ unsigned int moduleId,
            /* [in] */ intptr_t localExportSlotsAddr);

    HRESULT SetWellKnownHostTypeId(
        __in  intptr_t threadContextRoot,
        __in  int typeId);

    HRESULT InitializeScriptContext(
        __in ScriptContextDataIDL * data,
        __out intptr_t *scriptContextInfoAddress);

    HRESULT CleanupProcess();

    HRESULT CleanupScriptContext(
        __in intptr_t scriptContextInfoAddress);

    HRESULT CloseScriptContext(
        __in intptr_t scriptContextInfoAddress);

    HRESULT FreeAllocation(
        __in intptr_t threadContextInfoAddress,
        __in intptr_t address);

    HRESULT SetIsPRNGSeeded(
        __in intptr_t scriptContextInfoAddress,
        __in boolean value);

    HRESULT IsNativeAddr(
        __in intptr_t threadContextInfoAddress,
        __in intptr_t address,
        __out boolean * result);

    HRESULT RemoteCodeGenCall(
        __in CodeGenWorkItemIDL *workItemData,
        __in intptr_t threadContextInfoAddress,
        __in intptr_t scriptContextInfoAddress,
        __out JITOutputIDL *jitData);

    HRESULT Shutdown();

    static JITManager * GetJITManager();
private:
    JITManager();
    ~JITManager();

    HRESULT CreateBinding(
        __in HANDLE serverProcessHandle,
        __in_opt void* serverSecurityDescriptor,
        __in UUID* connectionUuid,
        __out RPC_BINDING_HANDLE* bindingHandle);

    RPC_BINDING_HANDLE m_rpcBindingHandle;
    HANDLE m_targetHandle;
    UUID m_jitConnectionId;
    bool m_oopJitEnabled;
    bool m_isJITServer;

    static JITManager s_jitManager;
};

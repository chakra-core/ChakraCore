//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITProcessManager
{
public:
    static HRESULT StartRpcServer(int argc, __in_ecount(argc) LPWSTR argv[]);
    static void TerminateJITServer();

    static HANDLE GetRpcProccessHandle();
    static UUID GetRpcConnectionId();
    static void RemoveArg(LPCWSTR flag, int * argc, __in_ecount(*argc) LPWSTR * argv[]);

private:
    static HRESULT CreateServerProcess(int argc, __in_ecount(argc) LPWSTR argv[]);

    static HANDLE s_rpcServerProcessHandle;
    static UUID s_connectionId;
};

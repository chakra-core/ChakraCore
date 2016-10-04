
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

class ServerContextManager
{
public:
    static void RegisterThreadContext(ServerThreadContext* threadContext);
    static void UnRegisterThreadContext(ServerThreadContext* threadContext);
    static bool IsThreadContextAlive(ServerThreadContext* threadContext);

    static void RegisterScriptContext(ServerScriptContext* scriptContext);
    static void UnRegisterScriptContext(ServerScriptContext* scriptContext);    
    static bool IsScriptContextAlive(ServerScriptContext* scriptContext);
private:
    static JsUtil::BaseHashSet<ServerThreadContext*, HeapAllocator> threadContexts;
    static JsUtil::BaseHashSet<ServerScriptContext*, HeapAllocator> scriptContexts;
    static CriticalSection cs;
};

template<class T>
struct AutoReleaseContext
{
    AutoReleaseContext(T* context)
        :context(context)
    {
        context->AddRef();
    }

    ~AutoReleaseContext()
    {
        context->Release();
    }

    T* context;
};

template<typename Fn>
HRESULT ServerCallWrapper(ServerThreadContext* threadContextInfo, Fn fn);
template<typename Fn>
HRESULT ServerCallWrapper(ServerScriptContext* scriptContextInfo, Fn fn);

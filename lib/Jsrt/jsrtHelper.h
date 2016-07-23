//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class JsrtCallbackState
{
public:
    JsrtCallbackState(ThreadContext* currentThreadContext);
    ~JsrtCallbackState();
    static void ObjectBeforeCallectCallbackWrapper(JsObjectBeforeCollectCallback callback, void* object, void* callbackState, void* threadContext);
private:
    ThreadContext* originalThreadContext;
    JsrtContext* originalJsrtContext;
};

#if defined(CHAKRA_STATIC_LIBRARY) || !defined(_WIN32)
_NOINLINE void VALIDATE_ENTER_CURRENT_THREAD();
#else
// Windows  Shared Library: DllMain is responsible from handling all these stuff
#define VALIDATE_ENTER_CURRENT_THREAD()
#endif

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "stdafx.h"
#include "catch.hpp"

#pragma warning(disable:6387) // suppressing preFAST which raises warning for passing null to the JsRT APIs

namespace ThreadServiceTests
{
    struct ThreadPoolCallbackContext
    {
        JsBackgroundWorkItemCallback callback;
        void * callbackData;
    };

    static bool sawCallback = false;

    void CALLBACK ThreadPoolCallback(PTP_CALLBACK_INSTANCE /* Instance */, void * Context)
    {
        ThreadPoolCallbackContext * c = (ThreadPoolCallbackContext *)Context;
        c->callback(c->callbackData);

        delete c;
    }

    bool CALLBACK SubmitBackgroundWorkToThreadPool(JsBackgroundWorkItemCallback callback, void * callbackData)
    {
        REQUIRE(callback != nullptr);
        REQUIRE(callbackData != nullptr);

        sawCallback = true;

        ThreadPoolCallbackContext * c = new ThreadPoolCallbackContext();
        c->callback = callback;
        c->callbackData = callbackData;

        BOOL success = TrySubmitThreadpoolCallback(ThreadPoolCallback, c, nullptr);
        REQUIRE(!!success);

        return true;
    }

    bool CALLBACK FailBackgroundWorkRequest(JsBackgroundWorkItemCallback callback, void * callbackData)
    {
        REQUIRE(callback != nullptr);
        REQUIRE(callbackData != nullptr);

        sawCallback = true;

        // Always fail the request and force work in-thread
        return false;
    }

    void Test(JsThreadServiceCallback threadService)
    {
        sawCallback = false;

        JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
        JsContextRef context = JS_INVALID_REFERENCE;
        REQUIRE(JsCreateRuntime(JsRuntimeAttributeAllowScriptInterrupt, threadService, &runtime) == JsNoError);
        REQUIRE(JsCreateContext(runtime, &context) == JsNoError);

        REQUIRE(JsSetCurrentContext(context) == JsNoError);

        LPCWSTR script = nullptr;
        REQUIRE(FileLoadHelpers::LoadScriptFromFile("Splay.js", script) == S_OK);
        REQUIRE(script != nullptr);

        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), nullptr) == JsNoError);

        REQUIRE(JsSetCurrentContext(JS_INVALID_REFERENCE) == JsNoError);
        REQUIRE(JsDisposeRuntime(runtime) == JsNoError);

        // Ensure that at least one callback happened
        CHECK(sawCallback);
    }

    TEST_CASE("ThreadServiceTest_ThreadPoolTest", "[ThreadServiceTest]")
    {
        Test(SubmitBackgroundWorkToThreadPool);
    }

    TEST_CASE("ThreadServiceTest_AlwaysDenyRequestTest", "[ThreadServiceTest]")
    {
        Test(FailBackgroundWorkRequest);
    }
}

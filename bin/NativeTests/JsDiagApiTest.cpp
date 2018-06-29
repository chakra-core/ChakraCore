//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "stdafx.h"
#pragma warning(disable:26434) // Function definition hides non-virtual function in base class
#pragma warning(disable:26439) // Implicit noexcept
#pragma warning(disable:26451) // Arithmetic overflow
#pragma warning(disable:26495) // Uninitialized member variable
#include "catch.hpp"
#include <process.h>

#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:6387) // suppressing preFAST which raises warning for passing null to the JsRT APIs
#pragma warning(disable:6262) // CATCH is using stack variables to report errors, suppressing the preFAST warning.

namespace JsDiagApiTest
{
    static void CALLBACK DebugEventCallback(JsDiagDebugEvent debugEvent, JsValueRef eventData, void* callbackState)
    {
    }

    bool TestSetup(JsRuntimeHandle *runtime)
    {
        JsValueRef context = JS_INVALID_REFERENCE;
        JsValueRef setContext = JS_INVALID_REFERENCE;

        // Create runtime, context and set current context
        REQUIRE(JsCreateRuntime(JsRuntimeAttributeNone, nullptr, runtime) == JsNoError);
        REQUIRE(JsCreateContext(*runtime, &context) == JsNoError);
        REQUIRE(JsSetCurrentContext(context) == JsNoError);
        REQUIRE(((JsGetCurrentContext(&setContext) == JsNoError) || setContext == context));

        // Enable debugging
        REQUIRE(JsDiagStartDebugging(*runtime, JsDiagApiTest::DebugEventCallback, nullptr) == JsNoError);

        return true;
    }

    bool TestCleanup(JsRuntimeHandle runtime)
    {
        if (runtime != nullptr)
        {
            // Disable debugging
            JsDiagStopDebugging(runtime, nullptr);

            JsSetCurrentContext(nullptr);
            JsDisposeRuntime(runtime);
        }
        return true;
    }

    template <class Handler>
    void WithSetup(Handler handler)
    {
        JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
        if (!TestSetup(&runtime))
        {
            REQUIRE(false);
            return;
        }

        handler(runtime);

        TestCleanup(runtime);
    }

#ifndef BUILD_WITHOUT_SCRIPT_DEBUG
    void BreakpointsContextTest(JsRuntimeHandle runtime)
    {
        JsContextRef context = JS_INVALID_REFERENCE;
        REQUIRE(JsGetCurrentContext(&context) == JsNoError);

        // Verify no errors with a context set
        JsValueRef scriptsArray = JS_INVALID_REFERENCE;
        REQUIRE(JsDiagGetScripts(&scriptsArray) == JsNoError);

        // Verify the APIs return an error when no current context set
        JsSetCurrentContext(nullptr);
        CHECK(JsDiagGetScripts(&scriptsArray) == JsErrorNoCurrentContext);

        JsValueRef breakpoint = JS_INVALID_REFERENCE;
        CHECK(JsDiagSetBreakpoint(0, 0, 0, &breakpoint) == JsErrorNoCurrentContext);

        CHECK(JsDiagRemoveBreakpoint(0) == JsErrorNoCurrentContext);
    }

    TEST_CASE("JsDiagApiTest_BreakpointsContextTest", "[JsDiagApiTest]")
    {
        JsDiagApiTest::WithSetup(JsDiagApiTest::BreakpointsContextTest);
    }
#endif // BUILD_WITHOUT_SCRIPT_DEBUG
}

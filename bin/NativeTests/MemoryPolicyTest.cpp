//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "catch.hpp"

#pragma warning(disable:6387) // suppressing preFAST which raises warning for passing null to the JsRT APIs

namespace MemoryPolicyTests
{
    static const size_t MemoryLimit = 10 * 1024 * 1024;

    void ValidateOOMException()
    {
        JsValueRef exception = JS_INVALID_REFERENCE;
        JsErrorCode errorCode = JsGetAndClearException(&exception);
        if (errorCode == JsNoError)
        {
            JsPropertyIdRef property = JS_INVALID_REFERENCE;
            JsValueRef value = JS_INVALID_REFERENCE;
            LPCWSTR str = nullptr;
            size_t length;

            REQUIRE(JsGetPropertyIdFromName(_u("message"), &property) == JsNoError);
            REQUIRE(JsGetProperty(exception, property, &value) == JsNoError);
            REQUIRE(JsStringToPointer(value, &str, &length) == JsNoError);
            CHECK(wcscmp(str, _u("Out of memory")) == 0);
        }
        else
        {
            // If we don't have enough memory to create error object, then GetAndClearException might 
            // fail and return ErrorInvalidArgument. Check if we don't get any other error code.
            CHECK(errorCode == JsErrorInvalidArgument);
        }
    }

    void BasicTest(JsRuntimeAttributes attributes, LPCSTR fileName)
    {
        LPCWSTR script = nullptr;
        REQUIRE(FileLoadHelpers::LoadScriptFromFile(fileName, script) == S_OK);
        REQUIRE(script != nullptr);

        // Create the runtime
        JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
        REQUIRE(JsCreateRuntime(attributes, nullptr, &runtime) == JsNoError);

        // Set memory limit
        REQUIRE(JsSetRuntimeMemoryLimit(runtime, MemoryLimit) == JsNoError);

        size_t memoryLimit;
        size_t memoryUsage;

        REQUIRE(JsGetRuntimeMemoryLimit(runtime, &memoryLimit) == JsNoError);
        CHECK(memoryLimit == MemoryLimit);
        REQUIRE(JsGetRuntimeMemoryUsage(runtime, &memoryUsage) == JsNoError);
        CHECK(memoryUsage < MemoryLimit);

        // Create and initialize the script context
        JsContextRef context = JS_INVALID_RUNTIME_HANDLE;
        REQUIRE(JsCreateContext(runtime, &context) == JsNoError);
        REQUIRE(JsSetCurrentContext(context) == JsNoError);

        // Invoke the script
        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), nullptr) == JsErrorScriptException);

        ValidateOOMException();

        REQUIRE(JsGetRuntimeMemoryLimit(runtime, &memoryLimit) == JsNoError);
        CHECK(memoryLimit == MemoryLimit);
        REQUIRE(JsGetRuntimeMemoryUsage(runtime, &memoryUsage) == JsNoError);
        CHECK(memoryUsage <= MemoryLimit);

        // Destroy the runtime
        REQUIRE(JsSetCurrentContext(JS_INVALID_REFERENCE) == JsNoError);
        REQUIRE(JsCollectGarbage(runtime) == JsNoError);
        REQUIRE(JsDisposeRuntime(runtime) == JsNoError);
    }

    TEST_CASE("MemoryPolicyTest_UnboundedMemory", "[MemoryPolicyTest]")
    {
        BasicTest(JsRuntimeAttributeNone, "UnboundedMemory.js");
        BasicTest(JsRuntimeAttributeDisableBackgroundWork, "UnboundedMemory.js");
    }

    TEST_CASE("MemoryPolicyTest_ArrayTest", "[MemoryPolicyTest]")
    {
        BasicTest(JsRuntimeAttributeNone, "arrayTest.js");
        BasicTest(JsRuntimeAttributeDisableBackgroundWork, "arrayTest.js");
    }

    TEST_CASE("MemoryPolicyTest_ArrayBuffer", "[MemoryPolicyTest]")
    {
        BasicTest(JsRuntimeAttributeNone, "arraybuffer.js");
        BasicTest(JsRuntimeAttributeDisableBackgroundWork, "arraybuffer.js");
    }

    void OOSTest(JsRuntimeAttributes attributes)
    {
        JsPropertyIdRef property;
        JsValueRef value, exception;
        LPCWSTR str;
        size_t length;

        LPCWSTR script = nullptr;
        REQUIRE(FileLoadHelpers::LoadScriptFromFile("oos.js", script) == S_OK);
        REQUIRE(script != nullptr);

        // Create the runtime
        JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
        REQUIRE(JsCreateRuntime(attributes, nullptr, &runtime) == JsNoError);

        // Create and initialize the script context
        JsContextRef context;
        REQUIRE(JsCreateContext(runtime, &context) == JsNoError);
        REQUIRE(JsSetCurrentContext(context) == JsNoError);

        // Invoke the script
        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, L"", nullptr) == JsErrorScriptException);

        // Verify we got OOS
        REQUIRE(JsGetAndClearException(&exception) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(L"message", &property) == JsNoError);
        REQUIRE(JsGetProperty(exception, property, &value) == JsNoError);
        REQUIRE(JsStringToPointer(value, &str, &length) == JsNoError);
        CHECK(wcscmp(str, _u("Out of stack space")) == 0);

        // Destroy the runtime
        REQUIRE(JsSetCurrentContext(JS_INVALID_REFERENCE) == JsNoError);
        REQUIRE(JsDisposeRuntime(runtime) == JsNoError);
    }

    TEST_CASE("MemoryPolicyTest_OOSTest", "[OOSTest]")
    {
        OOSTest(JsRuntimeAttributeNone);
        OOSTest(JsRuntimeAttributeDisableBackgroundWork);
    }

    class MemoryPolicyTest
    {
    public:
        MemoryPolicyTest() : totalAllocationSize(0) { }
        size_t totalAllocationSize;
    };

    static bool CALLBACK MemoryAllocationCallback(LPVOID context, JsMemoryEventType allocationEvent, size_t allocationSize)
    {
        REQUIRE(context != nullptr);
        MemoryPolicyTest* memoryPolicyTest = static_cast<MemoryPolicyTest*>(context);
        switch (allocationEvent)
        {
        case JsMemoryAllocate:
            memoryPolicyTest->totalAllocationSize += allocationSize;
            if (memoryPolicyTest->totalAllocationSize > MemoryLimit)
            {
                return false;
            }
            return true;
            break;
        case JsMemoryFree:
            memoryPolicyTest->totalAllocationSize -= allocationSize;
            break;
        case JsMemoryFailure:
            memoryPolicyTest->totalAllocationSize -= allocationSize;

            break;
        default:
            CHECK(false);
        }
        return true;
    }

    void NegativeTest(JsRuntimeAttributes attributes)
    {
        MemoryPolicyTest memoryPolicyTest;

        // Create the runtime
        JsRuntimeHandle runtime;
        REQUIRE(JsCreateRuntime(attributes, nullptr, &runtime) == JsNoError);

        // after setting callback, you can set limit
        REQUIRE(JsSetRuntimeMemoryAllocationCallback(runtime, &memoryPolicyTest, MemoryAllocationCallback) == JsNoError);
        REQUIRE(JsSetRuntimeMemoryLimit(runtime, MemoryLimit) == JsNoError);

        REQUIRE(JsSetRuntimeMemoryAllocationCallback(runtime, nullptr, nullptr) == JsNoError);
        // now we can set limit
        REQUIRE(JsSetRuntimeMemoryLimit(runtime, MemoryLimit) == JsNoError);
        // and we can set the callback again. 
        REQUIRE(JsSetRuntimeMemoryAllocationCallback(runtime, &memoryPolicyTest, MemoryAllocationCallback) == JsNoError);

        REQUIRE(JsSetRuntimeMemoryLimit(runtime, (size_t)-1) == JsNoError);
        REQUIRE(JsSetRuntimeMemoryAllocationCallback(runtime, &memoryPolicyTest, MemoryAllocationCallback) == JsNoError);

        // Destroy the runtime
        REQUIRE(JsSetCurrentContext(JS_INVALID_REFERENCE) == JsNoError);
        REQUIRE(JsDisposeRuntime(runtime) == JsNoError);
    }

    TEST_CASE("MemoryPolicyTest_NegativeTest", "[NegativeTest]")
    {
        NegativeTest(JsRuntimeAttributeNone);
        NegativeTest(JsRuntimeAttributeDisableBackgroundWork);
    }

    void ContextLeak()
    {
        MemoryPolicyTest memoryPolicyTest;
        JsRuntimeHandle jsRuntime = JS_INVALID_RUNTIME_HANDLE;
        JsErrorCode errorCode = ::JsCreateRuntime(
            (JsRuntimeAttributes)(JsRuntimeAttributeAllowScriptInterrupt | JsRuntimeAttributeDisableEval | JsRuntimeAttributeDisableNativeCodeGeneration),
            nullptr,
            &jsRuntime);
        REQUIRE(errorCode == JsNoError);
        REQUIRE(JsSetRuntimeMemoryAllocationCallback(jsRuntime, &memoryPolicyTest, MemoryAllocationCallback) == JsNoError);
        for (ULONG i = 0; i < 1000; ++i)
        {
            JsContextRef jsContext = JS_INVALID_REFERENCE;
            errorCode = ::JsCreateContext(jsRuntime, &jsContext);
            REQUIRE(errorCode == JsNoError);
            errorCode = ::JsSetCurrentContext(jsContext);
            REQUIRE(errorCode == JsNoError);
            errorCode = ::JsSetCurrentContext(JS_INVALID_REFERENCE);
            REQUIRE(errorCode == JsNoError);
            if (((ULONG)(i % 100)) * 100 == 0)
            {
                JsCollectGarbage(jsRuntime);
            }
        }
        REQUIRE(JsDisposeRuntime(jsRuntime) == JsNoError);
    }

    TEST_CASE("MemoryPolicyTest_ContextLeak", "[ContextLeak]")
    {
        ContextLeak();
    }

    void ContextLeak1()
    {
        MemoryPolicyTest memoryPolicyTest;
        JsRuntimeHandle jsRuntime = JS_INVALID_RUNTIME_HANDLE;
        JsErrorCode errorCode = ::JsCreateRuntime(
            (JsRuntimeAttributes)(JsRuntimeAttributeAllowScriptInterrupt | JsRuntimeAttributeDisableEval | JsRuntimeAttributeDisableNativeCodeGeneration),
            nullptr,
            &jsRuntime);
        REQUIRE(errorCode == JsNoError);
        REQUIRE(JsSetRuntimeMemoryAllocationCallback(jsRuntime, &memoryPolicyTest, MemoryAllocationCallback) == JsNoError);
        for (ULONG i = 0; i < 1000; ++i)
        {
            JsContextRef jsContext = JS_INVALID_REFERENCE;
            JsContextRef jsContext1 = JS_INVALID_REFERENCE;
            JsValueRef jsVal1 = JS_INVALID_REFERENCE, jsGO2 = JS_INVALID_REFERENCE;
            JsPropertyIdRef propertyId;
            REQUIRE(::JsCreateContext(jsRuntime, &jsContext) == JsNoError);
            REQUIRE(::JsSetCurrentContext(jsContext) == JsNoError);
            REQUIRE(::JsCreateObject(&jsVal1) == JsNoError);
            REQUIRE(::JsGetPropertyIdFromName(L"test", &propertyId) == JsNoError);
            REQUIRE(::JsCreateContext(jsRuntime, &jsContext1) == JsNoError);
            REQUIRE(::JsSetCurrentContext(jsContext1) == JsNoError);
            REQUIRE(::JsGetGlobalObject(&jsGO2) == JsNoError);
            REQUIRE(::JsSetProperty(jsGO2, propertyId, jsVal1, true) == JsNoError);
            REQUIRE(JsSetCurrentContext(JS_INVALID_REFERENCE) == JsNoError);
            if (((ULONG)(i % 100)) * 100 == 0)
            {
                jsVal1 = JS_INVALID_REFERENCE;
                jsGO2 = JS_INVALID_REFERENCE;
                jsContext = JS_INVALID_REFERENCE;
                jsContext1 = JS_INVALID_REFERENCE;
                JsCollectGarbage(jsRuntime);
            }
        }
        REQUIRE(JsDisposeRuntime(jsRuntime) == JsNoError);
    }

    TEST_CASE("MemoryPolicyTest_ContextLeak1", "[ContextLeak1]")
    {
        ContextLeak1();
    }
}

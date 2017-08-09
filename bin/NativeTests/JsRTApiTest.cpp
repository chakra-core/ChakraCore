//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "catch.hpp"
#include <process.h>

#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:6387) // suppressing preFAST which raises warning for passing null to the JsRT APIs
#pragma warning(disable:6262) // CATCH is using stack variables to report errors, suppressing the preFAST warning.

namespace JsRTApiTest
{
    bool TestSetup(JsRuntimeAttributes attributes, JsRuntimeHandle *runtime)
    {
        JsValueRef context = JS_INVALID_REFERENCE;
        JsValueRef setContext = JS_INVALID_REFERENCE;

        // Create runtime, context and set current context
        REQUIRE(JsCreateRuntime(attributes, nullptr, runtime) == JsNoError);
        REQUIRE(JsCreateContext(*runtime, &context) == JsNoError);
        REQUIRE(JsSetCurrentContext(context) == JsNoError);
        REQUIRE(((JsGetCurrentContext(&setContext) == JsNoError) || setContext == context));

        return true;
    }

    bool TestCleanup(JsRuntimeHandle runtime)
    {
        if (runtime != nullptr)
        {
            JsSetCurrentContext(nullptr);
            JsDisposeRuntime(runtime);
        }
        return true;
    }

    JsValueRef GetUndefined()
    {
        JsValueRef undefined = JS_INVALID_REFERENCE;
        REQUIRE(JsGetUndefinedValue(&undefined) == JsNoError);
        return undefined;
    }

    template <class Handler>
    void WithSetup(JsRuntimeAttributes attributes, Handler handler)
    {
        JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
        if (!TestSetup(attributes, &runtime))
        {
            REQUIRE(false);
            return;
        }

        handler(attributes, runtime);

        TestCleanup(runtime);
    }

    template <class Handler>
    void RunWithAttributes(Handler handler)
    {
        WithSetup(JsRuntimeAttributeNone, handler);
        WithSetup(JsRuntimeAttributeDisableBackgroundWork, handler);
        WithSetup(JsRuntimeAttributeAllowScriptInterrupt, handler);
        WithSetup(JsRuntimeAttributeEnableIdleProcessing, handler);
        WithSetup(JsRuntimeAttributeDisableNativeCodeGeneration, handler);
        WithSetup(JsRuntimeAttributeDisableEval, handler);
        WithSetup((JsRuntimeAttributes)(JsRuntimeAttributeDisableBackgroundWork | JsRuntimeAttributeAllowScriptInterrupt | JsRuntimeAttributeEnableIdleProcessing), handler);
    }

    void ReferenceCountingTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsContextRef context = JS_INVALID_REFERENCE;
        REQUIRE(JsGetCurrentContext(&context) == JsNoError);

        CHECK(JsAddRef(context, nullptr) == JsNoError);
        CHECK(JsRelease(context, nullptr) == JsNoError);

        JsValueRef undefined = JS_INVALID_REFERENCE;

        REQUIRE(JsGetUndefinedValue(&undefined) == JsNoError);

        REQUIRE(JsSetCurrentContext(nullptr) == JsNoError);
        CHECK(JsAddRef(undefined, nullptr) == JsErrorNoCurrentContext);
        CHECK(JsRelease(undefined, nullptr) == JsErrorNoCurrentContext);

        REQUIRE(JsSetCurrentContext(context) == JsNoError);
        CHECK(JsAddRef(undefined, nullptr) == JsNoError);
        CHECK(JsRelease(undefined, nullptr) == JsNoError);

        JsPropertyIdRef foo = JS_INVALID_REFERENCE;

        REQUIRE(JsGetPropertyIdFromName(_u("foo"), &foo) == JsNoError);
        CHECK(JsAddRef(foo, nullptr) == JsNoError);
        CHECK(JsRelease(foo, nullptr) == JsNoError);
    }

    TEST_CASE("ApiTest_ReferenceCountingTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ReferenceCountingTest);
    }

    void WeakReferenceTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef valueRef = JS_INVALID_REFERENCE;
        REQUIRE(JsCreateString("test", strlen("test"), &valueRef) == JsNoError);

        JsWeakRef weakRef = JS_INVALID_REFERENCE;
        REQUIRE(JsCreateWeakReference(valueRef, &weakRef) == JsNoError);

        // JsGetWeakReferenceValue should return the original value reference.
        JsValueRef valueRefFromWeakRef = JS_INVALID_REFERENCE;
        CHECK(JsGetWeakReferenceValue(weakRef, &valueRefFromWeakRef) == JsNoError);
        CHECK(valueRefFromWeakRef != JS_INVALID_REFERENCE);
        CHECK(valueRefFromWeakRef == valueRef);

        // Clear the references on the stack, so that the value will be GC'd.
        valueRef = JS_INVALID_REFERENCE;
        valueRefFromWeakRef = JS_INVALID_REFERENCE;

        CHECK(JsCollectGarbage(runtime) == JsNoError);

        // JsGetWeakReferenceValue should return an invalid reference after the value was GC'd.
        JsValueRef valueRefAfterGC = JS_INVALID_REFERENCE;
        CHECK(JsGetWeakReferenceValue(weakRef, &valueRefAfterGC) == JsNoError);
        CHECK(valueRefAfterGC == JS_INVALID_REFERENCE);
    }

    TEST_CASE("ApiTest_WeakReferenceTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::WeakReferenceTest);
    }

    void ObjectsAndPropertiesTest1(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef object = JS_INVALID_REFERENCE;
        REQUIRE(JsCreateObject(&object) == JsNoError);

        JsPropertyIdRef name1 = JS_INVALID_REFERENCE;
        const WCHAR* name = nullptr;
        REQUIRE(JsGetPropertyIdFromName(_u("stringProperty1"), &name1) == JsNoError);
        REQUIRE(JsGetPropertyNameFromId(name1, &name) == JsNoError);
        CHECK(!wcscmp(name, _u("stringProperty1")));

        JsPropertyIdType propertyIdType;
        REQUIRE(JsGetPropertyIdType(name1, &propertyIdType) == JsNoError);
        CHECK(propertyIdType == JsPropertyIdTypeString);

        JsPropertyIdRef name2 = JS_INVALID_REFERENCE;
        REQUIRE(JsGetPropertyIdFromName(_u("stringProperty2"), &name2) == JsNoError);

        JsValueRef value1 = JS_INVALID_REFERENCE;
        REQUIRE(JsPointerToString(_u("value1"), wcslen(_u("value1")), &value1) == JsNoError);

        JsValueRef value2 = JS_INVALID_REFERENCE;
        REQUIRE(JsPointerToString(_u("value1"), wcslen(_u("value1")), &value2) == JsNoError);

        REQUIRE(JsSetProperty(object, name1, value1, true) == JsNoError);
        REQUIRE(JsSetProperty(object, name2, value2, true) == JsNoError);

        JsValueRef value1Check = JS_INVALID_REFERENCE;
        REQUIRE(JsGetProperty(object, name1, &value1Check) == JsNoError);
        CHECK(value1 == value1Check);

        JsValueRef value2Check = JS_INVALID_REFERENCE;
        REQUIRE(JsGetProperty(object, name2, &value2Check) == JsNoError);
        CHECK(value1 == value1Check);
    }

    TEST_CASE("ApiTest_ObjectsAndPropertiesTest1", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ObjectsAndPropertiesTest1);
    }

    void ObjectsAndPropertiesTest2(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        // Run a script to setup some globals
        JsValueRef function = JS_INVALID_REFERENCE;

        LPCWSTR script = nullptr;
        REQUIRE(FileLoadHelpers::LoadScriptFromFile("ObjectsAndProperties2.js", script) == S_OK);
        REQUIRE(script != nullptr);

        REQUIRE(JsParseScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), &function) == JsNoError);

        JsValueRef args[] = { JS_INVALID_REFERENCE };
        REQUIRE(JsCallFunction(function, nullptr, 10, nullptr) == JsErrorInvalidArgument);
        REQUIRE(JsCallFunction(function, args, 0, nullptr) == JsErrorInvalidArgument);
        REQUIRE(JsCallFunction(function, args, _countof(args), nullptr) == JsErrorInvalidArgument);
        args[0] = GetUndefined();
        REQUIRE(JsCallFunction(function, args, _countof(args), nullptr) == JsNoError);

        // Get proto properties
        JsValueRef circle = JS_INVALID_REFERENCE;
        REQUIRE(JsRunScript(_u("new Circle()"), JS_SOURCE_CONTEXT_NONE, _u(""), &circle) == JsNoError);

        JsPropertyIdRef name = JS_INVALID_REFERENCE;
        REQUIRE(JsGetPropertyIdFromName(_u("color"), &name) == JsNoError);

        JsValueRef value = JS_INVALID_REFERENCE;
        REQUIRE(JsGetProperty(circle, name, &value) == JsNoError);

        JsValueRef asString = JS_INVALID_REFERENCE;
        REQUIRE(JsConvertValueToString(value, &asString) == JsNoError);

        LPCWSTR str = nullptr;
        size_t length;
        REQUIRE(JsStringToPointer(asString, &str, &length) == JsNoError);
        REQUIRE(!wcscmp(str, _u("white")));
    }

    TEST_CASE("ApiTest_ObjectsAndPropertiesTest2", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ObjectsAndPropertiesTest2);
    }

    void DeleteObjectIndexedPropertyBug(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef object;
        REQUIRE(JsRunScript(_u("({a: 'a', 1: 1, 100: 100})"), JS_SOURCE_CONTEXT_NONE, _u(""), &object) == JsNoError);

        JsPropertyIdRef idRef = JS_INVALID_REFERENCE;
        JsValueRef result = JS_INVALID_REFERENCE;
        // delete property "a" triggers PathTypeHandler -> SimpleDictionaryTypeHandler
        REQUIRE(JsGetPropertyIdFromName(_u("a"), &idRef) == JsNoError);
        REQUIRE(JsDeleteProperty(object, idRef, false, &result) == JsNoError);
        // Now delete property "100". Bug causes we always delete "1" instead.
        REQUIRE(JsGetPropertyIdFromName(_u("100"), &idRef) == JsNoError);
        REQUIRE(JsDeleteProperty(object, idRef, false, &result) == JsNoError);

        bool has;
        JsValueRef indexRef = JS_INVALID_REFERENCE;
        REQUIRE(JsIntToNumber(100, &indexRef) == JsNoError);
        REQUIRE(JsHasIndexedProperty(object, indexRef, &has) == JsNoError);
        CHECK(!has); // index 100 should be deleted
        REQUIRE(JsIntToNumber(1, &indexRef) == JsNoError);
        REQUIRE(JsHasIndexedProperty(object, indexRef, &has) == JsNoError);
        CHECK(has); // index 1 should be intact
    }

    TEST_CASE("ApiTest_DeleteObjectIndexedPropertyBug", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::DeleteObjectIndexedPropertyBug);
    }

    void CALLBACK ExternalObjectFinalizeCallback(void *data)
    {
        CHECK(data == (void *)0xdeadbeef);
    }

    void CrossContextSetPropertyTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        bool hasExternalData;
        JsContextRef oldContext = JS_INVALID_REFERENCE, secondContext = JS_INVALID_REFERENCE, testContext = JS_INVALID_REFERENCE;
        JsValueRef secondValueRef = JS_INVALID_REFERENCE, secondObjectRef = JS_INVALID_REFERENCE, jsrtExternalObjectRef = JS_INVALID_REFERENCE, mainObjectRef = JS_INVALID_REFERENCE;
        JsPropertyIdRef idRef = JS_INVALID_REFERENCE;
        JsValueRef indexRef = JS_INVALID_REFERENCE;
        REQUIRE(JsGetCurrentContext(&oldContext) == JsNoError);
        REQUIRE(JsCreateObject(&mainObjectRef) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("prop1"), &idRef) == JsNoError);
        REQUIRE(JsCreateContext(runtime, &secondContext) == JsNoError);
        REQUIRE(JsSetCurrentContext(secondContext) == JsNoError);
        REQUIRE(JsCreateObject(&secondObjectRef) == JsNoError);

        // Verify the main object is from first context
        REQUIRE(JsGetContextOfObject(mainObjectRef, &testContext) == JsNoError);
        REQUIRE(testContext == oldContext);

        // Create external Object in 2nd context which will be accessed in main context.
        REQUIRE(JsCreateExternalObject((void *)0xdeadbeef, ExternalObjectFinalizeCallback, &jsrtExternalObjectRef) == JsNoError);
        REQUIRE(JsIntToNumber(1, &secondValueRef) == JsNoError);
        REQUIRE(JsIntToNumber(2, &indexRef) == JsNoError);
        REQUIRE(JsSetCurrentContext(oldContext) == JsNoError);

        // Verify the second object is from second context
        REQUIRE(JsGetContextOfObject(secondObjectRef, &testContext) == JsNoError);
        CHECK(testContext == secondContext);

        REQUIRE(JsSetProperty(mainObjectRef, idRef, secondValueRef, false) == JsNoError);
        REQUIRE(JsSetProperty(mainObjectRef, idRef, secondObjectRef, false) == JsNoError);
        REQUIRE(JsSetIndexedProperty(mainObjectRef, indexRef, secondValueRef) == JsNoError);
        REQUIRE(JsSetIndexedProperty(mainObjectRef, indexRef, secondObjectRef) == JsNoError);
        REQUIRE(JsSetPrototype(jsrtExternalObjectRef, mainObjectRef) == JsNoError);
        REQUIRE(JsHasExternalData(jsrtExternalObjectRef, &hasExternalData) == JsNoError);
        REQUIRE(hasExternalData);
    }

    TEST_CASE("ApiTest_CrossContextSetPropertyTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::CrossContextSetPropertyTest);
    }

    void CrossContextFunctionCall(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        /*
        1. function changeFoo() { foo = 100 }
        2. CreateContext
        3. Set f : changeFoo in newContext
        4. Call f() from newContext
        */
        JsContextRef oldContext = JS_INVALID_REFERENCE, secondContext = JS_INVALID_REFERENCE;
        JsValueRef functionRef = JS_INVALID_REFERENCE, functionResultRef = JS_INVALID_REFERENCE, globalRef = JS_INVALID_REFERENCE, globalNewCtxRef = JS_INVALID_REFERENCE, valueRef = JS_INVALID_REFERENCE;
        JsPropertyIdRef propertyIdFRef = JS_INVALID_REFERENCE, propertyIdFooRef = JS_INVALID_REFERENCE, propertyIdChangeFooRef = JS_INVALID_REFERENCE;
        int answer;

        REQUIRE(JsGetCurrentContext(&oldContext) == JsNoError);

        REQUIRE(JsGetPropertyIdFromName(_u("f"), &propertyIdFRef) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("foo"), &propertyIdFooRef) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("changeFoo"), &propertyIdChangeFooRef) == JsNoError);

        //1. function changeFoo() { foo = 100 }
        REQUIRE(JsRunScript(_u("foo = 3; function changeFoo() { foo = 100 }"), JS_SOURCE_CONTEXT_NONE, _u(""), &functionResultRef) == JsNoError);

        REQUIRE(JsGetGlobalObject(&globalRef) == JsNoError);
        REQUIRE(JsGetProperty(globalRef, propertyIdChangeFooRef, &functionRef) == JsNoError);

        //2. CreateContext
        REQUIRE(JsCreateContext(runtime, &secondContext) == JsNoError);
        REQUIRE(JsSetCurrentContext(secondContext) == JsNoError);

        //3. Set f : changeFoo in newContext
        REQUIRE(JsGetGlobalObject(&globalNewCtxRef) == JsNoError);
        REQUIRE(JsSetProperty(globalNewCtxRef, propertyIdFRef, functionRef, false) == JsNoError);

        //4. Call 'f()' from newContext
        REQUIRE(JsRunScript(_u("f()"), JS_SOURCE_CONTEXT_NONE, _u(""), &functionResultRef) == JsNoError);

        //5. Change context to oldContext
        REQUIRE(JsSetCurrentContext(oldContext) == JsNoError);

        //6. Verify foo == 100
        REQUIRE(JsGetProperty(globalRef, propertyIdFooRef, &valueRef) == JsNoError);
        REQUIRE(JsNumberToInt(valueRef, &answer) == JsNoError);
        CHECK(answer == 100);
    }

    TEST_CASE("ApiTest_CrossContextFunctionCall", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::CrossContextSetPropertyTest);
    }

    void ExternalDataOnJsrtContextTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        int i = 5;
        void *j;
        JsContextRef currentContext = JS_INVALID_REFERENCE;

        REQUIRE(JsGetCurrentContext(&currentContext) == JsNoError);
        REQUIRE(JsSetContextData(currentContext, &i) == JsNoError);
        REQUIRE(JsGetContextData(currentContext, &j) == JsNoError);
        CHECK(static_cast<int*>(j) == &i);
    }

    TEST_CASE("ApiTest_ExternalDataOnJsrtContextTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ExternalDataOnJsrtContextTest);
    }

    void ArrayAndItemTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        // Create some arrays
        JsValueRef array1 = JS_INVALID_REFERENCE;
        JsValueRef array2 = JS_INVALID_REFERENCE;

        REQUIRE(JsCreateArray(0, &array1) == JsNoError);
        REQUIRE(JsCreateArray(100, &array2) == JsNoError);

        // Create an object we'll treat like an array
        JsValueRef object = JS_INVALID_REFERENCE;
        REQUIRE(JsCreateObject(&object) == JsNoError);

        // Create an index value to use
        JsValueRef index = JS_INVALID_REFERENCE;
        REQUIRE(JsDoubleToNumber(3, &index) == JsNoError);

        JsValueRef value1 = JS_INVALID_REFERENCE;
        REQUIRE(JsPointerToString(_u("value1"), wcslen(_u("value1")), &value1) == JsNoError);

        // Do some index-based manipulation
        bool result;
        REQUIRE(JsHasIndexedProperty(array1, index, &result) == JsNoError);
        REQUIRE(result == false);
        REQUIRE(JsHasIndexedProperty(array2, index, &result) == JsNoError);
        REQUIRE(result == false);
        REQUIRE(JsHasIndexedProperty(object, index, &result) == JsNoError);
        REQUIRE(result == false);

        REQUIRE(JsSetIndexedProperty(array1, index, value1) == JsNoError);
        REQUIRE(JsSetIndexedProperty(array2, index, value1) == JsNoError);
        REQUIRE(JsSetIndexedProperty(object, index, value1) == JsNoError);

        REQUIRE(JsHasIndexedProperty(array1, index, &result) == JsNoError);
        REQUIRE(result == true);
        REQUIRE(JsHasIndexedProperty(array2, index, &result) == JsNoError);
        REQUIRE(result == true);
        REQUIRE(JsHasIndexedProperty(object, index, &result) == JsNoError);
        REQUIRE(result == true);

        JsValueRef value2 = JS_INVALID_REFERENCE;
        REQUIRE(JsGetIndexedProperty(array1, index, &value2) == JsNoError);
        REQUIRE(JsStrictEquals(value1, value2, &result) == JsNoError);
        REQUIRE(result == true);
        REQUIRE(JsGetIndexedProperty(array2, index, &value2) == JsNoError);
        REQUIRE(JsStrictEquals(value1, value2, &result) == JsNoError);
        REQUIRE(result == true);
        REQUIRE(JsGetIndexedProperty(object, index, &value2) == JsNoError);
        REQUIRE(JsStrictEquals(value1, value2, &result) == JsNoError);
        REQUIRE(result == true);

        REQUIRE(JsDeleteIndexedProperty(array1, index) == JsNoError);
        REQUIRE(JsDeleteIndexedProperty(array2, index) == JsNoError);
        REQUIRE(JsDeleteIndexedProperty(object, index) == JsNoError);

        REQUIRE(JsHasIndexedProperty(array1, index, &result) == JsNoError);
        REQUIRE(result == false);
        REQUIRE(JsHasIndexedProperty(array2, index, &result) == JsNoError);
        REQUIRE(result == false);
        REQUIRE(JsHasIndexedProperty(object, index, &result) == JsNoError);
        REQUIRE(result == false);
    }

    TEST_CASE("ApiTest_ArrayAndItemTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ArrayAndItemTest);
    }

    void EqualsTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        // Create some values
        JsValueRef number1 = JS_INVALID_REFERENCE;
        REQUIRE(JsDoubleToNumber(1, &number1) == JsNoError);
        JsValueRef number2 = JS_INVALID_REFERENCE;
        REQUIRE(JsDoubleToNumber(2, &number2) == JsNoError);
        JsValueRef stringa = JS_INVALID_REFERENCE;
        REQUIRE(JsPointerToString(_u("1"), wcslen(_u("1")), &stringa) == JsNoError);
        JsValueRef stringb = JS_INVALID_REFERENCE;
        REQUIRE(JsPointerToString(_u("1"), wcslen(_u("1")), &stringb) == JsNoError);

        bool result;
        REQUIRE(JsEquals(number1, number1, &result) == JsNoError);
        CHECK(result == true);
        REQUIRE(JsEquals(number1, number2, &result) == JsNoError);
        CHECK(result == false);
        REQUIRE(JsEquals(number1, stringa, &result) == JsNoError);
        CHECK(result == true);
        REQUIRE(JsEquals(number1, stringb, &result) == JsNoError);
        CHECK(result == true);
        REQUIRE(JsEquals(number2, stringa, &result) == JsNoError);
        CHECK(result == false);
        REQUIRE(JsEquals(number2, stringb, &result) == JsNoError);
        CHECK(result == false);
        REQUIRE(JsEquals(stringa, stringb, &result) == JsNoError);
        CHECK(result == true);

        REQUIRE(JsStrictEquals(number1, number1, &result) == JsNoError);
        CHECK(result == true);
        REQUIRE(JsStrictEquals(number1, number2, &result) == JsNoError);
        CHECK(result == false);
        REQUIRE(JsStrictEquals(number1, stringa, &result) == JsNoError);
        CHECK(result == false);
        REQUIRE(JsStrictEquals(number1, stringb, &result) == JsNoError);
        CHECK(result == false);
        REQUIRE(JsStrictEquals(number2, stringa, &result) == JsNoError);
        CHECK(result == false);
        REQUIRE(JsStrictEquals(number2, stringb, &result) == JsNoError);
        CHECK(result == false);
        REQUIRE(JsStrictEquals(stringa, stringb, &result) == JsNoError);
        CHECK(result == true);
    }

    TEST_CASE("ApiTest_EqualsTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::EqualsTest);
    }

    void InstanceOfTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef F = JS_INVALID_REFERENCE;
        REQUIRE(JsRunScript(_u("F = function(){}"), JS_SOURCE_CONTEXT_NONE, _u(""), &F) == JsNoError);
        JsValueRef x = JS_INVALID_REFERENCE;
        REQUIRE(JsRunScript(_u("new F()"), JS_SOURCE_CONTEXT_NONE, _u(""), &x) == JsNoError);

        bool instanceOf;
        REQUIRE(JsInstanceOf(x, F, &instanceOf) == JsNoError);
        REQUIRE(instanceOf);

        REQUIRE(JsCreateObject(&x) == JsNoError);
        REQUIRE(JsInstanceOf(x, F, &instanceOf) == JsNoError);
        CHECK(instanceOf == false);
    }

    TEST_CASE("ApiTest_InstanceOfTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::InstanceOfTest);
    }

    void LanguageTypeConversionTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef value = JS_INVALID_REFERENCE;
        JsValueType type;
        JsValueRef asString = JS_INVALID_REFERENCE;
        LPCWSTR str = nullptr;
        size_t length;

        // Number

        REQUIRE(JsDoubleToNumber(3.141592, &value) == JsNoError);
        REQUIRE(JsGetValueType(value, &type) == JsNoError);
        REQUIRE(type == JsNumber);

        double dbl = 0.0;
        REQUIRE(JsNumberToDouble(value, &dbl) == JsNoError);
        REQUIRE(dbl == 3.141592);

        REQUIRE(JsPointerToString(_u("3.141592"), wcslen(_u("3.141592")), &asString) == JsNoError);
        REQUIRE(JsConvertValueToNumber(asString, &value) == JsNoError);
        REQUIRE(JsNumberToDouble(value, &dbl) == JsNoError);
        REQUIRE(dbl == 3.141592);

        int intValue;
        REQUIRE(JsNumberToInt(value, &intValue) == JsNoError);
        CHECK(3 == intValue);

        REQUIRE(JsDoubleToNumber(2147483648.1, &value) == JsNoError);
        REQUIRE(JsNumberToInt(value, &intValue) == JsNoError);
        CHECK(INT_MIN == intValue);
        REQUIRE(JsDoubleToNumber(-2147483649.1, &value) == JsNoError);
        REQUIRE(JsNumberToInt(value, &intValue) == JsNoError);
        CHECK(2147483647 == intValue);

        // String

        REQUIRE(JsDoubleToNumber(3.141592, &value) == JsNoError);
        REQUIRE(JsConvertValueToString(value, &asString) == JsNoError);
        REQUIRE(JsStringToPointer(asString, &str, &length) == JsNoError);
        CHECK(!wcscmp(str, _u("3.141592")));

        REQUIRE(JsGetTrueValue(&value) == JsNoError);

        REQUIRE(JsConvertValueToString(value, &asString) == JsNoError);
        REQUIRE(JsGetValueType(asString, &type) == JsNoError);
        CHECK(type == JsString);
        REQUIRE(JsStringToPointer(asString, &str, &length) == JsNoError);
        CHECK(!wcscmp(str, _u("true")));

        // Undefined

        REQUIRE(JsGetUndefinedValue(&value) == JsNoError);
        REQUIRE(JsGetValueType(value, &type) == JsNoError);
        CHECK(type == JsUndefined);

        // Null

        REQUIRE(JsGetNullValue(&value) == JsNoError);
        REQUIRE(JsGetValueType(value, &type) == JsNoError);
        CHECK(type == JsNull);

        // Boolean

        REQUIRE(JsGetTrueValue(&value) == JsNoError);
        REQUIRE(JsGetValueType(value, &type) == JsNoError);
        CHECK(type == JsBoolean);

        REQUIRE(JsGetFalseValue(&value) == JsNoError);
        REQUIRE(JsGetValueType(value, &type) == JsNoError);
        CHECK(type == JsBoolean);

        bool boolValue;

        REQUIRE(JsBoolToBoolean(true, &value) == JsNoError);
        REQUIRE(JsGetValueType(value, &type) == JsNoError);
        CHECK(type == JsBoolean);
        REQUIRE(JsBooleanToBool(value, &boolValue) == JsNoError);
        CHECK(boolValue == true);

        REQUIRE(JsPointerToString(_u("true"), wcslen(_u("true")), &asString) == JsNoError);
        REQUIRE(JsConvertValueToBoolean(asString, &value) == JsNoError);
        REQUIRE(JsGetValueType(value, &type) == JsNoError);
        CHECK(type == JsBoolean);
        REQUIRE(JsBooleanToBool(value, &boolValue) == JsNoError);
        CHECK(boolValue == true);

        // Object

        REQUIRE(JsCreateObject(&value) == JsNoError);
        REQUIRE(JsGetValueType(value, &type) == JsNoError);
        CHECK(type == JsObject);
    }

    TEST_CASE("ApiTest_LanguageTypeConversionTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::LanguageTypeConversionTest);
    }

    JsValueRef CALLBACK ExternalFunctionCallback(JsValueRef /* function */, bool /* isConstructCall */, JsValueRef * /* args */, USHORT /* cargs */, void * /* callbackState */)
    {
        return nullptr;
    }

    void CALLBACK FinalizeCallback(JsValueRef object)
    {
        CHECK(object != JS_INVALID_REFERENCE);
    }

    void CALLBACK OldFinalizeCallback(void *data)
    {
        CHECK(data == nullptr);
    }

    JsValueRef CALLBACK ExternalFunctionPreScriptAbortionCallback(JsValueRef /* function */, bool /* isConstructCall */, JsValueRef * args /* args */, USHORT /* cargs */, void * /* callbackState */)
    {
        JsValueRef result = JS_INVALID_REFERENCE;
        const WCHAR *scriptText = nullptr;
        size_t scriptTextLen;

        REQUIRE(JsStringToPointer(args[0], &scriptText, &scriptTextLen) == JsNoError);
        REQUIRE(JsRunScript(scriptText, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsErrorScriptTerminated);
        return nullptr;
    }

    JsValueRef CALLBACK ExternalFunctionPostScriptAbortionCallback(JsValueRef /* function */, bool /* isConstructCall */, JsValueRef * args /* args */, USHORT /* cargs */, void * /* callbackState */)
    {
        JsValueRef result = JS_INVALID_REFERENCE;
        const WCHAR *scriptText = nullptr;
        size_t scriptTextLen;

        REQUIRE(JsStringToPointer(args[0], &scriptText, &scriptTextLen) == JsNoError);
        REQUIRE(JsRunScript(scriptText, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsErrorInDisabledState);
        return nullptr;
    }

    JsValueRef CALLBACK ErrorHandlingCallback(JsValueRef /* function */, bool /* isConstructCall */, JsValueRef * /* args */, USHORT /* cargs */, void * /* callbackState */)
    {
        JsValueRef result = JS_INVALID_REFERENCE;

        REQUIRE(JsRunScript(_u("new Error()"), JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsNoError);
        REQUIRE(JsSetException(result) == JsNoError);

        return nullptr;
    }


    void ExternalFunctionTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef function = JS_INVALID_REFERENCE;
        REQUIRE(JsCreateFunction(ExternalFunctionCallback, nullptr, &function) == JsNoError);
        JsValueRef undefined = JS_INVALID_REFERENCE;
        REQUIRE(JsGetUndefinedValue(&undefined) == JsNoError);
        JsValueRef args[] = { undefined };
        REQUIRE(JsCallFunction(function, args, 1, nullptr) == JsNoError);
        JsValueRef result = JS_INVALID_REFERENCE;
        REQUIRE(JsConstructObject(function, args, 1, &result) == JsNoError);
    }

    TEST_CASE("ApiTest_ExternalFunctionTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ExternalFunctionTest);
    }

    void ExternalFunctionNameTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        auto testConstructorName = [=](JsValueRef function, PCWCHAR expectedName, size_t expectedNameLength)
        {
            JsValueRef undefined = JS_INVALID_REFERENCE;
            REQUIRE(JsGetUndefinedValue(&undefined) == JsNoError);
            JsValueRef args[] = { undefined };

            JsValueRef obj = JS_INVALID_REFERENCE, constructor = JS_INVALID_REFERENCE, name = JS_INVALID_REFERENCE;
            JsPropertyIdRef propId = JS_INVALID_REFERENCE;
            REQUIRE(JsConstructObject(function, args, 1, &obj) == JsNoError);
            REQUIRE(JsGetPropertyIdFromName(_u("constructor"), &propId) == JsNoError);
            REQUIRE(JsGetProperty(obj, propId, &constructor) == JsNoError);
            REQUIRE(JsGetPropertyIdFromName(_u("name"), &propId) == JsNoError);
            REQUIRE(JsGetProperty(constructor, propId, &name) == JsNoError);

            PCWCHAR actualName = nullptr;
            size_t actualNameLength;
            REQUIRE(JsStringToPointer(name, &actualName, &actualNameLength) == JsNoError);
            CHECK(expectedNameLength == actualNameLength);
            CHECK(wcscmp(expectedName, actualName) == 0);
        };

        auto testToStringResult = [=](JsValueRef function, PCWCHAR expectedResult, size_t expectedResultLength)
        {
            JsValueRef actualResult = JS_INVALID_REFERENCE;
            REQUIRE(JsConvertValueToString(function, &actualResult) == JsNoError);

            PCWCHAR actualResultString = nullptr;
            size_t actualResultLength;
            REQUIRE(JsStringToPointer(actualResult, &actualResultString, &actualResultLength) == JsNoError);
            CHECK(expectedResultLength == actualResultLength);
            CHECK(wcscmp(expectedResult, actualResultString) == 0);
        };

        JsValueRef function = JS_INVALID_REFERENCE;
        REQUIRE(JsCreateFunction(ExternalFunctionCallback, nullptr, &function) == JsNoError);
        testConstructorName(function, _u(""), 0);

        WCHAR name[] = _u("FooName");
        JsValueRef nameString = JS_INVALID_REFERENCE;
        REQUIRE(JsPointerToString(name, _countof(name) - 1, &nameString) == JsNoError);
        REQUIRE(JsCreateNamedFunction(nameString, ExternalFunctionCallback, nullptr, &function) == JsNoError);
        testConstructorName(function, name, _countof(name) - 1);

        WCHAR toStringExpectedResult[] = _u("function FooName() { [native code] }");
        testToStringResult(function, toStringExpectedResult, _countof(toStringExpectedResult) - 1);
        // Calling toString multiple times should return the same result.
        testToStringResult(function, toStringExpectedResult, _countof(toStringExpectedResult) - 1);
    }

    TEST_CASE("ApiTest_ExternalFunctionNameTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ExternalFunctionNameTest);
    }

    void ErrorHandlingTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        // Run a script to setup some globals
        LPCWSTR script = nullptr;
        REQUIRE(FileLoadHelpers::LoadScriptFromFile("ErrorHandling.js", script) == S_OK);
        REQUIRE(script != nullptr);

        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), nullptr) == JsNoError);

        JsValueRef global = JS_INVALID_REFERENCE;
        REQUIRE(JsGetGlobalObject(&global) == JsNoError);

        JsPropertyIdRef name = JS_INVALID_REFERENCE;
        JsValueRef result = JS_INVALID_REFERENCE;
        JsValueRef exception = JS_INVALID_REFERENCE;
        JsValueRef function = JS_INVALID_REFERENCE;
        JsValueType type;

        JsValueRef args[] = { GetUndefined() };

        // throw from script, handle in host
        REQUIRE(JsGetPropertyIdFromName(_u("throwAtHost"), &name) == JsNoError);
        REQUIRE(JsGetProperty(global, name, &function) == JsNoError);
        REQUIRE(JsCallFunction(function, args, _countof(args), &result) == JsErrorScriptException);
        REQUIRE(JsGetAndClearException(&exception) == JsNoError);

        // setup a host callback for the next couple of tests
        REQUIRE(JsCreateFunction(ErrorHandlingCallback, nullptr, &result) == JsNoError);
        REQUIRE(JsGetValueType(result, &type) == JsNoError);
        REQUIRE(type == JsFunction);
        REQUIRE(JsGetPropertyIdFromName(_u("callHost"), &name) == JsNoError);
        REQUIRE(JsSetProperty(global, name, result, true) == JsNoError);

        // throw from host callback, catch in script
        REQUIRE(JsGetPropertyIdFromName(_u("callHostWithTryCatch"), &name) == JsNoError);
        REQUIRE(JsGetProperty(global, name, &function) == JsNoError);
        REQUIRE(JsCallFunction(function, args, _countof(args), &result) == JsNoError);

        // throw from host callback, through script, handle in host
        REQUIRE(JsGetPropertyIdFromName(_u("callHostWithNoTryCatch"), &name) == JsNoError);
        REQUIRE(JsGetProperty(global, name, &function) == JsNoError);
        REQUIRE(JsCallFunction(function, args, _countof(args), &result) == JsErrorScriptException);
    }

    TEST_CASE("ApiTest_ErrorHandlingTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ErrorHandlingTest);
    }

    void EngineFlagTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef ret = JS_INVALID_REFERENCE;
        REQUIRE(JsRunScript(_u("new ActiveXObject('Scripting.FileSystemObject')"), JS_SOURCE_CONTEXT_NONE, _u(""), &ret) == JsErrorScriptException);
        REQUIRE(JsGetAndClearException(&ret) == JsNoError);
    }

    TEST_CASE("ApiTest_EngineFlagTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::EngineFlagTest);
    }

    void ExceptionHandlingTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        bool value;
        JsValueRef exception = JS_INVALID_REFERENCE;
        JsValueRef exceptionMetadata = JS_INVALID_REFERENCE;
        JsValueRef metadataValue = JS_INVALID_REFERENCE;
        JsPropertyIdRef property = JS_INVALID_REFERENCE;
        JsValueType type;

        REQUIRE(JsHasException(&value) == JsNoError);
        CHECK(value == false);
        REQUIRE(JsRunScript(_u("throw new Error()"), JS_SOURCE_CONTEXT_NONE, _u(""), nullptr) == JsErrorScriptException);
        REQUIRE(JsHasException(&value) == JsNoError);
        CHECK(value == true);

        REQUIRE(JsGetAndClearException(&exception) == JsNoError);
        REQUIRE(JsHasException(&value) == JsNoError);
        CHECK(value == false);

        REQUIRE(JsGetValueType(exception, &type) == JsNoError);
        CHECK(type == JsError);

        REQUIRE(JsSetException(exception) == JsNoError);
        REQUIRE(JsHasException(&value) == JsNoError);
        CHECK(value == true);

        REQUIRE(JsGetAndClearExceptionWithMetadata(&exceptionMetadata) == JsNoError);
        REQUIRE(JsHasException(&value) == JsNoError);
        CHECK(value == false);

        REQUIRE(JsGetPropertyIdFromName(_u("exception"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        CHECK(metadataValue == exception);

        REQUIRE(JsGetPropertyIdFromName(_u("line"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsNumber);

        REQUIRE(JsGetPropertyIdFromName(_u("column"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsNumber);

        REQUIRE(JsGetPropertyIdFromName(_u("length"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsNumber);

        REQUIRE(JsGetPropertyIdFromName(_u("url"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsString);

        REQUIRE(JsGetPropertyIdFromName(_u("source"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsString);


        REQUIRE(JsHasException(&value) == JsNoError);
        CHECK(value == false);
        REQUIRE(JsGetAndClearExceptionWithMetadata(&exceptionMetadata) == JsErrorInvalidArgument);
        CHECK(exceptionMetadata == JS_INVALID_REFERENCE);


        REQUIRE(JsRunScript(_u("@ bad syntax"), JS_SOURCE_CONTEXT_NONE, _u(""), nullptr) == JsErrorScriptCompile);
        REQUIRE(JsHasException(&value) == JsNoError);
        CHECK(value == true);

        REQUIRE(JsGetAndClearExceptionWithMetadata(&exceptionMetadata) == JsNoError);
        REQUIRE(JsHasException(&value) == JsNoError);
        CHECK(value == false);

        REQUIRE(JsGetPropertyIdFromName(_u("exception"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsError);

        REQUIRE(JsGetPropertyIdFromName(_u("line"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsNumber);

        REQUIRE(JsGetPropertyIdFromName(_u("column"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsNumber);

        REQUIRE(JsGetPropertyIdFromName(_u("length"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsNumber);

        REQUIRE(JsGetPropertyIdFromName(_u("url"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsString);

        REQUIRE(JsGetPropertyIdFromName(_u("source"), &property) == JsNoError);
        REQUIRE(JsGetProperty(exceptionMetadata, property, &metadataValue) == JsNoError);
        REQUIRE(JsGetValueType(metadataValue, &type) == JsNoError);
        CHECK(type == JsString);
    }

    TEST_CASE("ApiTest_ExceptionHandlingTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ExceptionHandlingTest);
    }

    void ScriptCompileErrorTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef error = JS_INVALID_REFERENCE;

        REQUIRE(JsRunScript(
            _u("if (x > 0) { \n") \
            _u("  x = 1;     \n") \
            _u("}}"),
            JS_SOURCE_CONTEXT_NONE, _u(""), nullptr) == JsErrorScriptCompile);

        REQUIRE(JsGetAndClearException(&error) == JsNoError);

        JsPropertyIdRef property = JS_INVALID_REFERENCE;
        JsValueRef value = JS_INVALID_REFERENCE;
        LPCWSTR str = nullptr;
        size_t length;

        REQUIRE(JsGetPropertyIdFromName(_u("message"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsStringToPointer(value, &str, &length) == JsNoError);
        CHECK(wcscmp(_u("Syntax error"), str) == 0);

        double dbl;

        REQUIRE(JsGetPropertyIdFromName(_u("line"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsNumberToDouble(value, &dbl) == JsNoError);
        CHECK(2 == dbl);

        REQUIRE(JsGetPropertyIdFromName(_u("column"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsNumberToDouble(value, &dbl) == JsNoError);
        CHECK(1 == dbl);

        REQUIRE(JsGetPropertyIdFromName(_u("length"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsNumberToDouble(value, &dbl) == JsNoError);
        CHECK(1 == dbl);

        REQUIRE(JsGetPropertyIdFromName(_u("source"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsStringToPointer(value, &str, &length) == JsNoError);
        CHECK(wcscmp(_u("}}"), str) == 0);

        REQUIRE(JsRunScript(
            _u("var for = 10;\n"),
            JS_SOURCE_CONTEXT_NONE, _u(""), nullptr) == JsErrorScriptCompile);
        REQUIRE(JsGetAndClearException(&error) == JsNoError);

        REQUIRE(JsGetPropertyIdFromName(_u("message"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsStringToPointer(value, &str, &length) == JsNoError);
        CHECK(wcscmp(_u("The use of a keyword for an identifier is invalid"), str) == 0);

        REQUIRE(JsGetPropertyIdFromName(_u("line"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsNumberToDouble(value, &dbl) == JsNoError);
        CHECK(0 == dbl);

        REQUIRE(JsGetPropertyIdFromName(_u("column"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsNumberToDouble(value, &dbl) == JsNoError);
        CHECK(4 == dbl);

        REQUIRE(JsGetPropertyIdFromName(_u("length"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsNumberToDouble(value, &dbl) == JsNoError);
        CHECK(3 == dbl);

        REQUIRE(JsGetPropertyIdFromName(_u("source"), &property) == JsNoError);
        REQUIRE(JsGetProperty(error, property, &value) == JsNoError);
        REQUIRE(JsStringToPointer(value, &str, &length) == JsNoError);
        CHECK(wcscmp(_u("var for = 10;"), str) == 0);
    }

    TEST_CASE("ApiTest_ScriptCompileErrorTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ScriptCompileErrorTest);
    }

    void ObjectTests(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        LPCWSTR script = _u("x = { a: \"abc\", b: \"def\", c: \"ghi\" }; x");
        JsValueRef result = JS_INVALID_REFERENCE;
        JsValueRef propertyNames = JS_INVALID_REFERENCE;

        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsNoError);
        REQUIRE(JsGetOwnPropertyNames(result, &propertyNames) == JsNoError);

        for (int index = 0; index < 3; index++)
        {
            JsValueRef indexValue = JS_INVALID_REFERENCE;
            REQUIRE(JsDoubleToNumber(index, &indexValue) == JsNoError);

            JsValueRef nameValue = JS_INVALID_REFERENCE;
            REQUIRE(JsGetIndexedProperty(propertyNames, indexValue, &nameValue) == JsNoError);

            const WCHAR *name = nullptr;
            size_t length;
            REQUIRE(JsStringToPointer(nameValue, &name, &length) == JsNoError);

            CHECK(length == 1);
            CHECK(name[0] == ('a' + index));
        }
    }

    TEST_CASE("ApiTest_ObjectTests", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ObjectTests);
    }

    void SymbolTests(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef object = JS_INVALID_REFERENCE;
        JsValueRef symbol1 = JS_INVALID_REFERENCE;
        JsValueRef string1 = JS_INVALID_REFERENCE;
        JsValueRef symbol2 = JS_INVALID_REFERENCE;
        JsValueRef value = JS_INVALID_REFERENCE;
        JsPropertyIdRef propertyId = JS_INVALID_REFERENCE;
        JsValueRef outValue = JS_INVALID_REFERENCE;
        JsValueRef propertySymbols = JS_INVALID_REFERENCE;
        const WCHAR* name = nullptr;
        JsPropertyIdType propertyIdType;

        REQUIRE(JsCreateObject(&object) == JsNoError);
        REQUIRE(JsGetPropertyIdFromSymbol(object, &propertyId) == JsErrorPropertyNotSymbol);

        REQUIRE(JsPointerToString(_u("abc"), 3, &string1) == JsNoError);
        REQUIRE(JsCreateSymbol(string1, &symbol1) == JsNoError);

        REQUIRE(JsCreateSymbol(JS_INVALID_REFERENCE, &symbol2) == JsNoError);

        REQUIRE(JsIntToNumber(1, &value) == JsNoError);
        REQUIRE(JsGetPropertyIdFromSymbol(symbol1, &propertyId) == JsNoError);
        REQUIRE(JsGetPropertyNameFromId(propertyId, &name) == JsErrorPropertyNotString);
        REQUIRE(JsGetPropertyIdType(propertyId, &propertyIdType) == JsNoError);
        CHECK(propertyIdType == JsPropertyIdTypeSymbol);

        JsValueRef symbol11 = JS_INVALID_REFERENCE;
        bool resultBool;
        REQUIRE(JsGetSymbolFromPropertyId(propertyId, &symbol11) == JsNoError);
        REQUIRE(JsEquals(symbol1, symbol11, &resultBool) == JsNoError);
        CHECK(resultBool);
        REQUIRE(JsStrictEquals(symbol1, symbol11, &resultBool) == JsNoError);
        CHECK(resultBool);

        REQUIRE(JsSetProperty(object, propertyId, value, true) == JsNoError);
        REQUIRE(JsGetProperty(object, propertyId, &outValue) == JsNoError);
        CHECK(value == outValue);

        REQUIRE(JsGetPropertyIdFromSymbol(symbol2, &propertyId) == JsNoError);
        REQUIRE(JsSetProperty(object, propertyId, value, true) == JsNoError);
        REQUIRE(JsGetProperty(object, propertyId, &outValue) == JsNoError);
        CHECK(value == outValue);

        JsValueType type;
        JsValueRef index = JS_INVALID_REFERENCE;

        REQUIRE(JsGetOwnPropertySymbols(object, &propertySymbols) == JsNoError);

        REQUIRE(JsIntToNumber(0, &index) == JsNoError);
        REQUIRE(JsGetIndexedProperty(propertySymbols, index, &outValue) == JsNoError);
        REQUIRE(JsGetValueType(outValue, &type) == JsNoError);
        CHECK(type == JsSymbol);

        REQUIRE(JsGetPropertyIdFromSymbol(outValue, &propertyId) == JsNoError);
        REQUIRE(JsGetProperty(object, propertyId, &outValue) == JsNoError);
        CHECK(value == outValue);

        REQUIRE(JsIntToNumber(1, &index) == JsNoError);
        REQUIRE(JsGetIndexedProperty(propertySymbols, index, &outValue) == JsNoError);
        REQUIRE(JsGetValueType(outValue, &type) == JsNoError);
        CHECK(type == JsSymbol);

        REQUIRE(JsGetPropertyIdFromSymbol(outValue, &propertyId) == JsNoError);
        REQUIRE(JsGetProperty(object, propertyId, &outValue) == JsNoError);
        CHECK(value == outValue);
    }

    TEST_CASE("ApiTest_SymbolTests", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::SymbolTests);
    }

    void ByteCodeTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        LPCWSTR script = _u("function test() { return true; }; test();");
        JsValueRef result = JS_INVALID_REFERENCE;
        JsValueType type;
        bool boolValue;
        BYTE *compiledScript = nullptr;
        unsigned int scriptSize = 0;

        REQUIRE(JsSerializeScript(script, compiledScript, &scriptSize) == JsNoError);
        compiledScript = new BYTE[scriptSize];
        unsigned int newScriptSize = scriptSize;
        REQUIRE(JsSerializeScript(script, compiledScript, &newScriptSize) == JsNoError);
        CHECK(newScriptSize == scriptSize);
        REQUIRE(JsRunSerializedScript(script, compiledScript, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsNoError);
        REQUIRE(JsGetValueType(result, &type) == JsNoError);
        REQUIRE(JsBooleanToBool(result, &boolValue) == JsNoError);
        CHECK(boolValue);

        JsRuntimeHandle second = JS_INVALID_RUNTIME_HANDLE;
        JsContextRef secondContext = JS_INVALID_REFERENCE, current = JS_INVALID_REFERENCE;

        REQUIRE(JsCreateRuntime(attributes, NULL, &second) == JsNoError);
        REQUIRE(JsCreateContext(second, &secondContext) == JsNoError);
        REQUIRE(JsGetCurrentContext(&current) == JsNoError);
        REQUIRE(JsSetCurrentContext(secondContext) == JsNoError);

        REQUIRE(JsRunSerializedScript(script, compiledScript, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsNoError);
        REQUIRE(JsGetValueType(result, &type) == JsNoError);
        REQUIRE(JsBooleanToBool(result, &boolValue) == JsNoError);
        CHECK(boolValue);

        REQUIRE(JsSetCurrentContext(current) == JsNoError);
        REQUIRE(JsDisposeRuntime(second) == JsNoError);

        delete[] compiledScript;
    }

    TEST_CASE("ApiTest_ByteCodeTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ByteCodeTest);
    }

#define BYTECODEWITHCALLBACK_METHODBODY _u("function test() { return true; }")
    typedef struct _ByteCodeCallbackTracker
    {
        bool loadedScript;
        bool unloadedScript;
        LPCWSTR script;
    } ByteCodeCallbackTracker;

    void ByteCodeWithCallbackTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        LPCWSTR fn_decl = BYTECODEWITHCALLBACK_METHODBODY;
        LPCWSTR script = BYTECODEWITHCALLBACK_METHODBODY _u("; test();");
        LPCWSTR scriptFnToString = BYTECODEWITHCALLBACK_METHODBODY _u("; test.toString();");
        JsValueRef result = JS_INVALID_REFERENCE;
        JsValueType type;
        bool boolValue;
        BYTE *compiledScript = nullptr;
        unsigned int scriptSize = 0;
        const WCHAR *stringValue;
        size_t stringLength;
        ByteCodeCallbackTracker tracker = {};

        JsRuntimeHandle first = JS_INVALID_RUNTIME_HANDLE, second = JS_INVALID_RUNTIME_HANDLE;
        JsContextRef firstContext = JS_INVALID_REFERENCE, secondContext = JS_INVALID_REFERENCE, current = JS_INVALID_REFERENCE;

        REQUIRE(JsCreateRuntime(attributes, NULL, &first) == JsNoError);
        REQUIRE(JsCreateContext(first, &firstContext) == JsNoError);
        REQUIRE(JsGetCurrentContext(&current) == JsNoError);

        // First run the script returning a boolean.  This should not require the source code.
        REQUIRE(JsSerializeScript(script, compiledScript, &scriptSize) == JsNoError);
        compiledScript = (BYTE*)VirtualAlloc(nullptr, scriptSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        unsigned int newScriptSize = scriptSize;
        REQUIRE(JsSerializeScript(script, compiledScript, &newScriptSize) == JsNoError);
        CHECK(newScriptSize == scriptSize);

        /*Change protection to READONLY as serialized byte code is supposed to be in READONLY region*/

        DWORD oldProtect;
        VirtualProtect(compiledScript, scriptSize, PAGE_READONLY, &oldProtect);
        CHECK(oldProtect == PAGE_READWRITE);

        tracker.script = script;
        REQUIRE(JsRunSerializedScriptWithCallback(
            [](JsSourceContext sourceContext, const WCHAR** scriptBuffer)
        {
            ((ByteCodeCallbackTracker*)sourceContext)->loadedScript = true;
            *scriptBuffer = ((ByteCodeCallbackTracker*)sourceContext)->script;
            return true;
        },
            [](JsSourceContext sourceContext)
        {
            // unless we can force unloaded before popping the stack we cant touch tracker.
            //((ByteCodeCallbackTracker*)sourceContext)->unloadedScript = true;
        },
            compiledScript, (JsSourceContext)&tracker, _u(""), &result) == JsNoError);

        CHECK(tracker.loadedScript == false);

        REQUIRE(JsGetValueType(result, &type) == JsNoError);
        REQUIRE(JsBooleanToBool(result, &boolValue) == JsNoError);
        CHECK(boolValue);

        REQUIRE(JsSetCurrentContext(current) == JsNoError);
        REQUIRE(JsDisposeRuntime(first) == JsNoError);

        // Create a second runtime.
        REQUIRE(JsCreateRuntime(attributes, nullptr, &second) == JsNoError);
        REQUIRE(JsCreateContext(second, &secondContext) == JsNoError);
        REQUIRE(JsSetCurrentContext(secondContext) == JsNoError);


        tracker.loadedScript = false;
        tracker.unloadedScript = false;
        VirtualFree(compiledScript, 0, MEM_RELEASE);
        compiledScript = nullptr;
        scriptSize = 0;

        // Second run the script returning function.toString().  This should require the source code.
        REQUIRE(JsSerializeScript(scriptFnToString, compiledScript, &scriptSize) == JsNoError);
        compiledScript = (BYTE*)VirtualAlloc(nullptr, scriptSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        newScriptSize = scriptSize;
        REQUIRE(JsSerializeScript(scriptFnToString, compiledScript, &newScriptSize) == JsNoError);
        CHECK(newScriptSize == scriptSize);

        /*Change protection to READONLY as serialized byte code is supposed to be in READONLY region*/

        oldProtect;
        VirtualProtect(compiledScript, scriptSize, PAGE_READONLY, &oldProtect);
        REQUIRE(oldProtect == PAGE_READWRITE);
        tracker.script = scriptFnToString;
        REQUIRE(JsRunSerializedScriptWithCallback(
            [](JsSourceContext sourceContext, const WCHAR** scriptBuffer)
        {
            ((ByteCodeCallbackTracker*)sourceContext)->loadedScript = true;
            *scriptBuffer = ((ByteCodeCallbackTracker*)sourceContext)->script;
            return true;
        },
            [](JsSourceContext sourceContext)
        {
            // unless we can force unloaded before popping the stack we cant touch tracker.

        },
            compiledScript, (JsSourceContext)&tracker, _u(""), &result) == JsNoError);

        CHECK(tracker.loadedScript);

        REQUIRE(JsGetValueType(result, &type) == JsNoError);
        REQUIRE(JsStringToPointer(result, &stringValue, &stringLength) == JsNoError);
        CHECK(wcscmp(fn_decl, stringValue) == 0);

        REQUIRE(JsSetCurrentContext(current) == JsNoError);
        REQUIRE(JsDisposeRuntime(second) == JsNoError);
        VirtualFree(compiledScript, 0, MEM_RELEASE);
    }

    TEST_CASE("ApiTest_ByteCodeWithCallbackTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ByteCodeWithCallbackTest);
    }

    void ContextCleanupTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsRuntimeHandle rt;
        REQUIRE(JsCreateRuntime(attributes, nullptr, &rt) == JsNoError);

        JsContextRef c1 = JS_INVALID_REFERENCE,
            c2 = JS_INVALID_REFERENCE,
            c3 = JS_INVALID_REFERENCE,
            c4 = JS_INVALID_REFERENCE,
            c5 = JS_INVALID_REFERENCE,
            c6 = JS_INVALID_REFERENCE,
            c7 = JS_INVALID_REFERENCE;

        // Create a bunch of contexts

        REQUIRE(JsCreateContext(rt, &c1) == JsNoError);
        REQUIRE(JsCreateContext(rt, &c2) == JsNoError);
        REQUIRE(JsCreateContext(rt, &c3) == JsNoError);
        REQUIRE(JsCreateContext(rt, &c4) == JsNoError);
        REQUIRE(JsCreateContext(rt, &c5) == JsNoError);
        REQUIRE(JsCreateContext(rt, &c6) == JsNoError);
        REQUIRE(JsCreateContext(rt, &c7) == JsNoError);

        // Clear references to some, then collect, causing them to be collected
        c1 = nullptr;
        c3 = nullptr;
        c5 = nullptr;
        c7 = nullptr;

        REQUIRE(JsCollectGarbage(rt) == JsNoError);

        // Now dispose
        REQUIRE(JsDisposeRuntime(rt) == JsNoError);
    }

    TEST_CASE("ApiTest_ContextCleanupTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ContextCleanupTest);
    }

    void ObjectMethodTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef proto = JS_INVALID_REFERENCE;
        JsValueRef object = JS_INVALID_REFERENCE;

        REQUIRE(JsCreateObject(&proto) == JsNoError);
        REQUIRE(JsCreateObject(&object) == JsNoError);
        REQUIRE(JsSetPrototype(object, proto) == JsNoError);

        JsValueRef objectProto = JS_INVALID_REFERENCE;

        REQUIRE(JsGetPrototype(object, &objectProto) == JsNoError);
        CHECK(proto == objectProto);

        JsPropertyIdRef propertyId = JS_INVALID_REFERENCE;
        bool hasProperty;
        JsValueRef deleteResult = JS_INVALID_REFERENCE;

        REQUIRE(JsGetPropertyIdFromName(_u("foo"), &propertyId) == JsNoError);
        REQUIRE(JsSetProperty(object, propertyId, object, true) == JsNoError);
        REQUIRE(JsHasProperty(object, propertyId, &hasProperty) == JsNoError);
        CHECK(hasProperty);
        REQUIRE(JsDeleteProperty(object, propertyId, true, &deleteResult) == JsNoError);
        REQUIRE(JsHasProperty(object, propertyId, &hasProperty) == JsNoError);
        CHECK(!hasProperty);

        bool canExtend;
        REQUIRE(JsGetExtensionAllowed(object, &canExtend) == JsNoError);
        CHECK(canExtend);
        REQUIRE(JsPreventExtension(object) == JsNoError);
        REQUIRE(JsGetExtensionAllowed(object, &canExtend) == JsNoError);
        CHECK(!canExtend);
        REQUIRE(JsSetProperty(object, propertyId, object, true) == JsErrorScriptException);
        JsValueRef exception = JS_INVALID_REFERENCE;
        REQUIRE(JsGetAndClearException(&exception) == JsNoError);
        REQUIRE(JsHasProperty(object, propertyId, &hasProperty) == JsNoError);
        CHECK(!hasProperty);
    }

    TEST_CASE("ApiTest_ObjectMethodTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ObjectMethodTest);
    }

    void SetPrototypeTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef proto = JS_INVALID_REFERENCE;
        JsValueRef object1 = JS_INVALID_REFERENCE;
        JsValueRef object2 = JS_INVALID_REFERENCE;
        JsPropertyIdRef obj1_a_pid = JS_INVALID_REFERENCE;
        JsPropertyIdRef obj1_b_pid = JS_INVALID_REFERENCE;
        JsPropertyIdRef obj2_x_pid = JS_INVALID_REFERENCE;
        JsPropertyIdRef obj2_y_pid = JS_INVALID_REFERENCE;
        JsPropertyIdRef obj2_z_pid = JS_INVALID_REFERENCE;
        JsValueRef obj1_a_value = JS_INVALID_REFERENCE;
        JsValueRef obj1_b_value = JS_INVALID_REFERENCE;
        JsValueRef obj2_x_value = JS_INVALID_REFERENCE;
        JsValueRef obj2_y_value = JS_INVALID_REFERENCE;
        JsValueRef obj2_z_value = JS_INVALID_REFERENCE;

        // var obj1 = {a : "obj1.a", b : "obj1.b"};
        // var obj2 = {x : "obj2.x", y : "obj2.y", z : "obj2.z"}
        REQUIRE(JsCreateObject(&proto) == JsNoError);
        REQUIRE(JsCreateExternalObject((void *)0xdeadbeef, ExternalObjectFinalizeCallback, &object1) == JsNoError);
        REQUIRE(JsCreateExternalObject((void *)0xdeadbeef, ExternalObjectFinalizeCallback, &object2) == JsNoError);

        size_t propNameLength = wcslen(_u("obj1.a"));
        REQUIRE(JsPointerToString(_u("obj1.a"), propNameLength, &obj1_a_value) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("a"), &obj1_a_pid) == JsNoError);
        REQUIRE(JsSetProperty(object1, obj1_a_pid, obj1_a_value, true) == JsNoError);

        REQUIRE(JsPointerToString(_u("obj1.b"), propNameLength, &obj1_b_value) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("b"), &obj1_b_pid) == JsNoError);
        REQUIRE(JsSetProperty(object1, obj1_b_pid, obj1_b_value, true) == JsNoError);

        REQUIRE(JsPointerToString(_u("obj2.x"), propNameLength, &obj2_x_value) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("x"), &obj2_x_pid) == JsNoError);
        REQUIRE(JsSetProperty(object2, obj2_x_pid, obj2_x_value, true) == JsNoError);

        REQUIRE(JsPointerToString(_u("obj1.y"), propNameLength, &obj2_y_value) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("y"), &obj2_y_pid) == JsNoError);
        REQUIRE(JsSetProperty(object2, obj2_y_pid, obj2_y_value, true) == JsNoError);

        REQUIRE(JsPointerToString(_u("obj1.z"), propNameLength, &obj2_z_value) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("z"), &obj2_z_pid) == JsNoError);
        REQUIRE(JsSetProperty(object2, obj2_z_pid, obj2_z_value, true) == JsNoError);


        REQUIRE(JsSetPrototype(object1, proto) == JsNoError);
        REQUIRE(JsSetPrototype(object2, proto) == JsNoError);

        JsValueRef objectProto = JS_INVALID_REFERENCE;

        REQUIRE(JsGetPrototype(object1, &objectProto) == JsNoError);
        CHECK(proto == objectProto);
        REQUIRE(JsGetPrototype(object2, &objectProto) == JsNoError);
        CHECK(proto == objectProto);

        JsValueRef value = JS_INVALID_REFERENCE;
        REQUIRE(JsGetProperty(object1, obj1_a_pid, &value) == JsNoError);
        CHECK(value == obj1_a_value);

        REQUIRE(JsGetProperty(object1, obj1_b_pid, &value) == JsNoError);
        CHECK(value == obj1_b_value);

        REQUIRE(JsGetProperty(object2, obj2_x_pid, &value) == JsNoError);
        CHECK(value == obj2_x_value);

        REQUIRE(JsGetProperty(object2, obj2_y_pid, &value) == JsNoError);
        CHECK(value == obj2_y_value);

        REQUIRE(JsGetProperty(object2, obj2_z_pid, &value) == JsNoError);
        CHECK(value == obj2_z_value);
    }

    TEST_CASE("ApiTest_SetPrototypeTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::SetPrototypeTest);
    }

    void DisableEval(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef result = JS_INVALID_REFERENCE;
        JsErrorCode error = JsRunScript(_u("eval(\"1 + 2\")"), JS_SOURCE_CONTEXT_NONE, _u(""), &result);

        if (!(attributes & JsRuntimeAttributeDisableEval))
        {
            CHECK(error == JsNoError);
        }
        else
        {
            CHECK(error == JsErrorScriptEvalDisabled);
        }

        error = JsRunScript(_u("new Function(\"return 1 + 2\")"), JS_SOURCE_CONTEXT_NONE, _u(""), &result);

        if (!(attributes & JsRuntimeAttributeDisableEval))
        {
            CHECK(error == JsNoError);
        }
        else
        {
            CHECK(error == JsErrorScriptEvalDisabled);
        }
    }

    TEST_CASE("ApiTest_DisableEval", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::DisableEval);
    }

    static void CALLBACK PromiseContinuationCallback(JsValueRef task, void *callbackState)
    {
        CHECK(callbackState != nullptr);

        // This simply saves the given task into the callback state
        // so that we can verify it in the test
        CHECK(*(JsValueRef *)callbackState == JS_INVALID_REFERENCE);
        *(JsValueRef *)callbackState = task;
    }

    void PromisesTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef result = JS_INVALID_REFERENCE;
        JsValueType valueType;
        JsValueRef task = JS_INVALID_REFERENCE;
        JsValueRef callback = JS_INVALID_REFERENCE;
        REQUIRE(JsSetPromiseContinuationCallback(PromiseContinuationCallback, &callback) == JsNoError);
        REQUIRE(JsRunScript(
            _u("new Promise(") \
            _u("  function(resolve, reject) {") \
            _u("    resolve('basic:success');") \
            _u("  }") \
            _u(").then (") \
            _u("  function () { return new Promise(") \
            _u("    function(resolve, reject) { ") \
            _u("      resolve('second:success'); ") \
            _u("    })") \
            _u("  }") \
            _u(");"), JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsNoError);
        CHECK(callback != nullptr);

        JsValueRef args[] = { GetUndefined() };

        // first then handler was queued
        task = callback;
        callback = JS_INVALID_REFERENCE;
        REQUIRE(JsGetValueType(task, &valueType) == JsNoError);
        CHECK(valueType == JsFunction);
        REQUIRE(JsCallFunction(task, args, _countof(args), &result) == JsNoError);

        // the second promise resolution was queued
        task = callback;
        callback = JS_INVALID_REFERENCE;
        REQUIRE(JsGetValueType(task, &valueType) == JsNoError);
        CHECK(valueType == JsFunction);
        REQUIRE(JsCallFunction(task, args, _countof(args), &result) == JsNoError);

        // second then handler was queued.
        task = callback;
        callback = JS_INVALID_REFERENCE;
        REQUIRE(JsGetValueType(task, &valueType) == JsNoError);
        CHECK(valueType == JsFunction);
        REQUIRE(JsCallFunction(task, args, _countof(args), &result) == JsNoError);

        // we are done; no more new task are queue.
        CHECK(callback == JS_INVALID_REFERENCE);
    }

    TEST_CASE("ApiTest_PromisesTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::PromisesTest);
    }

    void UnsetPromiseContinuation(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef result = JS_INVALID_REFERENCE, callbackState = JS_INVALID_REFERENCE, exception = JS_INVALID_REFERENCE;
        JsValueType cbStateType = JsUndefined;
        const wchar_t *script = _u("new Promise((res, rej) => res()).then(() => 1)");

        // script with no promise continuation callback should error
        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsErrorScriptException);
        CHECK(result == JS_INVALID_REFERENCE);
        REQUIRE(JsGetAndClearException(&exception) == JsNoError);

        // script with promise continuation callback should run successfully
        result = JS_INVALID_REFERENCE;
        callbackState = JS_INVALID_REFERENCE;
        REQUIRE(JsSetPromiseContinuationCallback(PromiseContinuationCallback, &callbackState) == JsNoError);
        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsNoError);
        CHECK(result != JS_INVALID_REFERENCE);
        REQUIRE(JsGetValueType(callbackState, &cbStateType) == JsNoError);
        CHECK(cbStateType == JsFunction);

        // unsetting the promise continuation callback should make promise scripts error
        result = JS_INVALID_REFERENCE;
        callbackState = JS_INVALID_REFERENCE;
        REQUIRE(JsSetPromiseContinuationCallback(nullptr, nullptr) == JsNoError);
        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsErrorScriptException);
        CHECK(result == JS_INVALID_REFERENCE);
        REQUIRE(JsGetAndClearException(&exception) == JsNoError);

        // resetting promise continuation callback should run successfully
        result = JS_INVALID_REFERENCE;
        callbackState = JS_INVALID_REFERENCE;
        REQUIRE(JsSetPromiseContinuationCallback(PromiseContinuationCallback, &callbackState) == JsNoError);
        REQUIRE(JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsNoError);
        CHECK(result != JS_INVALID_REFERENCE);
        REQUIRE(JsGetValueType(callbackState, &cbStateType) == JsNoError);
        CHECK(cbStateType == JsFunction);
    }

    TEST_CASE("ApiTest_UnsetPromiseContinuation", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::UnsetPromiseContinuation);
    }

    void ArrayBufferTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        for (int type = JsArrayTypeInt8; type <= JsArrayTypeFloat64; type++)
        {
            unsigned int size = 0;

            switch (type)
            {
            case JsArrayTypeInt16:
                size = sizeof(__int16);
                break;
            case JsArrayTypeInt8:
                size = sizeof(__int8);
                break;
            case JsArrayTypeUint8:
            case JsArrayTypeUint8Clamped:
                size = sizeof(unsigned __int8);
                break;
            case JsArrayTypeUint16:
                size = sizeof(unsigned __int16);
                break;
            case JsArrayTypeInt32:
                size = sizeof(__int32);
                break;
            case JsArrayTypeUint32:
                size = sizeof(unsigned __int32);
                break;
            case JsArrayTypeFloat32:
                size = sizeof(float);
                break;
            case JsArrayTypeFloat64:
                size = sizeof(double);
                break;
            }

            // ArrayBuffer
            JsValueRef arrayBuffer = JS_INVALID_REFERENCE;
            JsValueType valueType;
            BYTE *originBuffer = nullptr;
            unsigned int originBufferLength;

            REQUIRE(JsCreateArrayBuffer(16 * size, &arrayBuffer) == JsNoError);
            REQUIRE(JsGetValueType(arrayBuffer, &valueType) == JsNoError);
            CHECK(JsValueType::JsArrayBuffer == valueType);

            REQUIRE(JsGetArrayBufferStorage(arrayBuffer, &originBuffer, &originBufferLength) == JsNoError);
            CHECK(16 * size == originBufferLength);

            // TypedArray
            JsValueRef typedArray = JS_INVALID_REFERENCE;

            REQUIRE(JsCreateTypedArray((JsTypedArrayType)type, arrayBuffer, /*byteOffset*/size, /*length*/12, &typedArray) == JsNoError);
            REQUIRE(JsGetValueType(typedArray, &valueType) == JsNoError);
            CHECK(JsValueType::JsTypedArray == valueType);

            JsTypedArrayType arrayType;
            JsValueRef tmpArrayBuffer = JS_INVALID_REFERENCE;
            unsigned int tmpByteOffset, tmpByteLength;
            REQUIRE(JsGetTypedArrayInfo(typedArray, &arrayType, &tmpArrayBuffer, &tmpByteOffset, &tmpByteLength) == JsNoError);
            CHECK(type == arrayType);
            CHECK(arrayBuffer == tmpArrayBuffer);
            CHECK(size == tmpByteOffset);
            CHECK(12 * size == tmpByteLength);

            BYTE *buffer = nullptr;
            unsigned int bufferLength;
            int elementSize;
            REQUIRE(JsGetTypedArrayStorage(typedArray, &buffer, &bufferLength, &arrayType, &elementSize) == JsNoError);
            CHECK(originBuffer + size == buffer);
            CHECK(12 * size == bufferLength);
            CHECK(type == arrayType);
            CHECK(size == (size_t)elementSize);

            // DataView
            JsValueRef dataView = JS_INVALID_REFERENCE;

            REQUIRE(JsCreateDataView(arrayBuffer, /*byteOffset*/3, /*byteLength*/13, &dataView) == JsNoError);
            REQUIRE(JsGetValueType(dataView, &valueType) == JsNoError);
            CHECK(JsValueType::JsDataView == valueType);

            REQUIRE(JsGetDataViewStorage(dataView, &buffer, &bufferLength) == JsNoError);
            CHECK(originBuffer + 3 == buffer);
            CHECK(13 == (int)bufferLength);

            // InvalidArgs Get...
            JsValueRef bad = JS_INVALID_REFERENCE;
            REQUIRE(JsIntToNumber(5, &bad) == JsNoError);

            REQUIRE(JsGetArrayBufferStorage(typedArray, &buffer, &bufferLength) == JsErrorInvalidArgument);
            REQUIRE(JsGetArrayBufferStorage(dataView, &buffer, &bufferLength) == JsErrorInvalidArgument);
            REQUIRE(JsGetArrayBufferStorage(bad, &buffer, &bufferLength) == JsErrorInvalidArgument);

            REQUIRE(JsGetTypedArrayStorage(arrayBuffer, &buffer, &bufferLength, &arrayType, &elementSize) == JsErrorInvalidArgument);
            REQUIRE(JsGetTypedArrayStorage(dataView, &buffer, &bufferLength, &arrayType, &elementSize) == JsErrorInvalidArgument);
            REQUIRE(JsGetTypedArrayStorage(bad, &buffer, &bufferLength, &arrayType, &elementSize) == JsErrorInvalidArgument);

            REQUIRE(JsGetDataViewStorage(arrayBuffer, &buffer, &bufferLength) == JsErrorInvalidArgument);
            REQUIRE(JsGetDataViewStorage(typedArray, &buffer, &bufferLength) == JsErrorInvalidArgument);
            REQUIRE(JsGetDataViewStorage(bad, &buffer, &bufferLength) == JsErrorInvalidArgument);

            // no base array
            REQUIRE(JsCreateTypedArray((JsTypedArrayType)type, JS_INVALID_REFERENCE, /*byteOffset*/0, /*length*/0, &typedArray) == JsNoError); // no base array
            REQUIRE(JsGetTypedArrayInfo(typedArray, &arrayType, &tmpArrayBuffer, &tmpByteOffset, &tmpByteLength) == JsNoError);
            CHECK(type == arrayType);
            CHECK(tmpArrayBuffer != nullptr);
            CHECK(tmpByteOffset == 0);
            CHECK(tmpByteLength == 0);

            // InvalidArgs Create...
            REQUIRE(JsCreateTypedArray((JsTypedArrayType)(type + 100), arrayBuffer, /*byteOffset*/size, /*length*/12, &typedArray) == JsErrorInvalidArgument); // bad array type
            REQUIRE(JsCreateTypedArray((JsTypedArrayType)type, JS_INVALID_REFERENCE, /*byteOffset*/size, /*length*/12, &typedArray) == JsErrorInvalidArgument);  // byteOffset should be 0
            REQUIRE(JsCreateTypedArray((JsTypedArrayType)type, dataView, /*byteOffset*/size, /*length*/12, &typedArray) == JsErrorInvalidArgument);              // byteOffset should be 0
            REQUIRE(JsCreateTypedArray((JsTypedArrayType)type, bad, /*byteOffset*/size, /*length*/12, &typedArray) == JsErrorInvalidArgument);                   // byteOffset should be 0
            REQUIRE(JsCreateTypedArray((JsTypedArrayType)type, dataView, /*byteOffset*/0, /*length*/12, &typedArray) == JsErrorInvalidArgument); // length should be 0
            REQUIRE(JsCreateTypedArray((JsTypedArrayType)type, bad, /*byteOffset*/0, /*length*/12, &typedArray) == JsErrorInvalidArgument);      // length should be 0
            REQUIRE(JsCreateDataView(typedArray, /*byteOffset*/size, /*byteLength*/12, &dataView) == JsErrorInvalidArgument);    // must from arrayBuffer
            REQUIRE(JsCreateDataView(dataView, /*byteOffset*/size, /*byteLength*/12, &dataView) == JsErrorInvalidArgument);      // must from arrayBuffer
            REQUIRE(JsCreateDataView(bad, /*byteOffset*/size, /*byteLength*/12, &dataView) == JsErrorInvalidArgument);           // must from arrayBuffer
        }
    }

    TEST_CASE("ApiTest_ArrayBufferTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ArrayBufferTest);
    }

    struct ThreadArgsData
    {
        JsRuntimeHandle runtime;
        HANDLE hMonitor;
        BOOL isScriptActive;
        JsErrorCode disableExecutionResult;

        static const int waitTime = 1000;

        void BeginScriptExecution() { isScriptActive = true; }
        void EndScriptExecution() { isScriptActive = false; }
        void SignalMonitor() { SetEvent(hMonitor); }

        // CATCH is not thread-safe. Call this in main thread only.
        void CheckDisableExecutionResult()
        {
            REQUIRE(disableExecutionResult == JsNoError);
        }

        unsigned int ThreadProc()
        {
            while (1)
            {
                WaitForSingleObject(hMonitor, INFINITE);
                // TODO: have a generic stopping mechanism.
                if (isScriptActive)
                {
                    Sleep(waitTime);

                    // CATCH is not thread-safe. Do not verify in this thread.
                    disableExecutionResult = JsDisableRuntimeExecution(runtime);
                    if (disableExecutionResult == JsNoError)
                    {
                        continue;  // done, wait for next signal
                    }
                }

                CloseHandle(hMonitor);
                break;
            }
            return 0;
        }
    };

    static unsigned int  CALLBACK StaticThreadProc(LPVOID lpParameter)
    {
        DWORD ret = (DWORD)-1;
        ThreadArgsData * args = (ThreadArgsData *)lpParameter;
        ret = args->ThreadProc();
        return ret;
    }

#define TERMINATION_TESTS \
        _u("for (i=0; i<200; i = 20) {")              \
        _u("    var a = new Int8Array(800);")         \
        _u("}"),                                      \
                                                      \
        _u("function nextFunc() { ")                  \
        _u("    throw 'hello'")                       \
        _u("};")                                      \
        _u("for (i=0; i<200; i = 20)  { ")            \
        _u("    try {")                               \
        _u("        nextFunc();")                     \
        _u("    } ")                                  \
        _u("    catch(e) {}")                         \
        _u("}"),                                      \
                                                      \
        _u("function nextFunc() {")                   \
        _u("    bar = bar + nextFunc.toString();")    \
        _u("};")                                      \
        _u("bar = '';")                               \
        _u("for (i=0; i<200; i = 20) {")              \
        _u("    nextFunc()")                          \
        _u("}"),                                      \
                                                      \
        _u("while(1);"),                              \
                                                      \
        _u("function foo(){}")                        \
        _u("do{")                                     \
        _u("    foo();")                              \
        _u("}while(1);"),                             \
                                                      \
        _u("(function foo(){")                        \
        _u("    do {")                                \
        _u("        if (foo) continue;")              \
        _u("        if (!foo) break;")                \
        _u("    } while(1); ")                        \
        _u("})();"),                                  \
                                                      \
        _u("(function foo(a){")                       \
        _u("    while (a){")                          \
        _u(" L1:")                                    \
        _u("        do {")                            \
        _u("            while(1) {")                  \
        _u("                continue L1;")            \
        _u("            }")                           \
        _u("            a = 0;")                      \
        _u("        } while(0);")                     \
        _u("    }")                                   \
        _u("})(1);"),                                 \
                                                      \
        _u("(function (){")                           \
        _u("    while (1) {")                         \
        _u("        try {")                           \
        _u("            throw 0;")                    \
        _u("            break;")                      \
        _u("        }")                               \
        _u("        catch(e) {")                      \
        _u("            if (!e) continue;")           \
        _u("        }")                               \
        _u("        break;")                          \
        _u("    }")                                   \
        _u("})();")

    static const LPCWSTR terminationTests[] = { TERMINATION_TESTS };

    void ExternalFunctionWithScriptAbortionTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        if (!(attributes & JsRuntimeAttributeAllowScriptInterrupt))
        {
            REQUIRE(JsDisableRuntimeExecution(runtime) == JsErrorCannotDisableExecution);
            return;
        }

        ThreadArgsData threadArgs = {};
        threadArgs.runtime = runtime;
        threadArgs.hMonitor = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        HANDLE threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &StaticThreadProc, &threadArgs, 0, nullptr));
        REQUIRE(threadHandle != nullptr);
        if (threadHandle == nullptr)
        {
            // This is to satisfy preFAST, above REQUIRE call ensuring that it will report exception when threadHandle is null.
            return;
        }
        JsValueRef preScriptAbortFunction = JS_INVALID_REFERENCE, postScriptAbortFunction = JS_INVALID_REFERENCE;
        JsValueRef exception = JS_INVALID_REFERENCE;

        for (int i = 0; i < _countof(terminationTests); i++)
        {
            threadArgs.BeginScriptExecution();
            threadArgs.SignalMonitor();
            REQUIRE(JsCreateFunction(ExternalFunctionPreScriptAbortionCallback, nullptr, &preScriptAbortFunction) == JsNoError);
            REQUIRE(JsCreateFunction(ExternalFunctionPostScriptAbortionCallback, nullptr, &postScriptAbortFunction) == JsNoError);
            JsValueRef scriptTextArg = JS_INVALID_REFERENCE;

            WCHAR *scriptText = const_cast<WCHAR *>(terminationTests[i]);
            REQUIRE(JsPointerToString(scriptText, wcslen(scriptText), &scriptTextArg) == JsNoError);
            JsValueRef args[] = { scriptTextArg };

            REQUIRE(JsCallFunction(preScriptAbortFunction, args, 1, nullptr) == JsErrorScriptTerminated);

            bool isDisabled;
            REQUIRE(JsIsRuntimeExecutionDisabled(runtime, &isDisabled) == JsNoError);
            CHECK(isDisabled);

#ifdef NTBUILD
            REQUIRE(JsCallFunction(postScriptAbortFunction, args, 1, nullptr) == JsErrorInDisabledState);
#else // !JSRT_VERIFY_RUNTIME_STATE
            bool hasException = false;
            REQUIRE(JsHasException(&hasException) == JsErrorInDisabledState);
#endif
            REQUIRE(JsGetAndClearException(&exception) == JsErrorInDisabledState);
            REQUIRE(JsEnableRuntimeExecution(runtime) == JsNoError);
            threadArgs.CheckDisableExecutionResult();
            threadArgs.EndScriptExecution();
        }
        threadArgs.SignalMonitor();
        WaitForSingleObject(threadHandle, INFINITE);
        threadArgs.hMonitor = nullptr;
    }

    TEST_CASE("ApiTest_ExternalFunctionWithScriptAbortionTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ExternalFunctionWithScriptAbortionTest);
    }

    void ScriptTerminationTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        // can't interrupt if scriptinterrupt is disabled.
        if (!(attributes & JsRuntimeAttributeAllowScriptInterrupt))
        {
            REQUIRE(JsDisableRuntimeExecution(runtime) == JsErrorCannotDisableExecution);
            return;
        }
        ThreadArgsData threadArgs = {};
        threadArgs.runtime = runtime;
        threadArgs.hMonitor = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        HANDLE threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &StaticThreadProc, &threadArgs, 0, nullptr));
        REQUIRE(threadHandle != nullptr);
        if (threadHandle == nullptr)
        {
            // This is to satisfy preFAST, above REQUIRE call ensuring that it will report exception when threadHandle is null.
            return;
        }

        JsValueRef result;
        JsValueRef exception;
        for (int i = 0; i < _countof(terminationTests); i++)
        {
            threadArgs.BeginScriptExecution();
            threadArgs.SignalMonitor();
            REQUIRE(JsRunScript(terminationTests[i], JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsErrorScriptTerminated);
            bool isDisabled;
            REQUIRE(JsIsRuntimeExecutionDisabled(runtime, &isDisabled) == JsNoError);
            CHECK(isDisabled);
#ifdef NTBUILD
            REQUIRE(JsRunScript(terminationTests[i], JS_SOURCE_CONTEXT_NONE, _u(""), &result) == JsErrorInDisabledState);
#else // !JSRT_VERIFY_RUNTIME_STATE
            bool hasException = false;
            REQUIRE(JsHasException(&hasException) == JsErrorInDisabledState);
#endif
            REQUIRE(JsGetAndClearException(&exception) == JsErrorInDisabledState);
            REQUIRE(JsEnableRuntimeExecution(runtime) == JsNoError);
            threadArgs.CheckDisableExecutionResult();
            threadArgs.EndScriptExecution();
        }
        threadArgs.SignalMonitor();
        WaitForSingleObject(threadHandle, INFINITE);
        threadArgs.hMonitor = nullptr;
    }

    TEST_CASE("ApiTest_ScriptTerminationTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ScriptTerminationTest);
    }

    struct ModuleResponseData
    {
        ModuleResponseData()
            : mainModule(JS_INVALID_REFERENCE), childModule(JS_INVALID_REFERENCE), mainModuleException(JS_INVALID_REFERENCE), mainModuleReady(false)
        {
        }
        JsModuleRecord mainModule;
        JsModuleRecord childModule;
        JsValueRef mainModuleException;
        bool mainModuleReady;
    };
    ModuleResponseData successTest;

    static JsErrorCode CALLBACK Success_FIMC(_In_ JsModuleRecord referencingModule, _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord)
    {
        JsModuleRecord moduleRecord = JS_INVALID_REFERENCE;
        LPCWSTR specifierStr;
        size_t length;

        JsErrorCode errorCode = JsStringToPointer(specifier, &specifierStr, &length);
        REQUIRE(errorCode == JsNoError);
        REQUIRE(!wcscmp(specifierStr, _u("foo.js")));

        errorCode = JsInitializeModuleRecord(referencingModule, specifier, &moduleRecord);
        REQUIRE(errorCode == JsNoError);
        *dependentModuleRecord = moduleRecord;
        successTest.childModule = moduleRecord;
        return JsNoError;
    }

    static JsErrorCode CALLBACK Succes_NMRC(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef exceptionVar)
    {
        if (successTest.mainModule == referencingModule)
        {
            successTest.mainModuleReady = true;
            successTest.mainModuleException = exceptionVar;
        }
        return JsNoError;
    }

    void ModuleSuccessTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsModuleRecord requestModule = JS_INVALID_REFERENCE;
        JsValueRef specifier;

        REQUIRE(JsPointerToString(_u(""), 1, &specifier) == JsNoError);
        REQUIRE(JsInitializeModuleRecord(nullptr, specifier, &requestModule) == JsNoError);
        successTest.mainModule = requestModule;
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_FetchImportedModuleCallback, Success_FIMC) == JsNoError);
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_FetchImportedModuleFromScriptCallback, Success_FIMC) == JsNoError);
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_NotifyModuleReadyCallback, Succes_NMRC) == JsNoError);

        JsValueRef errorObject = JS_INVALID_REFERENCE;
        const char* fileContent = "import {x} from 'foo.js'";
        JsErrorCode errorCode = JsParseModuleSource(requestModule, 0, (LPBYTE)fileContent,
            (unsigned int)strlen(fileContent), JsParseModuleSourceFlags_DataIsUTF8, &errorObject);

        CHECK(errorCode == JsNoError);
        CHECK(errorObject == JS_INVALID_REFERENCE);
        CHECK(successTest.mainModuleReady == false);
        REQUIRE(successTest.childModule != JS_INVALID_REFERENCE);

        errorObject = JS_INVALID_REFERENCE;
        fileContent = "/*error code*/ var x x";

        errorCode = JsParseModuleSource(successTest.childModule, 1, (LPBYTE)fileContent,
            (unsigned int)strlen(fileContent), JsParseModuleSourceFlags_DataIsUTF8, &errorObject);

        CHECK(errorCode == JsErrorScriptCompile);
        CHECK(errorObject != JS_INVALID_REFERENCE);

        CHECK(successTest.mainModuleReady == true);
        REQUIRE(successTest.mainModuleException != JS_INVALID_REFERENCE);
        JsPropertyIdRef message = JS_INVALID_REFERENCE;

        REQUIRE(JsGetPropertyIdFromName(_u("message"), &message) == JsNoError);

        JsValueRef value1Check = JS_INVALID_REFERENCE;
        REQUIRE(JsGetProperty(successTest.mainModuleException, message, &value1Check) == JsNoError);

        JsValueRef asString = JS_INVALID_REFERENCE;
        REQUIRE(JsConvertValueToString(value1Check, &asString) == JsNoError);

        LPCWSTR str = nullptr;
        size_t length;
        REQUIRE(JsStringToPointer(asString, &str, &length) == JsNoError);
        REQUIRE(!wcscmp(str, _u("Expected ';'")));
    }

    TEST_CASE("ApiTest_ModuleSuccessTest", "[ApiTest]")
    {
        JsRTApiTest::WithSetup(JsRuntimeAttributeEnableExperimentalFeatures, ModuleSuccessTest);

    }

    ModuleResponseData reentrantParseData;
    static JsErrorCode CALLBACK ReentrantParse_FIMC(_In_ JsModuleRecord referencingModule, _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord)
    {
        JsModuleRecord moduleRecord = JS_INVALID_REFERENCE;
        LPCWSTR specifierStr;
        size_t length;

        JsErrorCode errorCode = JsStringToPointer(specifier, &specifierStr, &length);
        REQUIRE(!wcscmp(specifierStr, _u("foo.js")));

        REQUIRE(errorCode == JsNoError);
        errorCode = JsInitializeModuleRecord(referencingModule, specifier, &moduleRecord);
        REQUIRE(errorCode == JsNoError);
        *dependentModuleRecord = moduleRecord;
        reentrantParseData.childModule = moduleRecord;

        // directly make a call to parsemodulesource
        JsValueRef errorObject = JS_INVALID_REFERENCE;
        const char* fileContent = "/*error code*/ var x x";

        // Not checking the error code.
        JsParseModuleSource(moduleRecord, 1, (LPBYTE)fileContent,
            (unsigned int)strlen(fileContent), JsParseModuleSourceFlags_DataIsUTF8, &errorObject);

        // There must be an error
        CHECK(errorObject != JS_INVALID_REFERENCE);

        // Passed everything is valid.
        return JsNoError;
    }

    static JsErrorCode CALLBACK ReentrantParse_NMRC(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef exceptionVar)
    {
        if (reentrantParseData.mainModule == referencingModule)
        {
            reentrantParseData.mainModuleReady = true;
            reentrantParseData.mainModuleException = exceptionVar;
        }
        return JsNoError;
    }

    void ReentrantParseModuleTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsModuleRecord requestModule = JS_INVALID_REFERENCE;
        JsValueRef specifier;

        REQUIRE(JsPointerToString(_u(""), 1, &specifier) == JsNoError);
        REQUIRE(JsInitializeModuleRecord(nullptr, specifier, &requestModule) == JsNoError);
        reentrantParseData.mainModule = requestModule;
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_FetchImportedModuleCallback, ReentrantParse_FIMC) == JsNoError);
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_FetchImportedModuleFromScriptCallback, ReentrantParse_FIMC) == JsNoError);
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_NotifyModuleReadyCallback, ReentrantParse_NMRC) == JsNoError);

        JsValueRef errorObject = JS_INVALID_REFERENCE;
        const char* fileContent = "import {x} from 'foo.js'";
        JsParseModuleSource(requestModule, 0, (LPBYTE)fileContent,
            (unsigned int)strlen(fileContent), JsParseModuleSourceFlags_DataIsUTF8, &errorObject);

        CHECK(reentrantParseData.mainModuleReady == true);
        REQUIRE(reentrantParseData.mainModuleException != JS_INVALID_REFERENCE);

        JsPropertyIdRef message = JS_INVALID_REFERENCE;

        REQUIRE(JsGetPropertyIdFromName(_u("message"), &message) == JsNoError);

        JsValueRef value1Check = JS_INVALID_REFERENCE;
        REQUIRE(JsGetProperty(reentrantParseData.mainModuleException, message, &value1Check) == JsNoError);

        JsValueRef asString = JS_INVALID_REFERENCE;
        REQUIRE(JsConvertValueToString(value1Check, &asString) == JsNoError);

        LPCWSTR str = nullptr;
        size_t length;
        REQUIRE(JsStringToPointer(asString, &str, &length) == JsNoError);
        REQUIRE(!wcscmp(str, _u("Expected ';'")));
    }

    TEST_CASE("ApiTest_ReentrantParseModuleTest", "[ApiTest]")
    {
        JsRTApiTest::WithSetup(JsRuntimeAttributeEnableExperimentalFeatures, ReentrantParseModuleTest);
    }

    ModuleResponseData reentrantNoErrorParseData;
    static JsErrorCode CALLBACK reentrantNoErrorParse_FIMC(_In_ JsModuleRecord referencingModule, _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord)
    {
        JsModuleRecord moduleRecord = JS_INVALID_REFERENCE;
        LPCWSTR specifierStr;
        size_t length;

        JsErrorCode errorCode = JsStringToPointer(specifier, &specifierStr, &length);
        REQUIRE(!wcscmp(specifierStr, _u("foo.js")));

        REQUIRE(errorCode == JsNoError);
        errorCode = JsInitializeModuleRecord(referencingModule, specifier, &moduleRecord);
        REQUIRE(errorCode == JsNoError);
        *dependentModuleRecord = moduleRecord;
        reentrantNoErrorParseData.childModule = moduleRecord;

        JsValueRef errorObject = JS_INVALID_REFERENCE;
        const char* fileContent = "export var x = 10;";

        // Not checking the error code.
        JsParseModuleSource(moduleRecord, 1, (LPBYTE)fileContent,
            (unsigned int)strlen(fileContent), JsParseModuleSourceFlags_DataIsUTF8, &errorObject);

        // There must be an error
        CHECK(errorObject == JS_INVALID_REFERENCE);

        return JsNoError;
    }

    static JsErrorCode CALLBACK reentrantNoErrorParse_NMRC(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef exceptionVar)
    {
        if (reentrantNoErrorParseData.mainModule == referencingModule)
        {
            reentrantNoErrorParseData.mainModuleReady = true;
            reentrantNoErrorParseData.mainModuleException = exceptionVar;
        }
        return JsNoError;
    }

    void ReentrantNoErrorParseModuleTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsModuleRecord requestModule = JS_INVALID_REFERENCE;
        JsValueRef specifier;

        REQUIRE(JsPointerToString(_u(""), 1, &specifier) == JsNoError);
        REQUIRE(JsInitializeModuleRecord(nullptr, specifier, &requestModule) == JsNoError);
        reentrantNoErrorParseData.mainModule = requestModule;
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_FetchImportedModuleCallback, reentrantNoErrorParse_FIMC) == JsNoError);
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_FetchImportedModuleFromScriptCallback, reentrantNoErrorParse_FIMC) == JsNoError);
        REQUIRE(JsSetModuleHostInfo(requestModule, JsModuleHostInfo_NotifyModuleReadyCallback, reentrantNoErrorParse_NMRC) == JsNoError);

        JsValueRef errorObject = JS_INVALID_REFERENCE;
        const char* fileContent = "import {x} from 'foo.js'";
        JsErrorCode errorCode = JsParseModuleSource(requestModule, 0, (LPBYTE)fileContent,
            (unsigned int)strlen(fileContent), JsParseModuleSourceFlags_DataIsUTF8, &errorObject);

        // This is no error in this module parse.
        CHECK(errorCode == JsNoError);
        CHECK(errorObject == JS_INVALID_REFERENCE);
        CHECK(reentrantNoErrorParseData.mainModuleReady == true);
        REQUIRE(reentrantNoErrorParseData.mainModuleException == JS_INVALID_REFERENCE);
    }

    TEST_CASE("ApiTest_ReentrantNoErrorParseModuleTest", "[ApiTest]")
    {
        JsRTApiTest::WithSetup(JsRuntimeAttributeEnableExperimentalFeatures, ReentrantNoErrorParseModuleTest);
    }

    void ObjectHasOwnPropertyMethodTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        JsValueRef proto = JS_INVALID_REFERENCE;
        JsValueRef object = JS_INVALID_REFERENCE;

        REQUIRE(JsCreateObject(&proto) == JsNoError);
        REQUIRE(JsCreateObject(&object) == JsNoError);
        REQUIRE(JsSetPrototype(object, proto) == JsNoError);

        JsPropertyIdRef propertyIdFoo = JS_INVALID_REFERENCE;
        JsPropertyIdRef propertyIdBar = JS_INVALID_REFERENCE;
        bool hasProperty = false;

        REQUIRE(JsGetPropertyIdFromName(_u("foo"), &propertyIdFoo) == JsNoError);
        REQUIRE(JsGetPropertyIdFromName(_u("bar"), &propertyIdBar) == JsNoError);

        REQUIRE(JsSetProperty(object, propertyIdFoo, object, true) == JsNoError);
        REQUIRE(JsSetProperty(proto, propertyIdBar, object, true) == JsNoError);

        REQUIRE(JsHasProperty(object, propertyIdFoo, &hasProperty) == JsNoError);
        CHECK(hasProperty);

        REQUIRE(JsHasOwnProperty(object, propertyIdFoo, &hasProperty) == JsNoError);
        CHECK(hasProperty);

        REQUIRE(JsHasProperty(object, propertyIdBar, &hasProperty) == JsNoError);
        CHECK(hasProperty);

        REQUIRE(JsHasOwnProperty(object, propertyIdBar, &hasProperty) == JsNoError);
        CHECK(!hasProperty);
    }

    TEST_CASE("ApiTest_ObjectHasOwnPropertyMethodTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::ObjectHasOwnPropertyMethodTest);
    }

    void JsCopyStringOneByteMethodTest(JsRuntimeAttributes attributes, JsRuntimeHandle runtime)
    {
        size_t written = 0;
        char buf[10] = {0};
        JsValueRef value;
        REQUIRE(JsCreateStringUtf16(reinterpret_cast<const uint16_t*>(_u("0\x10\x80\xa9\uabcd\U000104377")), 8, &value) == JsNoError);
        REQUIRE(JsCopyStringOneByte(value, 0, -1, nullptr, &written) == JsNoError);
        CHECK(written == 8);
        buf[written] = '\xff';

        REQUIRE(JsCopyStringOneByte(value, 0, 10, buf, &written) == JsNoError);
        CHECK(written == 8);
        CHECK(buf[0] == '0');
        CHECK(buf[1] == '\x10');
        CHECK(buf[2] == '\x80');
        CHECK(buf[3] == '\xA9');
        CHECK(buf[4] == '\xcd');
        CHECK(buf[5] == '\x01');
        CHECK(buf[6] == '\x37');
        CHECK(buf[7] == '7');
        CHECK(buf[8] == '\xff');
    }

    TEST_CASE("ApiTest_JsCopyStringOneByteMethodTest", "[ApiTest]")
    {
        JsRTApiTest::RunWithAttributes(JsRTApiTest::JsCopyStringOneByteMethodTest);
    }

}

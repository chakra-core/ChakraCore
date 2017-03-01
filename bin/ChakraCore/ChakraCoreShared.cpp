//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Runtime.h"
#include "Core/ConfigParser.h"
#include "JsrtContext.h"
#include "TestHooks.h"

#ifdef __APPLE__
// dummy usage of some Jsrt to force export Jsrt on dylib
#include "ChakraCore.h"
void DummyJSRTCall()
{
    JsRuntimeHandle *runtime;
    JsRuntimeAttributes attr;
    JsCreateRuntime(attr, nullptr, runtime);
    JsDiagStartDebugging(runtime, nullptr, nullptr);
    JsInitializeModuleRecord(nullptr, nullptr, nullptr);
}
#endif

__attribute__ ((visibility ("default")))
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hmod, DWORD dwReason, PVOID pvReserved)
{
    // xplat doesn't need DllMain is being called.
    // Instead, we handle attach / detach stuff internally. (see lib/Jsrt/JsrtHelper.cpp)
    // However, PAL/ch will end-up calling here in order to enable TestFlags tooling
#ifdef ENABLE_TEST_HOOKS
    if (dwReason == DLL_PROCESS_ATTACH) OnChakraCoreLoaded();
#endif
    return TRUE;
}

static_assert(__LINE__ == 35, "You shouldn't add anything to this file or ChakraCoreDllFunc.cpp. Please consider again!");

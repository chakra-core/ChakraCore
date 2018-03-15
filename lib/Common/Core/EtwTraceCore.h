//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef ENABLE_JS_ETW
#define PAIR(a,b) a ## b

#ifdef NTBUILD
#define GCETW(e, args)                          \
    if (IsMemProtectMode())                     \
    {                                           \
        PAIR(EventWriteMEMPROTECT_ ## e, args); \
    }                                           \
    else                                        \
    {                                           \
        PAIR(EventWriteJSCRIPT_ ## e, args);    \
    }

#define GCETW_INTERNAL(e, args)                 \
    if (IsMemProtectMode())                     \
    {                                           \
        PAIR(EventWriteMEMPROTECT_ ## e, args); \
    }                                           \
    else                                        \
    {                                           \
        PAIR(EventWriteJSCRIPT_ ## e, args);    \
    }

#define IS_GCETW_Enabled(e) \
    (IsMemProtectMode() ? EventEnabledMEMPROTECT_##e() : EventEnabledJSCRIPT_##e())

#define JS_ETW_INTERNAL(s) s
#define EDGE_ETW_INTERNAL(s) s
#else  // !NTBUILD
#define GCETW(e, args)                          \
    PAIR(EventWriteJSCRIPT_ ## e, args);

#define IS_GCETW_Enabled(e)  EventEnabledJSCRIPT_##e()

#define GCETW_INTERNAL(e, args)
#define JS_ETW_INTERNAL(s)
#define EDGE_ETW_INTERNAL(s)
#endif  // !NTBUILD

#define JS_ETW(s) s
#define IS_JS_ETW(s) s

#ifdef ENABLE_JS_LTTNG
#include "jscriptEtw.h"

#else
// C-style callback
extern "C" {
    void EtwCallback(
        ULONG controlCode,
        PVOID callbackContext);
}

class EtwCallbackApi
{
public:
    static void OnSessionChange(ULONG controlCode, PVOID callbackContext);
};

// The macro 'MCGEN_PRIVATE_ENABLE_CALLBACK_V2' below should be defined before microsoft-scripting-jscript9events.h
// is included. This is a complier assert to ensure this happens. If this does not happen rundown events will not work.
#ifdef JSCRIPT_METHOD_METHODLOAD_OPCODE
CompileAssert(false)
#endif

// Declaring this macro enables the registration of the callback on DLL attach
#define MCGEN_PRIVATE_ENABLE_CALLBACK_V2(SourceId, ControlCode, Level, MatchAnyKeyword, MatchAllKeyword, FilterData, CallbackContext) \
       EtwCallback(ControlCode, CallbackContext)

// Work-around for a bug in the instrumentationevents generator
#pragma prefast(push)
#pragma prefast(disable:__WARNING_USING_UNINIT_VAR, "The ETW data generated from the manifest includes a default null function which uses unintialized memory.")

#include <Microsoft-Scripting-Chakra-InstrumentationEvents.h>
#ifdef NTBUILD
#include <ieresp_mshtml.h>
#include <microsoft-scripting-jscript9.internalevents.h>
#endif

#pragma prefast(pop)

//
// Encapsulates base routines to initialize ETW tracing in the module
//
class EtwTraceCore
{
public:
    static void Register();
    static void UnRegister();

    static bool s_registered;
};
#endif // ENABLE_JS_LTTNG

#else
#define GCETW(e, ...)
#define IS_GCETW_Enabled(e)  false
#define JS_ETW(s)
#define IS_JS_ETW(s) (false)
#define GCETW_INTERNAL(e, args)
#define JS_ETW_INTERNAL(s)
#define EDGE_ETW_INTERNAL(s)
#endif

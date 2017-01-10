// -------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// -------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

.syntax unified
.thumb

.global C_FUNC(_ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz)
.global C_FUNC(_ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz)

NESTED_ENTRY _ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_PUSH {r0-r3}
    PROLOG_PUSH {r11}
    PROLOG_PUSH {lr}      // save volatile registers

    mov  r0, sp           // Pass the address of the function at the saved r0 in case it need to be boxed
    bl   C_FUNC(_ZN2Js13ScriptContext24ProfileModeDeferredParseEPPNS_14ScriptFunctionE)
    mov  r12, r0          // back up entry point

    EPILOG_POP {lr}
    EPILOG_POP {r11}
    EPILOG_POP {r0-r3}   // restore arguments and return address

    bx   r12             // jump (tail call) to new entryPoint
NESTED_END _ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT


NESTED_ENTRY _ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
    PROLOG_PUSH {r0-r3}
    PROLOG_PUSH {r11}
    PROLOG_PUSH {lr}     // save volatile registers

    bl C_FUNC(_ZN2Js13ScriptContext30ProfileModeDeferredDeserializeEPNS_14ScriptFunctionE)
    mov  r12, r0         // backup entry point

    EPILOG_POP {lr}
    EPILOG_POP {r11}
    EPILOG_POP {r0-r3}   // restore arguments and return address

    bx   r12             // jump (tail call) to new entryPoint
NESTED_END _ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

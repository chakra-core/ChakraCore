//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

extern "C"
{
#ifdef _M_IX86
    void __cdecl _chkstk(int);
#else
    void __cdecl __chkstk(int);
#endif
}

#ifdef _CONTROL_FLOW_GUARD
extern "C" PVOID __guard_check_icall_fptr;
#endif


namespace IR
{
enum JnHelperMethod
{
#define HELPERCALL(Name, Address, Attributes) Helper##Name,
#include "JnHelperMethodList.h"
#undef HELPERCALL

    JnHelperMethodCount

};

class HelperCallOpnd;

// Verify the table is read-only.
void CheckJnHelperTable(intptr_t const *table);

// Return address of the helper which can be intercepted by debugger wrapper.
intptr_t GetMethodAddress(ThreadContextInfo * context, HelperCallOpnd* opnd);

intptr_t GetNonTableMethodAddress(ThreadContextInfo * context, JnHelperMethod helperMethod);

// Returns the original address of the helper, this one is never the intercepted by debugger helper.
intptr_t GetMethodOriginalAddress(ThreadContextInfo * context, JnHelperMethod helperMethod);

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)

char16 const* GetMethodName(JnHelperMethod helperMethod);

#endif

} // namespace IR.

namespace HelperMethodAttributes
{

// [Flags]
enum HelperMethodAttribute : BYTE
{
    AttrNone        = 0x00,
    AttrCanThrow    = 0x01,     // Can throw non-OOM / non-SO exceptions. Under debugger / Fast F12, these helpers are wrapped with try-catch wrapper.
    AttrInVariant   = 0x02,     // The method is "const" method that can be hoisted.
};

bool CanThrow(IR::JnHelperMethod helper);

bool IsInVariant(IR::JnHelperMethod helper);

} // namespace HelperMethodAttributes.

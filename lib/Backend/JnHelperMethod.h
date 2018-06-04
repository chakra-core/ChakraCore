//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

extern "C"
{
#ifdef _M_IX86
    DECLSPEC_CHPE_GUEST void __cdecl _chkstk(int);
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

bool CanThrow(IR::JnHelperMethod helper);

bool IsInVariant(IR::JnHelperMethod helper);

bool CanBeReentrant(IR::JnHelperMethod helper);

} // namespace HelperMethodAttributes.

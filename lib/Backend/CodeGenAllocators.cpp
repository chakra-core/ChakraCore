//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

CodeGenAllocators::CodeGenAllocators(AllocationPolicyManager * policyManager, Js::ScriptContext * scriptContext, CustomHeap::CodePageAllocators * codePageAllocators, HANDLE processHandle)
: pageAllocator(policyManager, Js::Configuration::Global.flags, PageAllocatorType_BGJIT, 0)
, allocator(_u("NativeCode"), &pageAllocator, Js::Throw::OutOfMemory)
, emitBufferManager(&allocator, codePageAllocators, scriptContext, _u("JIT code buffer"), processHandle)
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
, canCreatePreReservedSegment(false)
#endif
{
}

CodeGenAllocators::~CodeGenAllocators()
{
}

#if DBG
void
CodeGenAllocators::ClearConcurrentThreadId()
{    
    this->pageAllocator.ClearConcurrentThreadId();
}
#endif

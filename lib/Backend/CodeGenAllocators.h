//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

template<typename TAlloc, typename TPreReservedAlloc>
class CodeGenAllocators
{
    // emitBufferManager depends on allocator which in turn depends on pageAllocator, make sure the sequence is right
private:
    PageAllocator pageAllocator;
    NoRecoverMemoryArenaAllocator  allocator;
public:
    EmitBufferManager<TAlloc, TPreReservedAlloc, CriticalSection> emitBufferManager;
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    bool canCreatePreReservedSegment;
#endif

    CodeGenAllocators(AllocationPolicyManager * policyManager, Js::ScriptContext * scriptContext, CustomHeap::CodePageAllocators<TAlloc, TPreReservedAlloc> * codePageAllocators, HANDLE processHandle);
    ~CodeGenAllocators();

#if DBG
    void ClearConcurrentThreadId();
#endif
};

typedef CodeGenAllocators<VirtualAllocWrapper, PreReservedVirtualAllocWrapper> InProcCodeGenAllocators;
#if ENABLE_OOP_NATIVE_CODEGEN
typedef CodeGenAllocators<SectionAllocWrapper, PreReservedSectionAllocWrapper> OOPCodeGenAllocators;
#endif

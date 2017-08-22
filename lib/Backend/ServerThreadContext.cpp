//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if ENABLE_OOP_NATIVE_CODEGEN
#include "JITServer/JITServer.h"

ServerThreadContext::ServerThreadContext(ThreadContextDataIDL * data, HANDLE processHandle) :
    m_autoProcessHandle(processHandle),
    m_threadContextData(*data),
    m_refCount(0),
    m_numericPropertyBV(nullptr),
    m_preReservedSectionAllocator(processHandle),
    m_sectionAllocator(processHandle),
    m_thunkPageAllocators(nullptr, /* allocXData */ false, &m_sectionAllocator, nullptr, processHandle),
    m_codePageAllocators(nullptr, ALLOC_XDATA, &m_sectionAllocator, &m_preReservedSectionAllocator, processHandle),
    m_codeGenAlloc(nullptr, nullptr, &m_codePageAllocators, processHandle),
#if defined(_CONTROL_FLOW_GUARD) && (_M_IX86 || _M_X64)
    m_jitThunkEmitter(this, &m_sectionAllocator, processHandle),
#endif
    m_pageAlloc(nullptr, Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
        AutoSystemInfo::Data.IsLowMemoryProcess() ?
        PageAllocator::DefaultLowMaxFreePageCount :
        PageAllocator::DefaultMaxFreePageCount
    )
{
    m_pid = GetProcessId(processHandle);

#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    m_codeGenAlloc.canCreatePreReservedSegment = data->allowPrereserveAlloc != FALSE;
#endif
    m_numericPropertyBV = HeapNew(BVSparse<HeapAllocator>, &HeapAllocator::Instance);
}

ServerThreadContext::~ServerThreadContext()
{
    if (this->m_numericPropertyBV != nullptr)
    {
        HeapDelete(m_numericPropertyBV);
        this->m_numericPropertyBV = nullptr;
    }

}

PreReservedSectionAllocWrapper *
ServerThreadContext::GetPreReservedSectionAllocator()
{
    return &m_preReservedSectionAllocator;
}

intptr_t
ServerThreadContext::GetBailOutRegisterSaveSpaceAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.bailOutRegisterSaveSpaceAddr);
}

ptrdiff_t
ServerThreadContext::GetChakraBaseAddressDifference() const
{
    return GetRuntimeChakraBaseAddress() - (intptr_t)AutoSystemInfo::Data.GetChakraBaseAddr();
}

ptrdiff_t
ServerThreadContext::GetCRTBaseAddressDifference() const
{
    return GetRuntimeCRTBaseAddress() - GetJITCRTBaseAddress();
}

intptr_t
ServerThreadContext::GetDisableImplicitFlagsAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.disableImplicitFlagsAddr);
}

intptr_t
ServerThreadContext::GetImplicitCallFlagsAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.implicitCallFlagsAddr);
}

#if defined(ENABLE_SIMDJS) && (defined(_M_IX86) || defined(_M_X64))
intptr_t
ServerThreadContext::GetSimdTempAreaAddr(uint8 tempIndex) const
{
    Assert(tempIndex < SIMD_TEMP_SIZE);
    return m_threadContextData.simdTempAreaBaseAddr + tempIndex * sizeof(_x86_SIMDValue);
}
#endif

intptr_t
ServerThreadContext::GetThreadStackLimitAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.threadStackLimitAddr);
}

size_t
ServerThreadContext::GetScriptStackLimit() const
{
    return static_cast<size_t>(m_threadContextData.scriptStackLimit);
}

bool
ServerThreadContext::IsThreadBound() const
{
    return m_threadContextData.isThreadBound != FALSE;
}

HANDLE
ServerThreadContext::GetProcessHandle() const
{
    return m_autoProcessHandle.GetHandle();
}

CustomHeap::OOPCodePageAllocators *
ServerThreadContext::GetThunkPageAllocators()
{
    return &m_thunkPageAllocators;
}

CustomHeap::OOPCodePageAllocators *
ServerThreadContext::GetCodePageAllocators()
{
    return &m_codePageAllocators;
}

SectionAllocWrapper *
ServerThreadContext::GetSectionAllocator()
{
    return &m_sectionAllocator;
}

OOPCodeGenAllocators *
ServerThreadContext::GetCodeGenAllocators()
{
    return &m_codeGenAlloc;
}

#if defined(_CONTROL_FLOW_GUARD) && (_M_IX86 || _M_X64)
OOPJITThunkEmitter *
ServerThreadContext::GetJITThunkEmitter()
{
    return &m_jitThunkEmitter;
}
#endif

intptr_t
ServerThreadContext::GetRuntimeChakraBaseAddress() const
{
    return static_cast<intptr_t>(m_threadContextData.chakraBaseAddress);
}

intptr_t
ServerThreadContext::GetRuntimeCRTBaseAddress() const
{
    return static_cast<intptr_t>(m_threadContextData.crtBaseAddress);
}

/* static */
intptr_t
ServerThreadContext::GetJITCRTBaseAddress()
{
    return (intptr_t)AutoSystemInfo::Data.GetCRTHandle();
}

PageAllocator *
ServerThreadContext::GetForegroundPageAllocator()
{
    return &m_pageAlloc;
}

bool
ServerThreadContext::IsNumericProperty(Js::PropertyId propertyId)
{
    if (propertyId >= 0 && Js::IsInternalPropertyId(propertyId))
    {
        return Js::InternalPropertyRecords::GetInternalPropertyName(propertyId)->IsNumeric();
    }

    bool found = false;
    {
        AutoCriticalSection lock(&m_cs);
        found = m_numericPropertyBV->Test(propertyId) != FALSE;
    }

    return found;
}

void
ServerThreadContext::UpdateNumericPropertyBV(BVSparseNode * newProps)
{
    AutoCriticalSection lock(&m_cs);
    m_numericPropertyBV->CopyFromNode(newProps);
}

void ServerThreadContext::AddRef()
{
    InterlockedExchangeAdd(&m_refCount, (uint)1);
}
void ServerThreadContext::Release()
{
    InterlockedExchangeSubtract(&m_refCount, (uint)1);
    if (m_isClosed && m_refCount == 0)
    {
        HeapDelete(this);
    }
}
void ServerThreadContext::Close()
{
    this->m_isClosed = true;
#ifdef STACK_BACK_TRACE
    ServerContextManager::RecordCloseContext(this);
#endif
}
#endif

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if ENABLE_OOP_NATIVE_CODEGEN
#include "JITServer/JITServer.h"

ServerThreadContext::ServerThreadContext(ThreadContextDataIDL* data, ProcessContext* processContext) :
    m_threadContextData(*data),
    m_refCount(0),
    m_numericPropertyBV(nullptr),
    m_preReservedSectionAllocator(processContext->processHandle),
    m_sectionAllocator(processContext->processHandle),
    m_codePageAllocators(nullptr, ALLOC_XDATA, &m_sectionAllocator, &m_preReservedSectionAllocator, processContext->processHandle),
    m_thunkPageAllocators(nullptr, /* allocXData */ false, &m_sectionAllocator, nullptr, processContext->processHandle),
#if defined(_CONTROL_FLOW_GUARD) && !defined(_M_ARM)
    m_jitThunkEmitter(this, &m_sectionAllocator, processContext->processHandle),
#endif
    m_pageAlloc(nullptr, Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
        AutoSystemInfo::Data.IsLowMemoryProcess() ?
        PageAllocator::DefaultLowMaxFreePageCount :
        PageAllocator::DefaultMaxFreePageCount
    ),
    processContext(processContext),
    m_canCreatePreReservedSegment(data->allowPrereserveAlloc != FALSE)
{
    m_pid = GetProcessId(processContext->processHandle);

    m_numericPropertyBV = HeapNew(BVSparse<HeapAllocator>, &HeapAllocator::Instance);
}

ServerThreadContext::~ServerThreadContext()
{
    processContext->Release();
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

#ifdef ENABLE_WASM_SIMD
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
    return this->processContext->processHandle;
}

CustomHeap::OOPCodePageAllocators *
ServerThreadContext::GetThunkPageAllocators()
{
    return &m_thunkPageAllocators;
}

SectionAllocWrapper *
ServerThreadContext::GetSectionAllocator()
{
    return &m_sectionAllocator;
}

CustomHeap::OOPCodePageAllocators *
ServerThreadContext::GetCodePageAllocators()
{
    return &m_codePageAllocators;
}

#if defined(_CONTROL_FLOW_GUARD) && !defined(_M_ARM)
OOPJITThunkEmitter *
ServerThreadContext::GetJITThunkEmitter()
{
    return &m_jitThunkEmitter;
}
#endif

intptr_t
ServerThreadContext::GetRuntimeChakraBaseAddress() const
{
    return this->processContext->chakraBaseAddress;
}

intptr_t
ServerThreadContext::GetRuntimeCRTBaseAddress() const
{
    return this->processContext->crtBaseAddress;
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
ServerThreadContext::CanCreatePreReservedSegment() const
{
    return m_canCreatePreReservedSegment;
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

ProcessContext::ProcessContext(HANDLE processHandle, intptr_t chakraBaseAddress, intptr_t crtBaseAddress) :
    processHandle(processHandle),
    chakraBaseAddress(chakraBaseAddress),
    crtBaseAddress(crtBaseAddress),
    refCount(0)
{
}
ProcessContext::~ProcessContext()
{
    CloseHandle(processHandle);
}

void ProcessContext::AddRef()
{
    InterlockedExchangeAdd(&this->refCount, 1);
}
void ProcessContext::Release()
{
    InterlockedExchangeSubtract(&this->refCount, 1);
}

bool ProcessContext::HasRef()
{
    return this->refCount != 0;
}

#endif

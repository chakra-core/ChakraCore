//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if ENABLE_OOP_NATIVE_CODEGEN
#include "JITServer/JITServer.h"
#endif //ENABLE_OOP_NATIVE_CODEGEN

ServerThreadContext::ServerThreadContext(ThreadContextDataIDL * data) :
    m_threadContextData(*data),
    m_refCount(0),
    m_numericPropertySet(nullptr),
    m_preReservedVirtualAllocator((HANDLE)data->processHandle),
    m_codePageAllocators(nullptr, ALLOC_XDATA, &m_preReservedVirtualAllocator, (HANDLE)data->processHandle),
#if DYNAMIC_INTERPRETER_THUNK || defined(ASMJS_PLAT)
    m_thunkPageAllocators(nullptr, /* allocXData */ false, /* virtualAllocator */ nullptr, (HANDLE)data->processHandle),
#endif
    m_codeGenAlloc(nullptr, nullptr, &m_codePageAllocators, (HANDLE)data->processHandle),
    m_pageAlloc(nullptr, Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
        AutoSystemInfo::Data.IsLowMemoryProcess() ?
        PageAllocator::DefaultLowMaxFreePageCount :
        PageAllocator::DefaultMaxFreePageCount
    ),
    m_jitCRTBaseAddress((intptr_t)GetModuleHandle(UCrtC99MathApis::LibraryName))
{
#if ENABLE_OOP_NATIVE_CODEGEN
    m_pid = GetProcessId((HANDLE)data->processHandle);
#endif

#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    m_codeGenAlloc.canCreatePreReservedSegment = data->allowPrereserveAlloc != FALSE;
#endif
    m_numericPropertySet = HeapNew(PropertySet, &HeapAllocator::Instance);
}

ServerThreadContext::~ServerThreadContext()
{
    // TODO: OOP JIT, clear out elements of map. maybe should arena alloc?
    if (this->m_numericPropertySet != nullptr)
    {
        HeapDelete(m_numericPropertySet);
        this->m_numericPropertySet = nullptr;
    }

}

PreReservedVirtualAllocWrapper *
ServerThreadContext::GetPreReservedVirtualAllocator()
{
    return &m_preReservedVirtualAllocator;
}

intptr_t
ServerThreadContext::GetBailOutRegisterSaveSpaceAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.bailOutRegisterSaveSpaceAddr);
}

ptrdiff_t
ServerThreadContext::GetChakraBaseAddressDifference() const
{
#if ENABLE_OOP_NATIVE_CODEGEN
    return GetRuntimeChakraBaseAddress() - (intptr_t)AutoSystemInfo::Data.GetChakraBaseAddr();
#else
    return 0;
#endif
}

ptrdiff_t
ServerThreadContext::GetCRTBaseAddressDifference() const
{
    return GetRuntimeCRTBaseAddress() - m_jitCRTBaseAddress;
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
    return reinterpret_cast<HANDLE>(m_threadContextData.processHandle);
}

#if DYNAMIC_INTERPRETER_THUNK || defined(ASMJS_PLAT)
CustomHeap::CodePageAllocators *
ServerThreadContext::GetThunkPageAllocators()
{
    return &m_thunkPageAllocators;
}
#endif

CustomHeap::CodePageAllocators *
ServerThreadContext::GetCodePageAllocators()
{
    return &m_codePageAllocators;
}

CodeGenAllocators *
ServerThreadContext::GetCodeGenAllocators()
{
    return &m_codeGenAlloc;
}

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

    m_numericPropertySet->LockResize();
    bool found = m_numericPropertySet->ContainsKey(propertyId);
    m_numericPropertySet->UnlockResize();

    return found;
}

void
ServerThreadContext::RemoveFromNumericPropertySet(Js::PropertyId reclaimedId)
{
    if (m_numericPropertySet->ContainsKey(reclaimedId))
    {
        // if there was reclaimed property that had its pid reused, delete the old property record
        m_numericPropertySet->Remove(reclaimedId);
    }
    else
    {
        // we should only ever ask to reclaim properties which were previously added to the jit map
        Assert(UNREACHED);
    }
}

void
ServerThreadContext::AddToNumericPropertySet(Js::PropertyId propId)
{
    // this should only happen if there was reclaimed property that we failed to add to reclaimed list due to a prior oom
    if (m_numericPropertySet->ContainsKey(propId))
    {
        m_numericPropertySet->Remove(propId);
    }

    m_numericPropertySet->Add(propId);
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

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

ServerThreadContext::ServerThreadContext(ThreadContextDataIDL * data) :
    m_threadContextData(*data),
    m_policyManager(true),
    m_propertyMap(nullptr),
    m_pageAllocs(&HeapAllocator::Instance),
    m_preReservedVirtualAllocator((HANDLE)data->processHandle),
    m_codePageAllocators(&m_policyManager, ALLOC_XDATA, &m_preReservedVirtualAllocator, (HANDLE)data->processHandle),
    m_codeGenAlloc(&m_policyManager, nullptr, &m_codePageAllocators, (HANDLE)data->processHandle),
    // TODO: OOP JIT, don't hardcode name
#ifdef NTBUILD
    m_jitChakraBaseAddress((intptr_t)GetModuleHandle(L"Chakra.dll")),
#else
    m_jitChakraBaseAddress((intptr_t)GetModuleHandle(L"ChakraCore.dll")),
#endif
    m_jitCRTBaseAddress((intptr_t)GetModuleHandle(UCrtC99MathApis::LibraryName))
{
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    m_codeGenAlloc.canCreatePreReservedSegment = data->allowPrereserveAlloc != FALSE;
#endif
    m_propertyMap = HeapNew(PropertyMap, &HeapAllocator::Instance, TotalNumberOfBuiltInProperties + 700);
}

ServerThreadContext::~ServerThreadContext()
{
    // TODO: OOP JIT, clear out elements of map. maybe should arena alloc?    
    if (this->m_propertyMap != nullptr)
    {
        this->m_propertyMap->Map([](const Js::PropertyRecord* record) 
        {
            size_t allocLength = record->byteCount + sizeof(char16) + (record->isNumeric ? sizeof(uint32) : 0);
            HeapDeletePlus(allocLength, const_cast<Js::PropertyRecord*>(record));
        });
        HeapDelete(m_propertyMap);
        this->m_propertyMap = nullptr;
    }
    this->m_pageAllocs.Map([](DWORD thread, PageAllocator* alloc)
    {
        HeapDelete(alloc);
    });
}

PreReservedVirtualAllocWrapper *
ServerThreadContext::GetPreReservedVirtualAllocator()
{
    return &m_preReservedVirtualAllocator;
}

PageAllocator*
ServerThreadContext::GetPageAllocator()
{
    PageAllocator * alloc;

    if (!m_pageAllocs.TryGetValue(GetCurrentThreadId(), &alloc))
    {
        alloc = HeapNew(PageAllocator,
            &m_policyManager,
            Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
            AutoSystemInfo::Data.IsLowMemoryProcess() ?
            PageAllocator::DefaultLowMaxFreePageCount :
            PageAllocator::DefaultMaxFreePageCount);

        m_pageAllocs.Add(GetCurrentThreadId(), alloc);
    }
    return alloc;
}


intptr_t
ServerThreadContext::GetBailOutRegisterSaveSpaceAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.bailOutRegisterSaveSpaceAddr);
}

intptr_t
ServerThreadContext::GetDebuggingFlagsAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.debuggingFlagsAddr);
}

intptr_t
ServerThreadContext::GetDebugStepTypeAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.debugStepTypeAddr);
}

intptr_t
ServerThreadContext::GetDebugFrameAddressAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.debugFrameAddressAddr);
}

intptr_t
ServerThreadContext::GetDebugScriptIdWhenSetAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.debugScriptIdWhenSetAddr);
}

ptrdiff_t
ServerThreadContext::GetChakraBaseAddressDifference() const
{
    return GetRuntimeChakraBaseAddress() - m_jitChakraBaseAddress;
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

#if defined(_M_IX86) || defined(_M_X64)
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

AllocationPolicyManager *
ServerThreadContext::GetAllocationPolicyManager()
{
    return &m_policyManager;
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

Js::PropertyRecord const *
ServerThreadContext::GetPropertyRecord(Js::PropertyId propertyId)
{
    if (propertyId >= 0 && Js::IsInternalPropertyId(propertyId))
    {
        return Js::InternalPropertyRecords::GetInternalPropertyName(propertyId);
    }

    const Js::PropertyRecord * propertyRecord = nullptr;
    m_propertyMap->LockResize();
    bool found = m_propertyMap->TryGetValue(propertyId, &propertyRecord);
    m_propertyMap->UnlockResize();

    AssertMsg(found && propertyRecord != nullptr, "using invalid propertyid");
    return propertyRecord;
}

void
ServerThreadContext::RemoveFromPropertyMap(Js::PropertyId reclaimedId)
{
    const Js::PropertyRecord * oldRecord = nullptr;
    if (m_propertyMap->TryGetValue(reclaimedId, &oldRecord))
    {
        // if there was reclaimed property that had its pid reused, delete the old property record
        m_propertyMap->Remove(oldRecord);

        PropertyRecordTrace(_u("Reclaimed JIT property '%s' at 0x%08x, pid = %d\n"), oldRecord->GetBuffer(), oldRecord, oldRecord->pid);

        size_t oldLength = oldRecord->byteCount + sizeof(char16) + (oldRecord->isNumeric ? sizeof(uint32) : 0);
        HeapDeletePlus(oldLength, const_cast<Js::PropertyRecord*>(oldRecord));
    }
    else
    {
        // we should only ever ask to reclaim properties which were previously added to the jit map
        Assert(UNREACHED);
    }
}

void
ServerThreadContext::AddToPropertyMap(const Js::PropertyRecord * origRecord)
{
    size_t allocLength = origRecord->byteCount + sizeof(char16) + (origRecord->isNumeric ? sizeof(uint32) : 0);
    Js::PropertyRecord * record = HeapNewPlus(allocLength, Js::PropertyRecord, origRecord->byteCount, origRecord->isNumeric, origRecord->hash, origRecord->isSymbol);
    record->isBound = origRecord->isBound;

    char16* buffer = (char16 *)(record + 1);
    js_memcpy_s(buffer, origRecord->byteCount, origRecord->GetBuffer(), origRecord->byteCount);

    buffer[record->GetLength()] = _u('\0');

    if (record->isNumeric)
    {
        *(uint32 *)(buffer + record->GetLength() + 1) = origRecord->GetNumericValue();
        Assert(record->GetNumericValue() == origRecord->GetNumericValue());
    }
    record->pid = origRecord->pid;

    const Js::PropertyRecord * oldRecord = nullptr;

    // this should only happen if there was reclaimed property that we failed to add to reclaimed list due to a prior oom
    if (m_propertyMap->TryGetValue(origRecord->GetPropertyId(), &oldRecord))
    {
        m_propertyMap->Remove(oldRecord);
        size_t oldLength = oldRecord->byteCount + sizeof(char16) + (oldRecord->isNumeric ? sizeof(uint32) : 0);
        HeapDeletePlus(oldLength, const_cast<Js::PropertyRecord*>(oldRecord));
    }

    m_propertyMap->Add(record);

    PropertyRecordTrace(_u("Added JIT property '%s' at 0x%08x, pid = %d\n"), record->GetBuffer(), record, record->pid);
}

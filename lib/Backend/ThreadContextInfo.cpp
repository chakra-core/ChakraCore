//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

ThreadContextInfo::ThreadContextInfo(ThreadContextData * data) :
    m_threadContextData(*data),
    m_policyManager(true),
    m_pageAlloc(&m_policyManager, Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
        AutoSystemInfo::Data.IsLowMemoryProcess() ?
            PageAllocator::DefaultLowMaxFreePageCount :
            PageAllocator::DefaultMaxFreePageCount),
    m_codeGenAlloc(&m_policyManager, nullptr, (HANDLE)data->processHandle),
    m_isAllJITCodeInPreReservedRegion(true)
{
}

intptr_t
ThreadContextInfo::GetNullFrameDisplayAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::NullFrameDisplay) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetStrictNullFrameDisplayAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::StrictNullFrameDisplay) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetAbsDoubleCstAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::AbsDoubleCst) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetAbsFloatCstAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::AbsFloatCst) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetMaskNegFloatAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::MaskNegFloat) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetMaskNegDoubleAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::MaskNegDouble) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetUIntConvertConstAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::UIntConvertConst) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetUint8ClampedArraySetItemAddr() const
{
    return reinterpret_cast<intptr_t>((BOOL(*)(Js::Uint8ClampedArray * arr, uint32 index, Js::Var value))&Js::Uint8ClampedArray::DirectSetItem) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetConstructorCacheDefaultInstanceAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::ConstructorCache::DefaultInstance) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetJavascriptObjectNewInstanceAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptObject::EntryInfo::NewInstance) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetDoubleOnePointZeroAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::ONE_POINT_ZERO) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetDoublePointFiveAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_PointFive) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetFloatPointFiveAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_Float32PointFive) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetDoubleNegPointFiveAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_NegPointFive) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetFloatNegPointFiveAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_Float32NegPointFive) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetDoubleTwoToFractionAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_TwoToFraction) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetFloatTwoToFractionAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_Float32TwoToFraction) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetDoubleNegTwoToFractionAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_NegTwoToFraction) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetFloatNegTwoToFractionAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_Float32NegTwoToFraction) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetDoubleZeroAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_Zero) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetFloatZeroAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNumber::k_Float32Zero) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetNativeFloatArrayMissingItemAddr() const
{
    return reinterpret_cast<intptr_t>(&Js::JavascriptNativeFloatArray::MissingItem) - GetBaseAddressDifference();
}

intptr_t
ThreadContextInfo::GetThreadStackLimitAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.threadStackLimitAddr);
}

size_t
ThreadContextInfo::GetScriptStackLimit() const
{
    return static_cast<size_t>(m_threadContextData.scriptStackLimit);
}

bool
ThreadContextInfo::IsThreadBound() const
{
    return m_threadContextData.isThreadBound != FALSE;
}

PageAllocator *
ThreadContextInfo::GetPageAllocator()
{
    return &m_pageAlloc;
}

CodeGenAllocators *
ThreadContextInfo::GetCodeGenAllocators()
{
    return &m_codeGenAlloc;
}

AllocationPolicyManager *
ThreadContextInfo::GetAllocationPolicyManager()
{
    return &m_policyManager;
}

HANDLE
ThreadContextInfo::GetProcessHandle() const
{
    return reinterpret_cast<HANDLE>(m_threadContextData.processHandle);
}

bool
ThreadContextInfo::IsAllJITCodeInPreReservedRegion() const
{
    return m_isAllJITCodeInPreReservedRegion;
}

void
ThreadContextInfo::ResetIsAllJITCodeInPreReservedRegion()
{
    m_isAllJITCodeInPreReservedRegion = false;
}

intptr_t
ThreadContextInfo::GetRuntimeChakraBaseAddress() const
{
    return static_cast<intptr_t>(m_threadContextData.chakraBaseAddress);
}

ptrdiff_t
ThreadContextInfo::GetBaseAddressDifference() const
{
    return m_localChakraBaseAddress - GetRuntimeChakraBaseAddress();
}

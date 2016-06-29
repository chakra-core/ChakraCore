//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

ThreadContextInfo::ThreadContextInfo(ThreadContextDataIDL * data) :
    m_threadContextData(*data),
    m_policyManager(true),
    m_pageAlloc(&m_policyManager, Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
        AutoSystemInfo::Data.IsLowMemoryProcess() ?
            PageAllocator::DefaultLowMaxFreePageCount :
            PageAllocator::DefaultMaxFreePageCount),
    m_codeGenAlloc(&m_policyManager, nullptr, (HANDLE)data->processHandle),
    m_isAllJITCodeInPreReservedRegion(true),
    m_jitChakraBaseAddress((intptr_t)GetModuleHandle(L"Chakra.dll")), // TODO: OOP JIT, don't hardcode name
    m_jitCRTBaseAddress((intptr_t)GetModuleHandle(UCrtC99MathApis::LibraryName)),
    m_delayLoadWinCoreProcessThreads(),
    m_activeJITCount(0)
{
}

intptr_t
ThreadContextInfo::GetNullFrameDisplayAddr() const
{
    return SHIFT_ADDR(this, &Js::NullFrameDisplay);
}

intptr_t
ThreadContextInfo::GetStrictNullFrameDisplayAddr() const
{
    return SHIFT_ADDR(this, &Js::StrictNullFrameDisplay);
}

intptr_t
ThreadContextInfo::GetAbsDoubleCstAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::AbsDoubleCst);
}

intptr_t
ThreadContextInfo::GetAbsFloatCstAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::AbsFloatCst);
}

intptr_t
ThreadContextInfo::GetMaskNegFloatAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::MaskNegFloat);
}

intptr_t
ThreadContextInfo::GetMaskNegDoubleAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::MaskNegDouble);
}

intptr_t
ThreadContextInfo::GetUIntConvertConstAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::UIntConvertConst);
}

intptr_t
ThreadContextInfo::GetUint8ClampedArraySetItemAddr() const
{
    return SHIFT_ADDR(this, (BOOL(*)(Js::Uint8ClampedArray * arr, uint32 index, Js::Var value))&Js::Uint8ClampedArray::DirectSetItem);
}

intptr_t
ThreadContextInfo::GetConstructorCacheDefaultInstanceAddr() const
{
    return SHIFT_ADDR(this, &Js::ConstructorCache::DefaultInstance);
}

intptr_t
ThreadContextInfo::GetJavascriptObjectNewInstanceAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptObject::EntryInfo::NewInstance);
}

intptr_t
ThreadContextInfo::GetDoubleOnePointZeroAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::ONE_POINT_ZERO);
}

intptr_t
ThreadContextInfo::GetDoublePointFiveAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_PointFive);
}

intptr_t
ThreadContextInfo::GetFloatPointFiveAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_Float32PointFive);
}

intptr_t
ThreadContextInfo::GetDoubleNegPointFiveAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_NegPointFive);
}

intptr_t
ThreadContextInfo::GetFloatNegPointFiveAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_Float32NegPointFive);
}

intptr_t
ThreadContextInfo::GetDoubleTwoToFractionAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_TwoToFraction);
}

intptr_t
ThreadContextInfo::GetFloatTwoToFractionAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_Float32TwoToFraction);
}

intptr_t
ThreadContextInfo::GetDoubleNegTwoToFractionAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_NegTwoToFraction);
}

intptr_t
ThreadContextInfo::GetFloatNegTwoToFractionAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_Float32NegTwoToFraction);
}

intptr_t
ThreadContextInfo::GetDoubleZeroAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_Zero);
}

intptr_t
ThreadContextInfo::GetFloatZeroAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNumber::k_Float32Zero);
}

intptr_t
ThreadContextInfo::GetNativeFloatArrayMissingItemAddr() const
{
    return SHIFT_ADDR(this, &Js::JavascriptNativeFloatArray::MissingItem);
}

intptr_t
ThreadContextInfo::GetExponentMaskAddr() const
{
    return SHIFT_ADDR(this, &Js::Constants::ExponentMask);
}

intptr_t
ThreadContextInfo::GetMantissaMaskAddr() const
{
    return SHIFT_ADDR(this, &Js::Constants::MantissaMask);
}

intptr_t
ThreadContextInfo::GetThreadStackLimitAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.threadStackLimitAddr);
}

intptr_t
ThreadContextInfo::GetDisableImplicitFlagsAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.disableImplicitFlagsAddr);
}

intptr_t
ThreadContextInfo::GetImplicitCallFlagsAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.implicitCallFlagsAddr);
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

intptr_t
ThreadContextInfo::GetRuntimeCRTBaseAddress() const
{
    return static_cast<intptr_t>(m_threadContextData.crtBaseAddress);
}

intptr_t
ThreadContextInfo::GetBailOutRegisterSaveSpace() const
{
    return static_cast<intptr_t>(m_threadContextData.bailOutRegisterSaveSpace);
}

intptr_t
ThreadContextInfo::GetStringReplaceNameAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.stringReplaceNameAddr);
}

intptr_t
ThreadContextInfo::GetStringMatchNameAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.stringMatchNameAddr);
}


intptr_t
ThreadContextInfo::GetDebuggingFlagsAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.debuggingFlagsAddr);
}

intptr_t
ThreadContextInfo::GetDebugStepTypeAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.debugStepTypeAddr);
}

intptr_t
ThreadContextInfo::GetDebugFrameAddressAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.debugFrameAddressAddr);
}

intptr_t
ThreadContextInfo::GetDebugScriptIdWhenSetAddr() const
{
    return static_cast<intptr_t>(m_threadContextData.debugScriptIdWhenSetAddr);
}

ptrdiff_t
ThreadContextInfo::GetChakraBaseAddressDifference() const
{
    return GetRuntimeChakraBaseAddress() - m_jitChakraBaseAddress;
}

ptrdiff_t
ThreadContextInfo::GetCRTBaseAddressDifference() const
{
    return GetRuntimeCRTBaseAddress() - m_jitCRTBaseAddress;
}

bool
ThreadContextInfo::IsCFGEnabled()
{
#if defined(_CONTROL_FLOW_GUARD)
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY CfgPolicy;
    m_delayLoadWinCoreProcessThreads.EnsureFromSystemDirOnly();
    BOOL isGetMitigationPolicySucceeded = m_delayLoadWinCoreProcessThreads.GetMitigationPolicyForProcess(
        this->GetProcessHandle(),
        ProcessControlFlowGuardPolicy,
        &CfgPolicy,
        sizeof(CfgPolicy));
    Assert(isGetMitigationPolicySucceeded || !AutoSystemInfo::Data.IsCFGEnabled());
    return CfgPolicy.EnableControlFlowGuard && AutoSystemInfo::Data.IsCFGEnabled();
#else
    return false;
#endif
}

void
ThreadContextInfo::BeginJIT()
{
    InterlockedExchangeAdd(&m_activeJITCount, 1);
}

void
ThreadContextInfo::EndJIT()
{
    InterlockedExchangeSubtract(&m_activeJITCount, 1);
}

bool
ThreadContextInfo::IsJITActive()
{
    return m_activeJITCount != 0;
}

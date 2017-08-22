//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeBasePch.h"

// Originally defined in ntstatus.h, define here because including windows.h (via PCH
// above) with ntstatus.h causes macro redefinition errors for the common errors defined
// in both header files.
#define STATUS_PROCESS_IS_TERMINATING    ((NTSTATUS)0xC000010AL)

#if ENABLE_NATIVE_CODEGEN
#include "CodeGenAllocators.h"
#include "ServerThreadContext.h"
#endif

ThreadContextInfo::ThreadContextInfo() :
    m_isAllJITCodeInPreReservedRegion(true),
    m_isClosed(false)
{
    for (int i = 0; i <= WellKnownHostType_Last; ++i)
    {
        wellKnownHostTypeIds[i] = Js::TypeIds_Undefined;
    }
}

#if ENABLE_NATIVE_CODEGEN
intptr_t
ThreadContextInfo::GetNullFrameDisplayAddr() const
{
    return ShiftAddr(this, &Js::NullFrameDisplay);
}

intptr_t
ThreadContextInfo::GetStrictNullFrameDisplayAddr() const
{
    return ShiftAddr(this, &Js::StrictNullFrameDisplay);
}

intptr_t
ThreadContextInfo::GetAbsDoubleCstAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::AbsDoubleCst);
}

intptr_t
ThreadContextInfo::GetAbsFloatCstAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::AbsFloatCst);
}

intptr_t ThreadContextInfo::GetSgnFloatBitCst() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::SgnFloatBitCst);
}

intptr_t ThreadContextInfo::GetSgnDoubleBitCst() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::SgnDoubleBitCst);
}

intptr_t
ThreadContextInfo::GetMaskNegFloatAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::MaskNegFloat);
}

intptr_t
ThreadContextInfo::GetMaskNegDoubleAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::MaskNegDouble);
}

intptr_t
ThreadContextInfo::GetUIntConvertConstAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::UIntConvertConst);
}

intptr_t
ThreadContextInfo::GetUint8ClampedArraySetItemAddr() const
{
    return ShiftAddr(this, (BOOL(*)(Js::Uint8ClampedArray * arr, uint32 index, Js::Var value))&Js::Uint8ClampedArray::DirectSetItem);
}

intptr_t
ThreadContextInfo::GetConstructorCacheDefaultInstanceAddr() const
{
    return ShiftAddr(this, &Js::ConstructorCache::DefaultInstance);
}

intptr_t
ThreadContextInfo::GetJavascriptObjectNewInstanceAddr() const
{
    return ShiftAddr(this, &Js::JavascriptObject::EntryInfo::NewInstance);
}

intptr_t
ThreadContextInfo::GetJavascriptArrayNewInstanceAddr() const
{
    return ShiftAddr(this, &Js::JavascriptArray::EntryInfo::NewInstance);
}

intptr_t
ThreadContextInfo::GetDoubleOnePointZeroAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::ONE_POINT_ZERO);
}

intptr_t
ThreadContextInfo::GetDoublePointFiveAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_PointFive);
}

intptr_t
ThreadContextInfo::GetFloatPointFiveAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_Float32PointFive);
}

intptr_t
ThreadContextInfo::GetDoubleNegPointFiveAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_NegPointFive);
}

intptr_t
ThreadContextInfo::GetFloatNegPointFiveAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_Float32NegPointFive);
}

intptr_t
ThreadContextInfo::GetDoubleNegOneAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_NegOne);
}

intptr_t
ThreadContextInfo::GetDoubleTwoToFractionAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_TwoToFraction);
}

intptr_t
ThreadContextInfo::GetFloatTwoToFractionAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_Float32TwoToFraction);
}

intptr_t
ThreadContextInfo::GetDoubleNegTwoToFractionAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_NegTwoToFraction);
}

intptr_t
ThreadContextInfo::GetDoubleNaNAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_Nan);
}

intptr_t
ThreadContextInfo::GetDoubleUintMaxPlusOneAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_UintMaxPlusOne);
}

intptr_t
ThreadContextInfo::GetDoubleIntMaxPlusOneAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_IntMaxPlusOne);
}

intptr_t
ThreadContextInfo::GetDoubleIntMinMinusOneAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_MinIntMinusOne);
}

intptr_t
ThreadContextInfo::GetFloatNaNAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_Nan32);
}

intptr_t
ThreadContextInfo::GetFloatNegTwoToFractionAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_Float32NegTwoToFraction);
}

intptr_t
ThreadContextInfo::GetDoubleZeroAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_Zero);
}

intptr_t
ThreadContextInfo::GetFloatZeroAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::k_Float32Zero);
}

intptr_t
ThreadContextInfo::GetNativeFloatArrayMissingItemAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNativeFloatArray::MissingItem);
}

intptr_t
ThreadContextInfo::GetExponentMaskAddr() const
{
    return ShiftAddr(this, &Js::Constants::ExponentMask);
}

intptr_t
ThreadContextInfo::GetMantissaMaskAddr() const
{
    return ShiftAddr(this, &Js::Constants::MantissaMask);
}

#if _M_IX86 || _M_AMD64

intptr_t
ThreadContextInfo::GetX86AbsMaskF4Addr() const
{
    return ShiftAddr(this, &X86_ABS_MASK_F4);
}

intptr_t
ThreadContextInfo::GetX86AbsMaskD2Addr() const
{
    return ShiftAddr(this, &X86_ABS_MASK_D2);
}

intptr_t
ThreadContextInfo::GetX86NegMaskF4Addr() const
{
    return ShiftAddr(this, &X86_NEG_MASK_F4);
}

intptr_t
ThreadContextInfo::GetX86NegMaskD2Addr() const
{
    return ShiftAddr(this, &X86_NEG_MASK_D2);
}

intptr_t
ThreadContextInfo::GetX86AllNegOnesAddr() const
{
    return ShiftAddr(this, &X86_ALL_NEG_ONES);
}

intptr_t
ThreadContextInfo::GetX86AllNegOnesF4Addr() const
{
    return ShiftAddr(this, &X86_ALL_NEG_ONES_F4);
}

intptr_t
ThreadContextInfo::GetX86AllZerosAddr() const
{
    return ShiftAddr(this, &X86_ALL_ZEROS);
}

intptr_t
ThreadContextInfo::GetX86AllOnesF4Addr() const
{
    return ShiftAddr(this, &X86_ALL_ONES_F4);
}

intptr_t
ThreadContextInfo::GetX86LowBytesMaskAddr() const
{
    return ShiftAddr(this, &X86_LOWBYTES_MASK);
}

intptr_t
ThreadContextInfo::GetX86HighBytesMaskAddr() const
{
    return ShiftAddr(this, &X86_HIGHBYTES_MASK);
}

intptr_t
ThreadContextInfo::GetX86DoubleWordSignBitsAddr() const
{
    return ShiftAddr(this, &X86_DWORD_SIGNBITS);
}

intptr_t
ThreadContextInfo::GetX86WordSignBitsAddr() const
{
    return ShiftAddr(this, &X86_WORD_SIGNBITS);
}

intptr_t
ThreadContextInfo::GetX86ByteSignBitsAddr() const
{
    return ShiftAddr(this, &X86_BYTE_SIGNBITS);
}

intptr_t
ThreadContextInfo::GetX86TwoPower32F4Addr() const
{
    return ShiftAddr(this, &X86_TWO_32_F4);
}

intptr_t
ThreadContextInfo::GetX86TwoPower31F4Addr() const
{
    return ShiftAddr(this, &X86_TWO_31_F4);
}

intptr_t
ThreadContextInfo::GetX86TwoPower31I4Addr() const
{
    return ShiftAddr(this, &X86_TWO_31_I4);
}

intptr_t
ThreadContextInfo::GetX86NegTwoPower31F4Addr() const
{
    return ShiftAddr(this, &X86_NEG_TWO_31_F4);
}

intptr_t
ThreadContextInfo::GetX86FourLanesMaskAddr(uint8 minorityLane) const
{
    return ShiftAddr(this, &X86_4LANES_MASKS[minorityLane]);
}

intptr_t
ThreadContextInfo::GetDoubleIntMinAddr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::DOUBLE_INT_MIN);
}

intptr_t
ThreadContextInfo::GetDoubleTwoTo31Addr() const
{
    return ShiftAddr(this, &Js::JavascriptNumber::DOUBLE_TWO_TO_31);
}
#endif

intptr_t
ThreadContextInfo::GetStringReplaceNameAddr() const
{
    return ShiftAddr(this, Js::Constants::StringReplace);
}

intptr_t
ThreadContextInfo::GetStringMatchNameAddr() const
{
    return ShiftAddr(this, Js::Constants::StringMatch);
}
#endif

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

#ifdef ENABLE_GLOBALIZATION

#if defined(_CONTROL_FLOW_GUARD)
Js::DelayLoadWinCoreProcessThreads *
ThreadContextInfo::GetWinCoreProcessThreads()
{
    m_delayLoadWinCoreProcessThreads.EnsureFromSystemDirOnly();
    return &m_delayLoadWinCoreProcessThreads;
}

Js::DelayLoadWinCoreMemory *
ThreadContextInfo::GetWinCoreMemoryLibrary()
{
    m_delayLoadWinCoreMemoryLibrary.EnsureFromSystemDirOnly();
    return &m_delayLoadWinCoreMemoryLibrary;
}
#endif

bool
ThreadContextInfo::IsCFGEnabled()
{
#if defined(_CONTROL_FLOW_GUARD)
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY CfgPolicy;
    m_delayLoadWinCoreProcessThreads.EnsureFromSystemDirOnly();
    BOOL isGetMitigationPolicySucceeded = m_delayLoadWinCoreProcessThreads.GetMitigationPolicyForProcess(
        GetCurrentProcess(),
        ProcessControlFlowGuardPolicy,
        &CfgPolicy,
        sizeof(CfgPolicy));
    Assert(isGetMitigationPolicySucceeded || !AutoSystemInfo::Data.IsCFGEnabled());
    return CfgPolicy.EnableControlFlowGuard && AutoSystemInfo::Data.IsCFGEnabled();
#else
    return false;
#endif // _CONTROL_FLOW_GUARD
}
#endif // ENABLE_GLOBALIZATION

//Masking bits according to AutoSystemInfo::PageSize
#define PAGE_START_ADDR(address) ((size_t)(address) & ~(size_t)(AutoSystemInfo::PageSize - 1))
#define IS_16BYTE_ALIGNED(address) (((size_t)(address) & 0xF) == 0)
#define OFFSET_ADDR_WITHIN_PAGE(address) ((size_t)(address) & (AutoSystemInfo::PageSize - 1))

void
ThreadContextInfo::SetValidCallTargetForCFG(PVOID callTargetAddress, bool isSetValid)
{
#ifdef _CONTROL_FLOW_GUARD
    if (IsCFGEnabled())
    {
        AssertMsg(IS_16BYTE_ALIGNED(callTargetAddress), "callTargetAddress is not 16-byte page aligned?");

        // If SetProcessValidCallTargets is not allowed by global policy (e.g.
        // OOP JIT is in use in the client), then generate a fast fail
        // exception as state has been corrupted and attempt is being made to
        // illegally call SetProcessValidCallTargets.
        if (!GlobalSecurityPolicy::IsSetProcessValidCallTargetsAllowed())
        {
            RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
        }

        PVOID startAddressOfPage = (PVOID)(PAGE_START_ADDR(callTargetAddress));
        size_t codeOffset = OFFSET_ADDR_WITHIN_PAGE(callTargetAddress);

        CFG_CALL_TARGET_INFO callTargetInfo[1];

        callTargetInfo[0].Offset = codeOffset;
        callTargetInfo[0].Flags = (isSetValid ? CFG_CALL_TARGET_VALID : 0);

        AssertMsg((size_t)callTargetAddress - (size_t)startAddressOfPage <= AutoSystemInfo::PageSize - 1, "Only last bits corresponding to PageSize should be masked");
        AssertMsg((size_t)startAddressOfPage + (size_t)codeOffset == (size_t)callTargetAddress, "Wrong masking of address?");

        BOOL isCallTargetRegistrationSucceed = GetWinCoreMemoryLibrary()->SetProcessCallTargets(GetProcessHandle(), startAddressOfPage, AutoSystemInfo::PageSize, 1, callTargetInfo);

        if (!isCallTargetRegistrationSucceed)
        {
            DWORD gle = GetLastError();
            if (gle == ERROR_COMMITMENT_LIMIT)
            {
                //Throw OOM, if there is not enough virtual memory for paging (required for CFG BitMap)
                Js::Throw::OutOfMemory();
            }
            else if (gle == STATUS_PROCESS_IS_TERMINATING)
            {
                // When this error is set, the target process is exiting and thus cannot proceed with
                // JIT output. Throw this exception to safely abort this call.
                throw Js::OperationAbortedException();
            }
            else
            {
                Js::Throw::InternalError();
            }
        }
#if DBG
        if (isSetValid && !JITManager::GetJITManager()->IsOOPJITEnabled())
        {
            _guard_check_icall((uintptr_t)callTargetAddress);
        }

        if (PHASE_TRACE1(Js::CFGPhase))
        {
            if (!isSetValid)
            {
                Output::Print(_u("DEREGISTER:"));
            }
            Output::Print(_u("CFGRegistration: StartAddr: 0x%p , Offset: 0x%x, TargetAddr: 0x%x \n"), (char*)startAddressOfPage, callTargetInfo[0].Offset, ((size_t)startAddressOfPage + (size_t)callTargetInfo[0].Offset));
            Output::Flush();
        }
#endif
    }
#endif // _CONTROL_FLOW_GUARD
}

bool
ThreadContextInfo::IsClosed()
{
    return m_isClosed;
}

uintptr_t ShiftAddr(const ThreadContextInfo * const context, uintptr_t address)
{
#if ENABLE_OOP_NATIVE_CODEGEN
    ptrdiff_t diff = 0;
    if (AutoSystemInfo::Data.IsJscriptModulePointer((void*)address))
    {
        diff = context->GetChakraBaseAddressDifference();
    }
    else if(AutoSystemInfo::Data.IsCRTModulePointer(address))
    {
        diff = context->GetCRTBaseAddressDifference();
    }
    else
    {
        AssertOrFailFast(UNREACHED);
    }

    return (intptr_t)address + diff;
#else
    return address;
#endif

}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// TODO michhol (OOP JIT): rename this
class ThreadContextInfo
{
public:
    ThreadContextInfo(ThreadContextDataIDL * data);
    ~ThreadContextInfo();
    intptr_t GetNullFrameDisplayAddr() const;
    intptr_t GetStrictNullFrameDisplayAddr() const;
    intptr_t GetThreadStackLimitAddr() const;

    intptr_t GetAbsDoubleCstAddr() const;
    intptr_t GetAbsFloatCstAddr() const;
    intptr_t GetMaskNegFloatAddr() const;
    intptr_t GetMaskNegDoubleAddr() const;
    intptr_t GetDoubleOnePointZeroAddr() const;
    intptr_t GetDoublePointFiveAddr() const;
    intptr_t GetFloatPointFiveAddr() const;
    intptr_t GetDoubleNegPointFiveAddr() const;
    intptr_t GetFloatNegPointFiveAddr() const;
    intptr_t GetDoubleTwoToFractionAddr() const;
    intptr_t GetFloatTwoToFractionAddr() const;
    intptr_t GetDoubleNegTwoToFractionAddr() const;
    intptr_t GetFloatNegTwoToFractionAddr() const;
    intptr_t GetDoubleZeroAddr() const;
    intptr_t GetFloatZeroAddr() const;

    intptr_t GetUIntConvertConstAddr() const;
    intptr_t GetUint8ClampedArraySetItemAddr() const;
    intptr_t GetConstructorCacheDefaultInstanceAddr() const;
    intptr_t GetJavascriptObjectNewInstanceAddr() const;
    intptr_t GetNativeFloatArrayMissingItemAddr() const;
    intptr_t GetMantissaMaskAddr() const;
    intptr_t GetExponentMaskAddr() const;

    intptr_t GetDisableImplicitFlagsAddr() const;
    intptr_t GetImplicitCallFlagsAddr() const;
    intptr_t GetBailOutRegisterSaveSpace() const;
    intptr_t GetStringReplaceNameAddr() const;
    intptr_t GetStringMatchNameAddr() const;

    intptr_t GetDebuggingFlagsAddr() const;
    intptr_t GetDebugStepTypeAddr() const;
    intptr_t GetDebugFrameAddressAddr() const;
    intptr_t GetDebugScriptIdWhenSetAddr() const;

    size_t GetScriptStackLimit() const;

    bool IsThreadBound() const;

    PageAllocator * GetPageAllocator();
    CodeGenAllocators * GetCodeGenAllocators();
    AllocationPolicyManager * GetAllocationPolicyManager();
    CustomHeap::CodePageAllocators * GetCodePageAllocators();
    HANDLE GetProcessHandle() const;

    void ResetIsAllJITCodeInPreReservedRegion();
    bool IsAllJITCodeInPreReservedRegion() const;

    ptrdiff_t GetChakraBaseAddressDifference() const;
    ptrdiff_t GetCRTBaseAddressDifference() const;

    Js::PropertyRecord const * GetPropertyRecord(Js::PropertyId propertyId);

    bool IsCFGEnabled();
    void AddToPropertyMap(const Js::PropertyRecord * propertyRecord);
    void BeginJIT();
    void EndJIT();
    bool IsJITActive();

private:
    intptr_t GetRuntimeChakraBaseAddress() const;
    intptr_t GetRuntimeCRTBaseAddress() const;
    uint m_activeJITCount;

    Js::DelayLoadWinCoreProcessThreads m_delayLoadWinCoreProcessThreads;
    PreReservedVirtualAllocWrapper m_preReservedVirtualAllocator;
    CustomHeap::CodePageAllocators m_codePageAllocators;
    PageAllocator m_pageAlloc;
    AllocationPolicyManager m_policyManager;
    CodeGenAllocators m_codeGenAlloc;

    ThreadContextDataIDL m_threadContextData;

    typedef JsUtil::BaseHashSet<const Js::PropertyRecord *, HeapAllocator, PrimeSizePolicy, const Js::PropertyRecord *,
        DefaultComparer, JsUtil::SimpleHashedEntry, JsUtil::AsymetricResizeLock> JITPropertyMap;
    JITPropertyMap * m_propertyMap;
    intptr_t m_jitChakraBaseAddress;
    intptr_t m_jitCRTBaseAddress;
    bool m_isAllJITCodeInPreReservedRegion;
};

// TODO: OOP JIT, is there any issue when crossing over 2^31/2^63?
#define SHIFT_ADDR(context, address) \
    (intptr_t)address + context->GetChakraBaseAddressDifference()

#define SHIFT_CRT_ADDR(context, address) \
    (intptr_t)address + context->GetCRTBaseAddressDifference()

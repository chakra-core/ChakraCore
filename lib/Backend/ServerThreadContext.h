//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ServerThreadContext : public ThreadContextInfo
{
public:
    ServerThreadContext(ThreadContextDataIDL * data);
    ~ServerThreadContext();

    virtual HANDLE GetProcessHandle() const override;

    virtual bool IsThreadBound() const override;

    virtual size_t GetScriptStackLimit() const override;

    virtual intptr_t GetThreadStackLimitAddr() const override;

#if defined(_M_IX86) || defined(_M_X64)
    virtual intptr_t GetSimdTempAreaAddr(uint8 tempIndex) const override;
#endif

    virtual intptr_t GetDisableImplicitFlagsAddr() const override;
    virtual intptr_t GetImplicitCallFlagsAddr() const override;
    virtual intptr_t GetBailOutRegisterSaveSpaceAddr() const override;

    virtual intptr_t GetDebuggingFlagsAddr() const override;
    virtual intptr_t GetDebugStepTypeAddr() const override;
    virtual intptr_t GetDebugFrameAddressAddr() const override;
    virtual intptr_t GetDebugScriptIdWhenSetAddr() const override;

    ptrdiff_t GetChakraBaseAddressDifference() const;
    ptrdiff_t GetCRTBaseAddressDifference() const;

    virtual Js::PropertyRecord const * GetPropertyRecord(Js::PropertyId propertyId) override;
    virtual PreReservedVirtualAllocWrapper * GetPreReservedVirtualAllocator() override;

    CodeGenAllocators * GetCodeGenAllocators();
    AllocationPolicyManager * GetAllocationPolicyManager();
    CustomHeap::CodePageAllocators * GetCodePageAllocators();
    PageAllocator* GetPageAllocator();
    void AddToPropertyMap(const Js::PropertyRecord * propertyRecord);
    void SetWellKnownHostTypeId(Js::TypeId typeId) { this->wellKnownHostTypeHTMLAllCollectionTypeId = typeId; }
private:
    intptr_t GetRuntimeChakraBaseAddress() const;
    intptr_t GetRuntimeCRTBaseAddress() const;

    typedef JsUtil::BaseHashSet<const Js::PropertyRecord *, HeapAllocator, PrimeSizePolicy, const Js::PropertyRecord *,
        DefaultComparer, JsUtil::SimpleHashedEntry, JsUtil::AsymetricResizeLock> PropertyMap;
    PropertyMap * m_propertyMap;

    AllocationPolicyManager m_policyManager;
    JsUtil::BaseDictionary<DWORD, PageAllocator*, HeapAllocator> m_pageAllocs;
    PreReservedVirtualAllocWrapper m_preReservedVirtualAllocator;
    CustomHeap::CodePageAllocators m_codePageAllocators;
    CodeGenAllocators m_codeGenAlloc;

    ThreadContextDataIDL m_threadContextData;

    ThreadContext * m_threadContext;
    intptr_t m_jitChakraBaseAddress;
    intptr_t m_jitCRTBaseAddress;
};

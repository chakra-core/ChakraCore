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

#if defined(ENABLE_SIMDJS) && (defined(_M_IX86) || defined(_M_X64))
    virtual intptr_t GetSimdTempAreaAddr(uint8 tempIndex) const override;
#endif

    virtual intptr_t GetDisableImplicitFlagsAddr() const override;
    virtual intptr_t GetImplicitCallFlagsAddr() const override;
    virtual intptr_t GetBailOutRegisterSaveSpaceAddr() const override;

    virtual PreReservedVirtualAllocWrapper * GetPreReservedVirtualAllocator() override;

    virtual bool IsNumericProperty(Js::PropertyId propId) override;

    ptrdiff_t GetChakraBaseAddressDifference() const;
    ptrdiff_t GetCRTBaseAddressDifference() const;

    CodeGenAllocators * GetCodeGenAllocators();
    AllocationPolicyManager * GetAllocationPolicyManager();
    CustomHeap::CodePageAllocators * GetCodePageAllocators();
    PageAllocator* GetPageAllocator();
    void RemoveFromNumericPropertySet(Js::PropertyId reclaimedId);
    void AddToNumericPropertySet(Js::PropertyId propertyId);
    void SetWellKnownHostTypeId(Js::TypeId typeId) { this->wellKnownHostTypeHTMLAllCollectionTypeId = typeId; }
#if DYNAMIC_INTERPRETER_THUNK || defined(ASMJS_PLAT)
    CustomHeap::CodePageAllocators * GetThunkPageAllocators();
#endif
    void AddRef();
    void Release();
    void Close();
    PageAllocator * GetForegroundPageAllocator();
#ifdef STACK_BACK_TRACE
    DWORD GetRuntimePid() { return m_pid; }
#endif

private:
    intptr_t GetRuntimeChakraBaseAddress() const;
    intptr_t GetRuntimeCRTBaseAddress() const;

    typedef JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator, PrimeSizePolicy, Js::PropertyId,
        DefaultComparer, JsUtil::SimpleHashedEntry, JsUtil::AsymetricResizeLock> PropertySet;
    PropertySet * m_numericPropertySet;

    AllocationPolicyManager m_policyManager;
    JsUtil::BaseDictionary<DWORD, PageAllocator*, HeapAllocator> m_pageAllocs;
    PreReservedVirtualAllocWrapper m_preReservedVirtualAllocator;
#if DYNAMIC_INTERPRETER_THUNK || defined(ASMJS_PLAT)
    CustomHeap::CodePageAllocators m_thunkPageAllocators;
#endif
    CustomHeap::CodePageAllocators m_codePageAllocators;
    CodeGenAllocators m_codeGenAlloc;
    // only allocate with this from foreground calls (never from CodeGen calls)
    PageAllocator m_pageAlloc;

    ThreadContextDataIDL m_threadContextData;

    DWORD m_pid; //save client process id for easier diagnose
    
    intptr_t m_jitChakraBaseAddress;
    intptr_t m_jitCRTBaseAddress;
    uint m_refCount;

};

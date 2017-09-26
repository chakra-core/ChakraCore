//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ServerThreadContext : public ThreadContextInfo
{
#if ENABLE_OOP_NATIVE_CODEGEN
public:
    typedef BVSparseNode<JitArenaAllocator> BVSparseNode;

    ServerThreadContext(ThreadContextDataIDL * data, HANDLE processHandle);
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

    PreReservedSectionAllocWrapper * GetPreReservedSectionAllocator();

    virtual bool IsNumericProperty(Js::PropertyId propId) override;

    virtual ptrdiff_t GetChakraBaseAddressDifference() const override;
    virtual ptrdiff_t GetCRTBaseAddressDifference() const override;

    OOPCodeGenAllocators * GetCodeGenAllocators();
#if defined(_CONTROL_FLOW_GUARD) && (_M_IX86 || _M_X64)
    OOPJITThunkEmitter * GetJITThunkEmitter();
#endif
    CustomHeap::OOPCodePageAllocators * GetThunkPageAllocators();
    CustomHeap::OOPCodePageAllocators  * GetCodePageAllocators();
    SectionAllocWrapper * GetSectionAllocator();
    void UpdateNumericPropertyBV(BVSparseNode * newProps);
    void SetWellKnownHostTypeId(Js::TypeId typeId) { this->wellKnownHostTypeIds[WellKnownHostType_HTMLAllCollection] = typeId; }
    void AddRef();
    void Release();
    void Close();
    PageAllocator * GetForegroundPageAllocator();
    DWORD GetRuntimePid() { return m_pid; }

    intptr_t GetRuntimeChakraBaseAddress() const;
    intptr_t GetRuntimeCRTBaseAddress() const;

    static intptr_t GetJITCRTBaseAddress();

private:

    AutoCloseHandle m_autoProcessHandle;

    BVSparse<HeapAllocator> * m_numericPropertyBV;

    PreReservedSectionAllocWrapper m_preReservedSectionAllocator;
    SectionAllocWrapper m_sectionAllocator;
    CustomHeap::OOPCodePageAllocators m_thunkPageAllocators;
    CustomHeap::OOPCodePageAllocators  m_codePageAllocators;
#if defined(_CONTROL_FLOW_GUARD) && (_M_IX86 || _M_X64)
    OOPJITThunkEmitter m_jitThunkEmitter;
#endif
    OOPCodeGenAllocators m_codeGenAlloc;
    // only allocate with this from foreground calls (never from CodeGen calls)
    PageAllocator m_pageAlloc;

    ThreadContextDataIDL m_threadContextData;

    DWORD m_pid; //save client process id for easier diagnose

    CriticalSection m_cs;
    uint m_refCount;
#endif
};

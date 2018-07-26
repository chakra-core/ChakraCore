//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#if ENABLE_OOP_NATIVE_CODEGEN
class ProcessContext
{
private:
    uint refCount;

public:
    HANDLE processHandle;
    intptr_t chakraBaseAddress;
    intptr_t crtBaseAddress;

    ProcessContext(HANDLE processHandle, intptr_t chakraBaseAddress, intptr_t crtBaseAddress);
    ~ProcessContext();
    void AddRef();
    void Release();
    bool HasRef();

};

class ServerThreadContext : public ThreadContextInfo
{
public:
    typedef BVSparseNode<JitArenaAllocator> BVSparseNode;

    ServerThreadContext(ThreadContextDataIDL * data, ProcessContext* processContext);
    ~ServerThreadContext();

    virtual HANDLE GetProcessHandle() const override;

    virtual bool IsThreadBound() const override;

    virtual size_t GetScriptStackLimit() const override;

    virtual intptr_t GetThreadStackLimitAddr() const override;

#ifdef ENABLE_WASM_SIMD
    virtual intptr_t GetSimdTempAreaAddr(uint8 tempIndex) const override;
#endif

    virtual intptr_t GetDisableImplicitFlagsAddr() const override;
    virtual intptr_t GetImplicitCallFlagsAddr() const override;
    virtual intptr_t GetBailOutRegisterSaveSpaceAddr() const override;

    PreReservedSectionAllocWrapper * GetPreReservedSectionAllocator();

    virtual bool IsNumericProperty(Js::PropertyId propId) override;

    virtual ptrdiff_t GetChakraBaseAddressDifference() const override;
    virtual ptrdiff_t GetCRTBaseAddressDifference() const override;

#if defined(_CONTROL_FLOW_GUARD) && !defined(_M_ARM)
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
    bool CanCreatePreReservedSegment() const;
    void SetCanCreatePreReservedSegment(bool value);
    static intptr_t GetJITCRTBaseAddress();

private:
    ProcessContext* processContext;

    BVSparse<HeapAllocator> * m_numericPropertyBV;

    PreReservedSectionAllocWrapper m_preReservedSectionAllocator;
    SectionAllocWrapper m_sectionAllocator;
    CustomHeap::OOPCodePageAllocators m_thunkPageAllocators;
    CustomHeap::OOPCodePageAllocators  m_codePageAllocators;
#if defined(_CONTROL_FLOW_GUARD) && !defined(_M_ARM)
    OOPJITThunkEmitter m_jitThunkEmitter;
#endif
    // only allocate with this from foreground calls (never from CodeGen calls)
    PageAllocator m_pageAlloc;
    ThreadContextDataIDL m_threadContextData;

    DWORD m_pid; //save client process id for easier diagnose

    CriticalSection m_cs;
    uint m_refCount;
    bool m_canCreatePreReservedSegment;
};
#endif

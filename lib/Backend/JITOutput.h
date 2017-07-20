//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITOutput
{
public:
    JITOutput(JITOutputIDL * outputData);

    void SetHasJITStackClosure();

    void SetVarSlotsOffset(int32 offset);
    void SetVarChangedOffset(int32 offset);
    void SetHasBailoutInstr(bool val);
    void SetArgUsedForBranch(uint8 param);
    void SetFrameHeight(uint val);
    void RecordThrowMap(Js::ThrowMapEntry * throwMap, uint mapCount);
#ifdef _M_ARM
    void RecordXData(BYTE * xdata);
#endif
    bool IsTrackCompoundedIntOverflowDisabled() const;
    bool IsArrayCheckHoistDisabled() const;
    bool IsStackArgOptDisabled() const;
    bool IsSwitchOptDisabled() const;
    bool IsAggressiveIntTypeSpecDisabled() const;

    uint16 GetArgUsedForBranch() const;
    intptr_t GetCodeAddress() const;
    size_t GetCodeSize() const;
    ushort GetPdataCount() const;
    ushort GetXdataSize() const;

    void SetCodeAddress(intptr_t addr);

    EmitBufferAllocation<VirtualAllocWrapper, PreReservedVirtualAllocWrapper> * RecordInProcNativeCodeSize(Func *func, uint32 bytes, ushort pdataCount, ushort xdataSize);
#if ENABLE_OOP_NATIVE_CODEGEN
    EmitBufferAllocation<SectionAllocWrapper, PreReservedSectionAllocWrapper> * RecordOOPNativeCodeSize(Func *func, uint32 bytes, ushort pdataCount, ushort xdataSize);
#endif
    void RecordNativeCode(const BYTE* sourceBuffer, BYTE* localCodeAddress);
    void RecordInlineeFrameOffsetsInfo(unsigned int offsetsArrayOffset, unsigned int offsetsArrayCount);

#if _M_X64
    void RecordUnwindInfo(BYTE *unwindInfo, size_t size, BYTE * xdataAddr, BYTE* localXdataAddr);
#elif _M_ARM
    size_t RecordUnwindInfo(size_t offset, BYTE *unwindInfo, size_t size, BYTE * xdataAddr);
#endif

    void FinalizeNativeCode();

    JITOutputIDL * GetOutputData();
private:
    template <typename TEmitBufferAllocation, typename TCodeGenAllocators>
    void RecordNativeCode(const BYTE* sourceBuffer, BYTE* localCodeAddress, TEmitBufferAllocation allocation, TCodeGenAllocators codeGenAllocators);
    union
    {
        EmitBufferAllocation<VirtualAllocWrapper, PreReservedVirtualAllocWrapper> * m_inProcAlloc;
#if ENABLE_OOP_NATIVE_CODEGEN
        EmitBufferAllocation<SectionAllocWrapper, PreReservedSectionAllocWrapper> * m_oopAlloc;
#endif
    };
    Func * m_func;
    JITOutputIDL * m_outputData;
};

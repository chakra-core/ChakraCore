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
    uint16 GetArgUsedForBranch() const;
    intptr_t GetCodeAddress() const;
    size_t GetCodeSize() const;
    ushort GetPdataCount() const;
    ushort GetXdataSize() const;

    void SetCodeAddress(intptr_t addr);

    EmitBufferAllocation * RecordNativeCodeSize(Func *func, uint32 bytes, ushort pdataCount, ushort xdataSize);
    void RecordNativeCode(Func *func, const BYTE* sourceBuffer, EmitBufferAllocation * alloc);
    void RecordInlineeFrameOffsetsInfo(unsigned int offsetsArrayOffset, unsigned int offsetsArrayCount);

#if _M_X64 || _M_ARM
    size_t RecordUnwindInfo(size_t offset, BYTE *unwindInfo, size_t size, BYTE * xdataAddr, HANDLE processHandle);
#endif

    void FinalizeNativeCode(Func *func, EmitBufferAllocation * alloc);

    JITOutputIDL * GetOutputData();
private:
    JITOutputIDL * m_outputData;
};

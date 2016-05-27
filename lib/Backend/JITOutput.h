//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITOutput
{
public:
    JITOutput(JITOutputData * outputData);

    void SetHasJITStackClosure();

    void SetVarSlotsOffset(int32 offset);
    void SetVarChangedOffset(int32 offset);
    void SetHasBailoutInstr(bool val);
    void SetArgUsedForBranch(uint8 param);

    uint16 GetArgUsedForBranch() const;
    intptr_t GetCodeAddress() const;

    EmitBufferAllocation * RecordNativeCodeSize(Func *func, uint32 bytes, ushort pdataCount, ushort xdataSize);
    void RecordNativeCode(Func *func, const BYTE* sourceBuffer, EmitBufferAllocation * alloc);

#if _M_X64 || _M_ARM
    size_t RecordUnwindInfo(size_t offset, BYTE *unwindInfo, size_t size, BYTE * xdataAddr, HANDLE processHandle);
#endif

    void FinalizeNativeCode(Func *func, EmitBufferAllocation * alloc);

    JITOutputData * GetOutputData();
private:
    JITOutputData * m_outputData;
};

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITOutput::JITOutput(JITOutputIDL * outputData) :
    m_outputData(outputData)
{
}

void
JITOutput::SetHasJITStackClosure()
{
    m_outputData->hasJittedStackClosure = true;
}

void
JITOutput::SetVarSlotsOffset(int32 offset)
{
    m_outputData->localVarSlotsOffset = offset;
}

void
JITOutput::SetVarChangedOffset(int32 offset)
{
    m_outputData->localVarChangedOffset = offset;
}

void
JITOutput::SetHasBailoutInstr(bool val)
{
    m_outputData->hasBailoutInstr = val;
}

void
JITOutput::SetArgUsedForBranch(uint8 param)
{
    Assert(param > 0);
    Assert(param < Js::Constants::MaximumArgumentCountForConstantArgumentInlining);
    m_outputData->argUsedForBranch |= (1 << (param - 1));
}

void
JITOutput::SetFrameHeight(uint val)
{
    m_outputData->frameHeight = val;
}

void
JITOutput::RecordThrowMap(Js::ThrowMapEntry * throwMap, uint mapCount)
{
    m_outputData->throwMapOffset = NativeCodeData::GetDataTotalOffset(throwMap);
    m_outputData->throwMapCount = mapCount;
}

uint16
JITOutput::GetArgUsedForBranch() const
{
    return m_outputData->argUsedForBranch;
}

intptr_t
JITOutput::GetCodeAddress() const
{
    return (intptr_t)m_outputData->codeAddress;
}

void
JITOutput::SetCodeAddress(intptr_t addr)
{
    m_outputData->codeAddress = addr;
}

size_t
JITOutput::GetCodeSize() const
{
    return (size_t)m_outputData->codeSize;
}

ushort
JITOutput::GetPdataCount() const
{
    return m_outputData->pdataCount;
}

ushort
JITOutput::GetXdataSize() const
{
    return m_outputData->xdataSize;
}

EmitBufferAllocation *
JITOutput::RecordNativeCodeSize(Func *func, uint32 bytes, ushort pdataCount, ushort xdataSize)
{
    BYTE *buffer;

#if defined(_M_ARM32_OR_ARM64)
    bool canAllocInPreReservedHeapPageSegment = false;
#else
    bool canAllocInPreReservedHeapPageSegment = func->CanAllocInPreReservedHeapPageSegment();
#endif

    EmitBufferAllocation *allocation = func->GetEmitBufferManager()->AllocateBuffer(bytes, &buffer, pdataCount, xdataSize, canAllocInPreReservedHeapPageSegment, true);

#if DBG
    MEMORY_BASIC_INFORMATION memBasicInfo;
    size_t resultBytes = VirtualQueryEx(func->GetThreadContextInfo()->GetProcessHandle(), allocation->allocation->address, &memBasicInfo, sizeof(memBasicInfo));
    Assert(resultBytes != 0 && memBasicInfo.Protect == PAGE_EXECUTE);
#endif

    Assert(allocation != nullptr);
    if (buffer == nullptr)
        Js::Throw::OutOfMemory();

    m_outputData->codeAddress = (intptr_t)buffer;
    m_outputData->codeSize = bytes;
    m_outputData->pdataCount = pdataCount;
    m_outputData->xdataSize = xdataSize;
    m_outputData->isInPrereservedRegion = allocation->inPrereservedRegion;
    return allocation;
}

void
JITOutput::RecordNativeCode(Func *func, const BYTE* sourceBuffer, EmitBufferAllocation * alloc)
{
    Assert(m_outputData->codeAddress == (intptr_t)alloc->allocation->address);
    if (!func->GetEmitBufferManager()->CommitBuffer(alloc, (BYTE *)m_outputData->codeAddress, m_outputData->codeSize, sourceBuffer))
    {
        Js::Throw::OutOfMemory();
    }

#if DBG_DUMP
    if (func->IsLoopBody())
    {
        func->GetEmitBufferManager()->totalBytesLoopBody += m_outputData->codeSize;
    }
#endif
}

void
JITOutput::RecordInlineeFrameOffsetsInfo(unsigned int offsetsArrayOffset, unsigned int offsetsArrayCount)
{
    m_outputData->inlineeFrameOffsetArrayOffset = offsetsArrayOffset;
    m_outputData->inlineeFrameOffsetArrayCount = offsetsArrayCount;
}

#if _M_X64
size_t
JITOutput::RecordUnwindInfo(size_t offset, BYTE *unwindInfo, size_t size, BYTE * xdataAddr, HANDLE processHandle)
{
    Assert(offset == 0);
    Assert(XDATA_SIZE >= size);
    ChakraMemCopy(xdataAddr, XDATA_SIZE, unwindInfo, size, processHandle);
    m_outputData->xdataAddr = (intptr_t)xdataAddr;
    return 0;
}
#elif _M_ARM
size_t
JITOutput::RecordUnwindInfo(size_t offset, BYTE *unwindInfo, size_t size, BYTE * xdataAddr, HANDLE processHandle)
{
    BYTE *xdataFinal = xdataAddr + offset;

    Assert(xdataFinal);
    Assert(((DWORD)xdataFinal & 0x3) == 0); // 4 byte aligned
    memcpy_s(xdataFinal, size, unwindInfo, size);

    return (size_t)xdataFinal;
}

void
JITOutput::RecordXData(BYTE * xdata)
{
    m_outputData->xdataOffset = NativeCodeData::GetDataTotalOffset(xdata);
}
#endif

void
JITOutput::FinalizeNativeCode(Func *func, EmitBufferAllocation * alloc)
{
    func->GetEmitBufferManager()->CompletePreviousAllocation(alloc);
    if (!func->IsOOPJIT())
    {
        func->GetInProcJITEntryPointInfo()->SetInProcJITNativeCodeData(func->GetNativeCodeDataAllocator()->Finalize());
        func->GetInProcJITEntryPointInfo()->GetJitTransferData()->SetRawData(func->GetTransferDataAllocator()->Finalize());
#if !FLOATVAR
        CodeGenNumberChunk * numberChunks = func->GetNumberAllocator()->Finalize();
        func->GetInProcJITEntryPointInfo()->SetNumberChunks(numberChunks);
#endif
    }
}

JITOutputIDL *
JITOutput::GetOutputData()
{
    return m_outputData;
}

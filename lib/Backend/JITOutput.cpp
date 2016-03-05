//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITOutput::JITOutput(JITOutputData * outputData) :
    m_outputData(outputData)
{
    m_outputData->writeableEPData.hasJittedStackClosure = false;
    // TODO: (michhol) validate initial values
    m_outputData->writeableEPData.localVarSlotsOffset = 0;
    m_outputData->writeableEPData.localVarChangedOffset = 0;
}

void
JITOutput::SetHasJITStackClosure()
{
    m_outputData->writeableEPData.hasJittedStackClosure = true;
}

void
JITOutput::SetVarSlotsOffset(int32 offset)
{
    m_outputData->writeableEPData.localVarSlotsOffset = offset;
}

void
JITOutput::SetVarChangedOffset(int32 offset)
{
    m_outputData->writeableEPData.localVarChangedOffset = offset;
}

intptr_t
JITOutput::GetCodeAddress() const
{
    return m_outputData->codeAddress;
}

EmitBufferAllocation *
JITOutput::RecordNativeCodeSize(Func *func, size_t bytes, ushort pdataCount, ushort xdataSize)
{
    BYTE *buffer;

    EmitBufferAllocation *allocation = func->GetEmitBufferManager()->AllocateBuffer(bytes, &buffer, pdataCount, xdataSize, false, true);

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
    if (!func->GetEmitBufferManager()->CommitBuffer(alloc, (BYTE *)alloc->allocation->address, alloc->allocation->size, sourceBuffer))
    {
        Js::Throw::OutOfMemory();
    }

#if DBG_DUMP
    if (func->IsLoopBody())
    {
        func->GetEmitBufferManager()->totalBytesLoopBody += alloc->allocation->size;
    }
#endif
}

#if _M_X64
size_t
JITOutput::RecordUnwindInfo(size_t offset, BYTE *unwindInfo, size_t size, BYTE * xdataAddr, HANDLE processHandle)
{
    Assert(offset == 0);
    Assert(XDATA_SIZE >= size);
    ChakraMemCopy(xdataAddr, XDATA_SIZE, unwindInfo, size, processHandle);
    return 0;
}
#elif _M_ARM
size_t
JITOutput::RecordUnwindInfo(size_t offset, BYTE *unwindInfo, size_t size, BYTE * xdataAddr, HANDLE processHandle)
{
    BYTE *xdataFinal = xdataAddr + offset;

    Assert(xdataFinal);
    Assert(((DWORD)xdataFinal & 0x3) == 0); // 4 byte aligned
    ChakraMemCopy(xdataFinal, size, unwindInfo, size, processHandle);
    return (size_t)xdataFinal;
}
#endif

void
JITOutput::FinalizeNativeCode(Func *func, EmitBufferAllocation * alloc)
{
    func->GetEmitBufferManager()->CompletePreviousAllocation(alloc);
}

JITOutputData *
JITOutput::GetOutputData()
{
    return m_outputData;
}

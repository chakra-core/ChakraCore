//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITOutput::JITOutput(JITOutputIDL * outputData) :
    m_outputData(outputData),
    m_inProcAlloc(nullptr),
    m_func(nullptr)
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

bool
JITOutput::IsTrackCompoundedIntOverflowDisabled() const
{
    return m_outputData->disableTrackCompoundedIntOverflow != FALSE;
}

bool
JITOutput::IsArrayCheckHoistDisabled() const
{
    return m_outputData->disableArrayCheckHoist != FALSE;
}

bool
JITOutput::IsStackArgOptDisabled() const
{
    return m_outputData->disableStackArgOpt != FALSE;
}

bool
JITOutput::IsSwitchOptDisabled() const
{
    return m_outputData->disableSwitchOpt != FALSE;
}

bool
JITOutput::IsAggressiveIntTypeSpecDisabled() const
{
    return m_outputData->disableAggressiveIntTypeSpec != FALSE;
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

EmitBufferAllocation<VirtualAllocWrapper, PreReservedVirtualAllocWrapper> *
JITOutput::RecordInProcNativeCodeSize(Func *func, uint32 bytes, ushort pdataCount, ushort xdataSize)
{
    m_func = func;

#if defined(_M_ARM32_OR_ARM64)
    bool canAllocInPreReservedHeapPageSegment = false;
#else
    bool canAllocInPreReservedHeapPageSegment = m_func->CanAllocInPreReservedHeapPageSegment();
#endif

    BYTE *buffer = nullptr;
    m_inProcAlloc = m_func->GetInProcCodeGenAllocators()->emitBufferManager.AllocateBuffer(bytes, &buffer, pdataCount, xdataSize, canAllocInPreReservedHeapPageSegment, true);

    if (buffer == nullptr)
    {
        Js::Throw::OutOfMemory();
    }
    m_outputData->codeAddress = (intptr_t)buffer;
    m_outputData->codeSize = bytes;
    m_outputData->pdataCount = pdataCount;
    m_outputData->xdataSize = xdataSize;
    m_outputData->isInPrereservedRegion = m_inProcAlloc->inPrereservedRegion;
    return m_inProcAlloc;
}

#if ENABLE_OOP_NATIVE_CODEGEN
EmitBufferAllocation<SectionAllocWrapper, PreReservedSectionAllocWrapper> *
JITOutput::RecordOOPNativeCodeSize(Func *func, uint32 bytes, ushort pdataCount, ushort xdataSize)
{
    m_func = func;

#if defined(_M_ARM32_OR_ARM64)
    bool canAllocInPreReservedHeapPageSegment = false;
#else
    bool canAllocInPreReservedHeapPageSegment = m_func->CanAllocInPreReservedHeapPageSegment();
#endif

    BYTE *buffer = nullptr;
    m_oopAlloc = m_func->GetOOPCodeGenAllocators()->emitBufferManager.AllocateBuffer(bytes, &buffer, pdataCount, xdataSize, canAllocInPreReservedHeapPageSegment, true);

    if (buffer == nullptr)
    {
        Js::Throw::OutOfMemory();
    }

    m_outputData->codeAddress = (intptr_t)buffer;
    m_outputData->codeSize = bytes;
    m_outputData->pdataCount = pdataCount;
    m_outputData->xdataSize = xdataSize;
    m_outputData->isInPrereservedRegion = m_oopAlloc->inPrereservedRegion;
    return m_oopAlloc;
}
#endif

void
JITOutput::RecordNativeCode(const BYTE* sourceBuffer, BYTE* localCodeAddress)
{
#if ENABLE_OOP_NATIVE_CODEGEN
    if (JITManager::GetJITManager()->IsJITServer())
    {
        RecordNativeCode(sourceBuffer, localCodeAddress, m_oopAlloc, m_func->GetOOPCodeGenAllocators());
    }
    else
#endif
    {
        RecordNativeCode(sourceBuffer, localCodeAddress, m_inProcAlloc, m_func->GetInProcCodeGenAllocators());
    }
}

template <typename TEmitBufferAllocation, typename TCodeGenAllocators>
void
JITOutput::RecordNativeCode(const BYTE* sourceBuffer, BYTE* localCodeAddress, TEmitBufferAllocation allocation, TCodeGenAllocators codeGenAllocators)
{
    Assert(m_outputData->codeAddress == (intptr_t)allocation->allocation->address);
    if (!codeGenAllocators->emitBufferManager.CommitBuffer(allocation, localCodeAddress, m_outputData->codeSize, sourceBuffer))
    {
        Js::Throw::OutOfMemory();
    }

#if DBG_DUMP
    if (m_func->IsLoopBody())
    {
        codeGenAllocators->emitBufferManager.totalBytesLoopBody += m_outputData->codeSize;
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
void
JITOutput::RecordUnwindInfo(BYTE *unwindInfo, size_t size, BYTE * xdataAddr, BYTE* localXdataAddr)
{
    Assert(XDATA_SIZE >= size);
    memcpy_s(localXdataAddr, XDATA_SIZE, unwindInfo, size);
    m_outputData->xdataAddr = (intptr_t)xdataAddr;
}

#elif _M_ARM
size_t
JITOutput::RecordUnwindInfo(size_t offset, BYTE *unwindInfo, size_t size, BYTE * xdataAddr)
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
JITOutput::FinalizeNativeCode()
{
#if ENABLE_OOP_NATIVE_CODEGEN
    if (JITManager::GetJITManager()->IsJITServer())
    {
        m_func->GetOOPCodeGenAllocators()->emitBufferManager.CompletePreviousAllocation(m_oopAlloc);
#if defined(_CONTROL_FLOW_GUARD)
#if _M_IX86 || _M_X64
        if (!m_func->IsLoopBody() && CONFIG_FLAG(UseJITTrampoline))
        {
            m_outputData->thunkAddress = m_func->GetOOPThreadContext()->GetJITThunkEmitter()->CreateThunk(m_outputData->codeAddress);
        }
#endif
#endif
    }
    else
#endif
    {
        m_func->GetInProcCodeGenAllocators()->emitBufferManager.CompletePreviousAllocation(m_inProcAlloc);
        m_func->GetInProcJITEntryPointInfo()->SetInProcJITNativeCodeData(m_func->GetNativeCodeDataAllocator()->Finalize());
        m_func->GetInProcJITEntryPointInfo()->GetJitTransferData()->SetRawData(m_func->GetTransferDataAllocator()->Finalize());
#if !FLOATVAR
        CodeGenNumberChunk * numberChunks = m_func->GetNumberAllocator()->Finalize();
        m_func->GetInProcJITEntryPointInfo()->SetNumberChunks(numberChunks);
#endif

#if defined(_CONTROL_FLOW_GUARD)
#if _M_IX86 || _M_X64
        if (!m_func->IsLoopBody() && CONFIG_FLAG(UseJITTrampoline))
        {
            m_outputData->thunkAddress = m_func->GetInProcThreadContext()->GetJITThunkEmitter()->CreateThunk(m_outputData->codeAddress);
        }
#endif
#endif
    }

    if (!m_outputData->thunkAddress && CONFIG_FLAG(OOPCFGRegistration))
    {
        m_func->GetThreadContextInfo()->SetValidCallTargetForCFG((PVOID)m_outputData->codeAddress);
    }
}

JITOutputIDL *
JITOutput::GetOutputData()
{
    return m_outputData;
}

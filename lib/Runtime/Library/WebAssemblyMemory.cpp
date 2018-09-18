//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "WasmLimits.h"

using namespace Js;

WebAssemblyMemory::WebAssemblyMemory(ArrayBufferBase* buffer, uint32 initial, uint32 maximum, DynamicType * type) :
    DynamicObject(type),
    m_buffer(buffer),
    m_initial(initial),
    m_maximum(maximum)
{
    Assert(buffer->IsWebAssemblyArrayBuffer());
    Assert(m_buffer);
    Assert(m_buffer->GetByteLength() >= UInt32Math::Mul<WebAssembly::PageSize>(initial));
}


_Must_inspect_result_ bool WebAssemblyMemory::AreLimitsValid(uint32 initial, uint32 maximum)
{
    return initial <= maximum && initial <= Wasm::Limits::GetMaxMemoryInitialPages() && maximum <= Wasm::Limits::GetMaxMemoryMaximumPages();
}


_Must_inspect_result_ bool WebAssemblyMemory::AreLimitsValid(uint32 initial, uint32 maximum, uint32 bufferLength)
{
    if (!AreLimitsValid(initial, maximum))
    {
        return false;
    }
    // Do the mul after initial checks to avoid potential unneeded OOM exception
    const uint32 initBytes = UInt32Math::Mul<WebAssembly::PageSize>(initial);
    const uint32 maxBytes = UInt32Math::Mul<WebAssembly::PageSize>(maximum);
    return initBytes <= bufferLength && bufferLength <= maxBytes;
}

Var
WebAssemblyMemory::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    AssertMsg(args.HasArg(), "Should always have implicit 'this'");

    Var newTarget = args.GetNewTarget();
    JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

    if (!(callInfo.Flags & CallFlags_New) || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("WebAssembly.Memory"));
    }

    if (args.Info.Count < 2 || !JavascriptOperators::IsObject(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("memoryDescriptor"));
    }
    DynamicObject * memoryDescriptor = VarTo<DynamicObject>(args[1]);

    Var initVar = JavascriptOperators::OP_GetProperty(memoryDescriptor, PropertyIds::initial, scriptContext);
    uint32 initial = WebAssembly::ToNonWrappingUint32(initVar, scriptContext);

    uint32 maximum = Wasm::Limits::GetMaxMemoryMaximumPages();
    bool hasMaximum = false;
    if (JavascriptOperators::OP_HasProperty(memoryDescriptor, PropertyIds::maximum, scriptContext))
    {
        hasMaximum = true;
        Var maxVar = JavascriptOperators::OP_GetProperty(memoryDescriptor, PropertyIds::maximum, scriptContext);
        maximum = WebAssembly::ToNonWrappingUint32(maxVar, scriptContext);
    }

    bool isShared = false;
    if (Wasm::Threads::IsEnabled() && JavascriptOperators::OP_HasProperty(memoryDescriptor, PropertyIds::shared, scriptContext))
    {
        if (!hasMaximum)
        {
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_SharedNoMaximum);
        }
        Var sharedVar = JavascriptOperators::OP_GetProperty(memoryDescriptor, PropertyIds::shared, scriptContext);
        isShared = JavascriptConversion::ToBool(sharedVar, scriptContext);
    }

    return CreateMemoryObject(initial, maximum, isShared, scriptContext);
}

Var
WebAssemblyMemory::EntryGrow(RecyclableObject* function, CallInfo callInfo, ...)
{
    ScriptContext* scriptContext = function->GetScriptContext();

    PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);

    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

    Assert(!(callInfo.Flags & CallFlags_New));

    if (!VarIs<WebAssemblyMemory>(args[0]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedMemoryObject);
    }

    WebAssemblyMemory* memory = VarTo<WebAssemblyMemory>(args[0]);
    Assert(VarIsCorrectType(memory->m_buffer));

    Var deltaVar = scriptContext->GetLibrary()->GetUndefined();
    if (args.Info.Count >= 2)
    {
        deltaVar = args[1];
    }

    uint32 deltaPages = WebAssembly::ToNonWrappingUint32(deltaVar, scriptContext);
    if (memory->m_buffer->IsDetached())
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray);
    }

    int32 oldPageCount = memory->GrowInternal(deltaPages);
    if (oldPageCount == -1)
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }

    return JavascriptNumber::ToVar(oldPageCount, scriptContext);
}

int32
WebAssemblyMemory::GrowInternal(uint32 deltaPages)
{
    const uint64 deltaBytes = (uint64)deltaPages * WebAssembly::PageSize;
    if (deltaBytes > ArrayBuffer::MaxArrayBufferLength)
    {
        return -1;
    }
    const uint32 oldBytes = m_buffer->GetByteLength();
    const uint64 newBytesLong = deltaBytes + oldBytes;
    if (newBytesLong > ArrayBuffer::MaxArrayBufferLength)
    {
        return -1;
    }
    CompileAssert(ArrayBuffer::MaxArrayBufferLength <= UINT32_MAX);
    const uint32 newBytes = (uint32)newBytesLong;

    const uint32 oldPageCount = oldBytes / WebAssembly::PageSize;
    Assert(oldBytes % WebAssembly::PageSize == 0);

    const uint32 newPageCount = oldPageCount + deltaPages;
    if (newPageCount > m_maximum)
    {
        return -1;
    }

    AssertOrFailFast(m_buffer->IsWebAssemblyArrayBuffer());
#ifdef ENABLE_WASM_THREADS
    if (m_buffer->IsSharedArrayBuffer())
    {
        AssertOrFailFast(Wasm::Threads::IsEnabled());
        if (!((WebAssemblySharedArrayBuffer*)GetBuffer())->GrowMemory(newBytes))
        {
            return -1;
        }
    }
    else
#endif
    {
        JavascriptExceptionObject* caughtExceptionObject = nullptr;
        try
        {
            WebAssemblyArrayBuffer* newBuffer = ((WebAssemblyArrayBuffer*)GetBuffer())->GrowMemory(newBytes);
            if (newBuffer == nullptr)
            {
                return -1;
            }
            m_buffer = newBuffer;
        }
        catch (const JavascriptException& err)
        {
            caughtExceptionObject = err.GetAndClear();
            Assert(caughtExceptionObject && caughtExceptionObject == ThreadContext::GetContextForCurrentThread()->GetPendingOOMErrorObject());
            return -1;
        }
    }

    CompileAssert(ArrayBuffer::MaxArrayBufferLength / WebAssembly::PageSize <= INT32_MAX);
    return (int32)oldPageCount;
}

int32
WebAssemblyMemory::GrowHelper(WebAssemblyMemory * mem, uint32 deltaPages)
{
    JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_GrowWasmMemory);
    return mem->GrowInternal(deltaPages);
    JIT_HELPER_END(Op_GrowWasmMemory);
}

#if DBG
void WebAssemblyMemory::TraceMemWrite(WebAssemblyMemory* mem, uint32 index, uint32 offset, Js::ArrayBufferView::ViewType viewType, uint32 bytecodeOffset, ScriptContext* context)
{
    JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_WasmMemoryTraceWrite);
    // Must call after the write
    Assert(mem);
    Output::Print(_u("#%04x "), bytecodeOffset);
    uint64 bigIndex = (uint64)index + (uint64)offset;
    if (index >= mem->m_buffer->GetByteLength())
    {
        Output::Print(_u("WasmMemoryTrace:: Writing out of bounds. %llu >= %u\n"), bigIndex, mem->m_buffer->GetByteLength());
    }
    if (offset)
    {
        Output::Print(_u("WasmMemoryTrace:: buf[%u + %u (%llu)]"), index, offset, bigIndex);
    }
    else
    {
        Output::Print(_u("WasmMemoryTrace:: buf[%u]"), index);
    }
    BYTE* buffer = mem->m_buffer->GetBuffer();
    switch (viewType)
    {
    case ArrayBufferView::ViewType::TYPE_INT8_TO_INT64:
    case ArrayBufferView::ViewType::TYPE_INT8: Output::Print(_u(".int8 = %d\n"), *(int8*)(buffer + bigIndex)); break;
    case ArrayBufferView::ViewType::TYPE_UINT8_TO_INT64:
    case ArrayBufferView::ViewType::TYPE_UINT8: Output::Print(_u(".uint8 = %u\n"), *(uint8*)(buffer + bigIndex)); break;
    case ArrayBufferView::ViewType::TYPE_INT16_TO_INT64:
    case ArrayBufferView::ViewType::TYPE_INT16: Output::Print(_u(".int16 = %d\n"), *(int16*)(buffer + bigIndex)); break;
    case ArrayBufferView::ViewType::TYPE_UINT16_TO_INT64:
    case ArrayBufferView::ViewType::TYPE_UINT16: Output::Print(_u(".uint16 = %u\n"), *(uint16*)(buffer + bigIndex)); break;
    case ArrayBufferView::ViewType::TYPE_INT32_TO_INT64:
    case ArrayBufferView::ViewType::TYPE_INT32: Output::Print(_u(".int32 = %d\n"), *(int32*)(buffer + bigIndex)); break;
    case ArrayBufferView::ViewType::TYPE_UINT32_TO_INT64:
    case ArrayBufferView::ViewType::TYPE_UINT32: Output::Print(_u(".uint32 = %u\n"), *(uint32*)(buffer + bigIndex)); break;
    case ArrayBufferView::ViewType::TYPE_FLOAT32: Output::Print(_u(".f32 = %.4f\n"), *(float*)(buffer + bigIndex)); break;
    case ArrayBufferView::ViewType::TYPE_FLOAT64: Output::Print(_u(".f64 = %.8f\n"), *(double*)(buffer + bigIndex)); break;
    case ArrayBufferView::ViewType::TYPE_INT64: Output::Print(_u(".int64 = %lld\n"), *(int64*)(buffer + bigIndex)); break;
    default:
        CompileAssert(ArrayBufferView::ViewType::TYPE_COUNT == 15);
        Assert(UNREACHED);
    }
    return;
    JIT_HELPER_END(Op_WasmMemoryTraceWrite);
}
#endif

Var
WebAssemblyMemory::EntryGetterBuffer(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count == 0 || !VarIs<WebAssemblyMemory>(args[0]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedMemoryObject);
    }

    WebAssemblyMemory* memory = VarTo<WebAssemblyMemory>(args[0]);
    Assert(VarIsCorrectType(memory->m_buffer));
    return CrossSite::MarshalVar(scriptContext, memory->m_buffer);
}

WebAssemblyMemory *
WebAssemblyMemory::CreateMemoryObject(uint32 initial, uint32 maximum, bool isShared, ScriptContext * scriptContext)
{
    if (!AreLimitsValid(initial, maximum))
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }
    uint32 byteLength = UInt32Math::Mul<WebAssembly::PageSize>(initial);
    ArrayBufferBase* buffer = nullptr;
#ifdef ENABLE_WASM_THREADS
    if (isShared)
    {
        Assert(Wasm::Threads::IsEnabled());
        uint32 maxByteLength = UInt32Math::Mul<WebAssembly::PageSize>(maximum);
        buffer = scriptContext->GetLibrary()->CreateWebAssemblySharedArrayBuffer(byteLength, maxByteLength);
    }
    else
#endif
    {
        buffer = scriptContext->GetLibrary()->CreateWebAssemblyArrayBuffer(byteLength);
    }
    Assert(buffer);
    if (byteLength > 0 && buffer->GetByteLength() == 0)
    {
        // Failed to allocate buffer
        return nullptr;
    }
    return RecyclerNewFinalized(scriptContext->GetRecycler(), WebAssemblyMemory, buffer, initial, maximum, scriptContext->GetLibrary()->GetWebAssemblyMemoryType());
}

WebAssemblyMemory * WebAssemblyMemory::CreateForExistingBuffer(uint32 initial, uint32 maximum, uint32 currentByteLength, ScriptContext * scriptContext)
{
    if (!AreLimitsValid(initial, maximum, currentByteLength))
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }
    ArrayBufferBase* buffer = scriptContext->GetLibrary()->CreateWebAssemblyArrayBuffer(currentByteLength);
    Assert(buffer);
    if (currentByteLength > 0 && buffer->GetByteLength() == 0)
    {
        // Failed to allocate buffer
        return nullptr;
    }
    return RecyclerNewFinalized(scriptContext->GetRecycler(), WebAssemblyMemory, buffer, initial, maximum, scriptContext->GetLibrary()->GetWebAssemblyMemoryType());
}

#ifdef ENABLE_WASM_THREADS
WebAssemblyMemory * WebAssemblyMemory::CreateFromSharedContents(uint32 initial, uint32 maximum, SharedContents* sharedContents, ScriptContext * scriptContext)
{
    if (!sharedContents || !AreLimitsValid(initial, maximum, sharedContents->bufferLength))
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }
    ArrayBufferBase* buffer = scriptContext->GetLibrary()->CreateWebAssemblySharedArrayBuffer(sharedContents);
    return RecyclerNewFinalized(scriptContext->GetRecycler(), WebAssemblyMemory, buffer, initial, maximum, scriptContext->GetLibrary()->GetWebAssemblyMemoryType());
}
#endif

ArrayBufferBase*
WebAssemblyMemory::GetBuffer() const
{
    return m_buffer;
}

uint
WebAssemblyMemory::GetInitialLength() const
{
    return m_initial;
}

uint
WebAssemblyMemory::GetMaximumLength() const
{
    return m_maximum;
}

uint
WebAssemblyMemory::GetCurrentMemoryPages() const
{
    return m_buffer->GetByteLength() / WebAssembly::PageSize;
}

#ifdef ENABLE_WASM_THREADS
bool WebAssemblyMemory::IsSharedMemory() const
{
    return VarIs<WebAssemblySharedArrayBuffer>(m_buffer);
}
#endif

#endif // ENABLE_WASM

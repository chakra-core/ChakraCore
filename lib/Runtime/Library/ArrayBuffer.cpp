//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    long RefCountedBuffer::AddRef()
    {
        long ref = InterlockedIncrement(&refCount);
        AssertOrFailFast(ref > 1);
        return ref;
    }

    long RefCountedBuffer::Release()
    {
        long ref = InterlockedDecrement(&refCount);
        AssertOrFailFastMsg(ref >= 0, "Buffer already freed");
        return ref;
    }

    void ArrayBufferDetachedStateBase::AddRefBufferContent()
    {
        if (buffer != nullptr)
        {
            buffer->AddRef();
        }
    }

    long ArrayBufferDetachedStateBase::ReleaseRefBufferContent()
    {
        if (buffer != nullptr)
        {
            long ref = buffer->Release();
            AssertOrFailFast(ref > 0);
            return ref;
        }
        return 0;
    }

    template <> bool VarIsImpl<ArrayBufferBase>(RecyclableObject* obj)
    {
        return VarIs<ArrayBuffer>(obj) || VarIs<SharedArrayBuffer>(obj);
    }

    ArrayBuffer* ArrayBuffer::NewFromDetachedState(DetachedStateBase* state, JavascriptLibrary *library)
    {
        ArrayBufferDetachedStateBase* arrayBufferState = (ArrayBufferDetachedStateBase *)state;
        ArrayBuffer *toReturn = nullptr;

        switch (arrayBufferState->allocationType)
        {
        case ArrayBufferAllocationType::CoTask:
            toReturn = library->CreateProjectionArraybuffer(arrayBufferState->buffer, arrayBufferState->bufferLength);
            break;
        case ArrayBufferAllocationType::Heap:
        case ArrayBufferAllocationType::MemAlloc:
            toReturn = library->CreateArrayBuffer(arrayBufferState->buffer, arrayBufferState->bufferLength);
            break;
        case ArrayBufferAllocationType::External:
            toReturn = static_cast<ExternalArrayBufferDetachedState*>(state)->Create(library);
            break;
        default:
            AssertMsg(false, "Unknown allocationType of ArrayBufferDetachedStateBase ");
        }

        return toReturn;
    }

    uint32 ArrayBuffer::GetByteLength() const
    {
        return this->bufferLength;
    }

    BYTE* ArrayBuffer::GetBuffer() const
    {
        return this->bufferContent != nullptr ? this->bufferContent->GetBuffer() : nullptr;
    }

    void ArrayBuffer::DetachBufferFromParent(ArrayBufferParent* parent)
    {
        if (parent == nullptr)
        {
            return;
        }

        switch (JavascriptOperators::GetTypeId(parent))
        {
        case TypeIds_Int8Array:
                if (VarIs<Int8VirtualArray>(parent))
                {
                    if (VirtualTableInfo<Int8VirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Int8Array>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Int8VirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Int8Array>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Uint8Array:
                if (VarIs<Uint8VirtualArray>(parent))
                {
                    if (VirtualTableInfo<Uint8VirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Uint8Array>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Uint8VirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Uint8Array>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Uint8ClampedArray:
                if (VarIs<Uint8ClampedVirtualArray>(parent))
                {
                    if (VirtualTableInfo<Uint8ClampedVirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Uint8ClampedArray>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Uint8ClampedVirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Uint8ClampedArray>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Int16Array:
                if (VarIs<Int16VirtualArray>(parent))
                {
                    if (VirtualTableInfo<Int16VirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Int16Array>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Int16VirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Int16Array>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Uint16Array:
                if (VarIs<Uint16VirtualArray>(parent))
                {
                    if (VirtualTableInfo<Uint16VirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Uint16Array>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Uint16VirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Uint16Array>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Int32Array:
                if (VarIs<Int32VirtualArray>(parent))
                {
                    if (VirtualTableInfo<Int32VirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Int32Array>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Int32VirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Int32Array>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Uint32Array:
                if (VarIs<Uint32VirtualArray>(parent))
                {
                    if (VirtualTableInfo<Uint32VirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Uint32Array>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Uint32VirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Uint32Array>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Float32Array:
                if (VarIs<Float32VirtualArray>(parent))
                {
                    if (VirtualTableInfo<Float32VirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Float32Array>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Float32VirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Float32Array>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Float64Array:
                if (VarIs<Float64VirtualArray>(parent))
                {
                    if (VirtualTableInfo<Float64VirtualArray>::HasVirtualTable(parent))
                    {
                        VirtualTableInfo<Float64Array>::SetVirtualTable(parent);
                    }
                    else
                    {
                        Assert(VirtualTableInfo<CrossSiteObject<Float64VirtualArray>>::HasVirtualTable(parent));
                        VirtualTableInfo<CrossSiteObject<Float64Array>>::SetVirtualTable(parent);
                    }
                }
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Int64Array:
        case TypeIds_Uint64Array:
        case TypeIds_CharArray:
        case TypeIds_BoolArray:
                UnsafeVarTo<TypedArrayBase>(parent)->ClearLengthAndBufferOnDetach();
            break;

        case TypeIds_DataView:
                VarTo<DataView>(parent)->ClearLengthAndBufferOnDetach();
            break;

        default:
            AssertMsg(false, "We need an explicit case for any parent of ArrayBuffer.");
            break;
        }
    }

    void ArrayBuffer::ReportExternalMemoryFree()
    {
        Recycler* recycler = GetType()->GetLibrary()->GetRecycler();
        recycler->ReportExternalMemoryFree(bufferLength);
    }

    void ArrayBuffer::Detach()
    {
        Assert(!this->isDetached);

        // we are about to lose track of the buffer to another owner
        // report that we no longer own the memory
        ReportExternalMemoryFree();

        this->bufferContent = nullptr;
        this->bufferLength = 0;
        this->isDetached = true;

        if (this->primaryParent != nullptr && this->primaryParent->Get() == nullptr)
        {
            this->primaryParent = nullptr;
        }

        if (this->primaryParent != nullptr)
        {
            this->DetachBufferFromParent(this->primaryParent->Get());
        }

        if (this->otherParents != nullptr)
        {
            this->otherParents->Map([&](RecyclerWeakReference<ArrayBufferParent>* item)
            {
                this->DetachBufferFromParent(item->Get());
            });
        }
    }

    ArrayBufferDetachedStateBase* ArrayBuffer::DetachAndGetState(bool queueForDelayFree/* = true*/)
    {
        // Save the state before detaching
        AutoPtr<ArrayBufferDetachedStateBase> arrayBufferState(this->CreateDetachedState(this->bufferContent, this->bufferLength));
        Detach();

        // Now put this bufferContent to the queue so that we can manage the lifetime of the buffer later.
        if (queueForDelayFree && arrayBufferState->buffer != nullptr)
        {
            DelayedFreeArrayBuffer * local = GetScriptContext()->GetThreadContext()->GetScanStackCallback();
            local->Push(this->CopyBufferContentForDelayedFree(arrayBufferState->buffer, arrayBufferState->bufferLength));
        }

        return arrayBufferState.Detach();
    }

    void ArrayBuffer::AddParent(ArrayBufferParent* parent)
    {
        if (this->primaryParent == nullptr || this->primaryParent->Get() == nullptr)
        {
            this->primaryParent = this->GetRecycler()->CreateWeakReferenceHandle(parent);
        }
        else
        {
            if (this->otherParents == nullptr)
            {
                this->otherParents = RecyclerNew(this->GetRecycler(), OtherParents, this->GetRecycler());
            }

            if (this->otherParents->increasedCount >= ParentsCleanupThreshold)
            {
                auto iter = this->otherParents->GetEditingIterator();
                while (iter.Next())
                {
                    if (iter.Data()->Get() == nullptr)
                    {
                        iter.RemoveCurrent();
                    }
                }

                this->otherParents->increasedCount = 0;
            }

            this->otherParents->PrependNode(this->GetRecycler()->CreateWeakReferenceHandle(parent));
            this->otherParents->increasedCount++;
        }
    }

    ArrayBuffer * ArrayBuffer::GetAsArrayBuffer()
    {
        AssertOrFailFast(VarIsCorrectType(this));
        return this;
    }

    uint32 ArrayBuffer::ToIndex(Var value, int32 errorCode, ScriptContext *scriptContext, uint32 MaxAllowedLength, bool checkSameValueZero)
    {
        if (JavascriptOperators::IsUndefined(value))
        {
            return 0;
        }

        if (TaggedInt::Is(value))
        {
            int64 index = TaggedInt::ToInt64(value);
            if (index < 0 || index >(int64)MaxAllowedLength)
            {
                JavascriptError::ThrowRangeError(scriptContext, errorCode);
            }

            return  (uint32)index;
        }

        // Slower path

        double d = JavascriptConversion::ToInteger(value, scriptContext);
        if (d < 0.0 || d >(double)MaxAllowedLength)
        {
            JavascriptError::ThrowRangeError(scriptContext, errorCode);
        }

        if (checkSameValueZero)
        {
            Var integerIndex = JavascriptNumber::ToVarNoCheck(d, scriptContext);
            Var index = JavascriptNumber::ToVar(JavascriptConversion::ToLength(integerIndex, scriptContext), scriptContext);
            if (!JavascriptConversion::SameValueZero(integerIndex, index))
            {
                JavascriptError::ThrowRangeError(scriptContext, errorCode);
            }
        }

        return (uint32)d;
    }

    Var ArrayBuffer::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

        if (!args.IsNewCall() || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("ArrayBuffer"));
        }

        uint32 byteLength = 0;
        if (args.Info.Count > 1)
        {
            byteLength = ToIndex(args[1], JSERR_ArrayLengthConstructIncorrect, scriptContext, MaxArrayBufferLength);
        }

        RecyclableObject* newArr = scriptContext->GetLibrary()->CreateArrayBuffer(byteLength);
        Assert(VarIs<ArrayBuffer>(newArr));
        if (byteLength > 0 && !VarTo<ArrayBuffer>(newArr)->GetByteLength())
        {
            JavascriptError::ThrowRangeError(scriptContext, JSERR_FunctionArgument_Invalid);
        }
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            newArr = Js::JavascriptProxy::AutoProxyWrapper(newArr);
        }
#endif
        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), newArr, nullptr, scriptContext) :
            newArr;
    }

    // ArrayBuffer.prototype.byteLength as described in ES6 draft #20 section 24.1.4.1
    Var ArrayBuffer::EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<ArrayBuffer>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }

        ArrayBuffer* arrayBuffer = VarTo<ArrayBuffer>(args[0]);
        if (arrayBuffer->IsDetached())
        {
            return JavascriptNumber::ToVar(0, scriptContext);
        }
        return JavascriptNumber::ToVar(arrayBuffer->GetByteLength(), scriptContext);
    }

    // ArrayBuffer.isView as described in ES6 draft #20 section 24.1.3.1
    Var ArrayBuffer::EntryIsView(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        Assert(!(callInfo.Flags & CallFlags_New));

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        JavascriptLibrary* library = function->GetScriptContext()->GetLibrary();

        Var arg = library->GetUndefined();

        if (args.Info.Count > 1)
        {
            arg = args[1];
        }

        // Only DataView or any TypedArray objects have [[ViewedArrayBuffer]] internal slots
        if (VarIs<DataView>(arg) || VarIs<TypedArrayBase>(arg))
        {
            return library->GetTrue();
        }

        return library->GetFalse();
    }

#if ENABLE_DEBUG_CONFIG_OPTIONS
    Var ArrayBuffer::EntryDetach(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2 || !VarIs<ArrayBuffer>(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }

        ArrayBuffer* arrayBuffer = VarTo<ArrayBuffer>(args[1]);

        if (arrayBuffer->IsDetached())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("ArrayBuffer.detach"));
        }

        // Discard the buffer
        // We are clearing out the buffer now instead of queueing to flush out JIT bugs.
        DetachedStateBase* state = arrayBuffer->DetachAndGetState(false /*queueForDelayFree*/);
        state->CleanUp();

        return scriptContext->GetLibrary()->GetUndefined();
    }
#endif

    // ArrayBuffer.prototype.slice as described in ES6 draft #19 section 24.1.4.3.
    Var ArrayBuffer::EntrySlice(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Assert(!(callInfo.Flags & CallFlags_New));

        if (!VarIs<ArrayBuffer>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        ArrayBuffer* arrayBuffer = VarTo<ArrayBuffer>(args[0]);

        if (arrayBuffer->IsDetached()) // 24.1.4.3: 5. If IsDetachedBuffer(O) is true, then throw a TypeError exception.
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("ArrayBuffer.prototype.slice"));
        }

        int64 len = arrayBuffer->bufferLength;
        int64 start = 0, end = 0;
        int64 newLen;

        // If no start or end arguments, use the entire length
        if (args.Info.Count < 2)
        {
            newLen = len;
        }
        else
        {
            start = JavascriptArray::GetIndexFromVar(args[1], len, scriptContext);

            // If no end argument, use length as the end
            if (args.Info.Count < 3 || args[2] == library->GetUndefined())
            {
                end = len;
            }
            else
            {
                end = JavascriptArray::GetIndexFromVar(args[2], len, scriptContext);
            }

            newLen = end > start ? end - start : 0;
        }

        // We can't have allocated an ArrayBuffer with byteLength > MaxArrayBufferLength.
        // start and end are clamped to valid indices, so the new length also cannot exceed MaxArrayBufferLength.
        // Therefore, should be safe to cast down newLen to uint32.
        // TODO: If we ever support allocating ArrayBuffer with byteLength > MaxArrayBufferLength we may need to review this math.
        Assert(newLen < MaxArrayBufferLength);
        uint32 byteLength = static_cast<uint32>(newLen);

        ArrayBuffer* newBuffer = nullptr;

        if (scriptContext->GetConfig()->IsES6SpeciesEnabled())
        {
            JavascriptFunction* defaultConstructor = scriptContext->GetLibrary()->GetArrayBufferConstructor();
            RecyclableObject* constructor = JavascriptOperators::SpeciesConstructor(arrayBuffer, defaultConstructor, scriptContext);
            AssertOrFailFast(JavascriptOperators::IsConstructor(constructor));

            bool isDefaultConstructor = constructor == defaultConstructor;
            Js::Var newVar = JavascriptOperators::NewObjectCreationHelper_ReentrancySafe(constructor, isDefaultConstructor, scriptContext->GetThreadContext(), [=]()->Js::Var
            {
                Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(byteLength, scriptContext) };
                Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
                return JavascriptOperators::NewScObject(constructor, Js::Arguments(constructorCallInfo, constructorArgs), scriptContext);
            });

            if (!VarIs<ArrayBuffer>(newVar)) // 24.1.4.3: 19.If new does not have an [[ArrayBufferData]] internal slot throw a TypeError exception.
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
            }

            newBuffer = VarTo<ArrayBuffer>(newVar);

            if (newBuffer->IsDetached()) // 24.1.4.3: 21. If IsDetachedBuffer(new) is true, then throw a TypeError exception.
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("ArrayBuffer.prototype.slice"));
            }

            if (newBuffer == arrayBuffer) // 24.1.4.3: 22. If SameValue(new, O) is true, then throw a TypeError exception.
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
            }

            if (newBuffer->bufferLength < byteLength) // 24.1.4.3: 23.If the value of new's [[ArrayBufferByteLength]] internal slot < newLen, then throw a TypeError exception.
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_ArgumentOutOfRange, _u("ArrayBuffer.prototype.slice"));
            }
        }
        else
        {
            newBuffer = library->CreateArrayBuffer(byteLength);
        }

        Assert(newBuffer);
        Assert(newBuffer->bufferLength >= byteLength);

        if (arrayBuffer->IsDetached()) // 24.1.4.3: 24. NOTE: Side-effects of the above steps may have detached O. 25. If IsDetachedBuffer(O) is true, then throw a TypeError exception.
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("ArrayBuffer.prototype.slice"));
        }

        // Don't bother doing memcpy if we aren't copying any elements
        if (byteLength > 0)
        {
            AssertMsg(arrayBuffer->GetBuffer() != nullptr, "buffer must not be null when we copy from it");

            js_memcpy_s(newBuffer->GetBuffer(), byteLength, arrayBuffer->GetBuffer() + start, byteLength);
        }

        return newBuffer;
    }

    Var ArrayBuffer::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);

        Assert(args.Info.Count > 0);

        return args[0];
    }

    ArrayBufferContentForDelayedFreeBase* ArrayBuffer::CopyBufferContentForDelayedFree(RefCountedBuffer * content, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength)
    {
        Assert(content != nullptr);
        FreeFn* freeFn = nullptr;
#if ENABLE_FAST_ARRAYBUFFER
        if (IsValidVirtualBufferLength(bufferLength))
        {
            freeFn = FreeMemAlloc;
        }
        else
#endif
        {
            freeFn = free;
        }

        // This heap object will be deleted when the Recycler::DelayedFreeArrayBuffer determines to remove this item
        return HeapNew(ArrayBufferContentForDelayedFree<FreeFn>, content, bufferLength, GetScriptContext()->GetRecycler(), freeFn);
    }

    template <class Allocator>
    ArrayBuffer::ArrayBuffer(uint32 length, DynamicType * type, Allocator allocator) :
        ArrayBufferBase(type), bufferContent(nullptr), bufferLength(0)
    {
        if (length > MaxArrayBufferLength)
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_FunctionArgument_Invalid);
        }
        else if (length > 0)
        {
            BYTE * buffer = nullptr;
            Recycler* recycler = GetType()->GetLibrary()->GetRecycler();
            if (recycler->RequestExternalMemoryAllocation(length))
            {
                buffer = (BYTE*)allocator(length);
                if (buffer == nullptr)
                {
                    recycler->ReportExternalMemoryFree(length);
                }
            }

            if (buffer == nullptr)
            {
                recycler->CollectNow<CollectOnTypedArrayAllocation>();

                if (recycler->RequestExternalMemoryAllocation(length))
                {
                    buffer = (BYTE*)allocator(length);
                    if (buffer == nullptr)
                    {
                        recycler->ReportExternalMemoryFailure(length);
                    }
                }
            }

            if (buffer == nullptr)
            {
                JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
            }
            else
            {
                bufferLength = length;
                ZeroMemory(buffer, bufferLength);
                RefCountedBuffer* localContent = HeapNew(RefCountedBuffer, buffer);
                this->bufferContent = localContent;
            }
        }
    }

    ArrayBuffer::ArrayBuffer(RefCountedBuffer* buffContent, uint32 length, DynamicType * type)
       : bufferContent(nullptr), bufferLength(length), ArrayBufferBase(type)
    {
        if (length > MaxArrayBufferLength)
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_FunctionArgument_Invalid);
        }

        // we take the ownership of the buffer and will have to free it so charge it to our quota.
        if (!this->GetRecycler()->RequestExternalMemoryAllocation(length))
        {
            JavascriptError::ThrowOutOfMemoryError(this->GetScriptContext());
        }

        this->bufferContent = buffContent;

        // The bufferContent can be null as might have detached an ArrayBuffer which does not have bufferContent.
        if (this->bufferContent != nullptr)
        {
            this->bufferContent->AddRef();
        }
    }

    ArrayBuffer::ArrayBuffer(byte* buffer, uint32 length, DynamicType * type, bool isExternal) :
        bufferContent(nullptr), bufferLength(length), ArrayBufferBase(type)
    {
        if (length > MaxArrayBufferLength)
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_FunctionArgument_Invalid);
        }

        if (!isExternal)
        {
            // we take the ownership of the buffer and will have to free it so charge it to our quota.
            if (!this->GetRecycler()->RequestExternalMemoryAllocation(length))
            {
                JavascriptError::ThrowOutOfMemoryError(this->GetScriptContext());
            }
        }

        if (buffer != nullptr)
        {
            RefCountedBuffer* localContent = HeapNew(RefCountedBuffer, buffer);
            this->bufferContent = localContent;
        }
    }

    BOOL ArrayBuffer::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object, (ArrayBuffer)"));
        return TRUE;
    }

    BOOL ArrayBuffer::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("[object ArrayBuffer]"));
        return TRUE;
    }

    void ArrayBuffer::ReleaseBufferContent()
    {
        if (this->bufferContent != nullptr && this->bufferContent->GetBuffer() != nullptr)
        {
            RefCountedBuffer *content = this->bufferContent;
            this->bufferContent = nullptr;
            long refCount = content->Release();
            AssertOrFailFast(refCount == 0);
            HeapDelete(content);
        }
    }

#if ENABLE_TTD
    void ArrayBufferParent::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        extractor->MarkVisitVar(this->arrayBuffer);
    }

    void ArrayBufferParent::ProcessCorePaths()
    {
        this->GetScriptContext()->TTDWellKnownInfo->EnqueueNewPathVarAsNeeded(this, this->arrayBuffer, _u("!buffer"));
    }
#endif

    JavascriptArrayBuffer::JavascriptArrayBuffer(uint32 length, DynamicType * type) :
        ArrayBuffer(length, type, IsValidVirtualBufferLength(length) ? AsmJsVirtualAllocator : malloc)
    {
    }
    JavascriptArrayBuffer::JavascriptArrayBuffer(byte* buffer, uint32 length, DynamicType * type) :
        ArrayBuffer(buffer, length, type)
    {
    }

    JavascriptArrayBuffer::JavascriptArrayBuffer(RefCountedBuffer* buffer, uint32 length, DynamicType * type) :
        ArrayBuffer(buffer, length, type)
    {
    }


    JavascriptArrayBuffer::JavascriptArrayBuffer(DynamicType * type) : ArrayBuffer(0, type, malloc)
    {
    }

    JavascriptArrayBuffer* JavascriptArrayBuffer::Create(uint32 length, DynamicType * type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        JavascriptArrayBuffer* result = RecyclerNewFinalized(recycler, JavascriptArrayBuffer, length, type);
        Assert(result);
        recycler->AddExternalMemoryUsage(length);
        return result;
    }

    JavascriptArrayBuffer* JavascriptArrayBuffer::Create(byte* buffer, uint32 length, DynamicType * type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        JavascriptArrayBuffer* result = RecyclerNewFinalized(recycler, JavascriptArrayBuffer, buffer, length, type);
        Assert(result);
        recycler->AddExternalMemoryUsage(length);
        return result;
    }

    JavascriptArrayBuffer* JavascriptArrayBuffer::Create(RefCountedBuffer* content, uint32 length, DynamicType * type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        JavascriptArrayBuffer* result = RecyclerNewFinalized(recycler, JavascriptArrayBuffer, content, length, type);
        Assert(result);
        recycler->AddExternalMemoryUsage(length);
        return result;
    }

    ArrayBufferDetachedStateBase* JavascriptArrayBuffer::CreateDetachedState(RefCountedBuffer * content, uint32 bufferLength)
    {
        FreeFn* freeFn = nullptr;
        ArrayBufferAllocationType allocationType;
#if ENABLE_FAST_ARRAYBUFFER
        if (IsValidVirtualBufferLength(bufferLength))
        {
            allocationType = ArrayBufferAllocationType::MemAlloc;
            freeFn = FreeMemAlloc;
        }
        else
#endif
        {
            allocationType = ArrayBufferAllocationType::Heap;
            freeFn = free;
        }
        return HeapNew(ArrayBufferDetachedState<FreeFn>, content, bufferLength, freeFn, GetScriptContext()->GetRecycler(), allocationType);
    }


    bool JavascriptArrayBuffer::IsValidAsmJsBufferLengthAlgo(uint length, bool forceCheck)
    {
        /*
        1. length >= 2^16
        2. length is power of 2 or (length > 2^24 and length is multiple of 2^24)
        3. length is a multiple of 4K
        */
        const bool isLongEnough = length >= 0x10000;
        const bool isPow2 = ::Math::IsPow2(length);
        // No need to check for length > 2^24, because it already has to be non zero length
        const bool isMultipleOf2e24 = (length & 0xFFFFFF) == 0;
        const bool isPageSizeMultiple = (length % AutoSystemInfo::PageSize) == 0;
        return (
#ifndef ENABLE_FAST_ARRAYBUFFER
            forceCheck &&
#endif
            isLongEnough &&
            (isPow2 || isMultipleOf2e24) &&
            isPageSizeMultiple
        );
    }

    bool JavascriptArrayBuffer::IsValidAsmJsBufferLength(uint length, bool forceCheck)
    {
        return IsValidAsmJsBufferLengthAlgo(length, forceCheck);
    }

    bool JavascriptArrayBuffer::IsValidVirtualBufferLength(uint length) const
    {
#if ENABLE_FAST_ARRAYBUFFER
        return PHASE_FORCE1(Js::TypedArrayVirtualPhase) || (!PHASE_OFF1(Js::TypedArrayVirtualPhase) && IsValidAsmJsBufferLengthAlgo(length, true));
#else
        return false;
#endif
    }

    void JavascriptArrayBuffer::Finalize(bool isShutdown)
    {
        if (this->bufferContent == nullptr)
        {
            return;
        }

        RefCountedBuffer *content = this->bufferContent;
        this->bufferContent = nullptr;
        long refCount = content->Release();
        if (refCount == 0)
        {
            BYTE * buffer = content->GetBuffer();
            if (buffer)
            {
                // Recycler may not be available at Dispose. We need to
                // free the memory and report that it has been freed at the same
                // time. Otherwise, AllocationPolicyManager is unable to provide correct feedback
#if ENABLE_FAST_ARRAYBUFFER
        //AsmJS Virtual Free
                if (buffer && IsValidVirtualBufferLength(this->bufferLength))
                {
                    FreeMemAlloc(buffer);
                }
                else
                {
                    free(buffer);
                }
#else
                free(buffer);
#endif
            }
            Recycler* recycler = GetType()->GetLibrary()->GetRecycler();
            recycler->ReportExternalMemoryFree(bufferLength);
            HeapDelete(content);
        }

        bufferLength = 0;
    }

    void JavascriptArrayBuffer::Dispose(bool isShutdown)
    {
        /* See JavascriptArrayBuffer::Finalize */
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType JavascriptArrayBuffer::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapArrayBufferObject;
    }

    void JavascriptArrayBuffer::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapArrayBufferInfo* sabi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapArrayBufferInfo>();

        sabi->Length = this->GetByteLength();
        if (sabi->Length == 0)
        {
            sabi->Buff = nullptr;
        }
        else
        {
            sabi->Buff = alloc.SlabAllocateArray<byte>(sabi->Length);
            memcpy(sabi->Buff, this->GetBuffer(), sabi->Length);
        }

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapArrayBufferInfo*, TTD::NSSnapObjects::SnapObjectType::SnapArrayBufferObject>(objData, sabi);
    }
#endif

#ifdef ENABLE_WASM
    // Same as realloc but zero newly allocated portion if newSize > oldSize
    static BYTE* ReallocZero(BYTE* ptr, size_t oldSize, size_t newSize)
    {
        BYTE* ptrNew = (BYTE*)realloc(ptr, newSize);
        if (ptrNew && newSize > oldSize)
        {
            ZeroMemory(ptrNew + oldSize, newSize - oldSize);
        }
        return ptrNew;
    }

    template<typename Allocator>
    Js::WebAssemblyArrayBuffer::WebAssemblyArrayBuffer(uint32 length, DynamicType * type, Allocator allocator):
        JavascriptArrayBuffer(length, type, allocator)
    {
#ifndef ENABLE_FAST_ARRAYBUFFER
        CompileAssert(UNREACHED);
#endif
        Assert(allocator == WasmVirtualAllocator);
        // Make sure we always have a buffer even if the length is 0
        if (bufferContent == nullptr && length == 0)
        {
            // We want to allocate an empty buffer using virtual memory
            BYTE *buffer = (BYTE*)allocator(0);
            if (buffer == nullptr)
            {
                JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
            }

            RefCountedBuffer* localContent = HeapNew(RefCountedBuffer, buffer);
            this->bufferContent = localContent;
        }
        if (bufferContent == nullptr)
        {
            JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
        }
    }

    // Treat as a normal JavascriptArrayBuffer
    WebAssemblyArrayBuffer::WebAssemblyArrayBuffer(uint32 length, DynamicType * type) :
        JavascriptArrayBuffer(length, type, malloc)
    {
        // Make sure we always have a bufferContent even if the length is 0
        if (bufferContent == nullptr && length == 0)
        {
            bufferContent = HeapNew(RefCountedBuffer, nullptr);
        }
        if (bufferContent == nullptr)
        {
            JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
        }
    }

    WebAssemblyArrayBuffer::WebAssemblyArrayBuffer(byte* buffer, uint32 length, DynamicType * type):
        JavascriptArrayBuffer(buffer, length, type)
    {
#if ENABLE_FAST_ARRAYBUFFER
        if (CONFIG_FLAG(WasmFastArray) && buffer == nullptr)
        {
            JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
        }
#endif
    }

    WebAssemblyArrayBuffer* WebAssemblyArrayBuffer::Create(byte* buffer, uint32 length, DynamicType * type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        WebAssemblyArrayBuffer* result = nullptr;
        if (buffer)
        {
            result = RecyclerNewFinalized(recycler, WebAssemblyArrayBuffer, buffer, length, type);
        }
        else
        {
#if ENABLE_FAST_ARRAYBUFFER
            if (CONFIG_FLAG(WasmFastArray))
            {
                result = RecyclerNewFinalized(recycler, WebAssemblyArrayBuffer, length, type, WasmVirtualAllocator);
            }
            else
#endif
            {
                result = RecyclerNewFinalized(recycler, WebAssemblyArrayBuffer, length, type);
            }
        }

        recycler->AddExternalMemoryUsage(length);
        Assert(result);
        return result;
    }

    bool WebAssemblyArrayBuffer::IsValidVirtualBufferLength(uint length) const
    {
#if ENABLE_FAST_ARRAYBUFFER
        return CONFIG_FLAG(WasmFastArray);
#else
        return false;
#endif
    }

    ArrayBufferDetachedStateBase* WebAssemblyArrayBuffer::CreateDetachedState(RefCountedBuffer* buffer, uint32 bufferLength)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), WASMERR_CantDetach);
    }

    WebAssemblyArrayBuffer* WebAssemblyArrayBuffer::GrowMemory(uint32 newBufferLength)
    {
        if (newBufferLength < this->bufferLength)
        {
            Assert(UNREACHED);
            JavascriptError::ThrowTypeError(GetScriptContext(), WASMERR_BufferGrowOnly);
        }

        uint32 growSize = newBufferLength - this->bufferLength;
        const auto finalizeGrowMemory = [&](WebAssemblyArrayBuffer* newArrayBuffer)
        {
            AssertOrFailFast(newArrayBuffer && newArrayBuffer->GetByteLength() == newBufferLength);
            RefCountedBuffer *local = this->GetBufferContent();
            // Detach the buffer from this ArrayBuffer
            this->Detach();
            if (local != nullptr)
            {
                HeapDelete(local);
            }
            return newArrayBuffer;
        };

        // We're not growing the buffer, just create a new WebAssemblyArrayBuffer and detach this
        if (growSize == 0)
        {
            return finalizeGrowMemory(this->GetLibrary()->CreateWebAssemblyArrayBuffer(this->GetBuffer(), this->bufferLength));
        }

#if ENABLE_FAST_ARRAYBUFFER
        // 8Gb Array case
        if (CONFIG_FLAG(WasmFastArray))
        {
            AssertOrFailFast(this->GetBuffer());
            const auto virtualAllocFunc = [&]
            {
                return !!VirtualAlloc(this->GetBuffer() + this->bufferLength, growSize, MEM_COMMIT, PAGE_READWRITE);
            };
            if (!this->GetRecycler()->DoExternalAllocation(growSize, virtualAllocFunc))
            {
                return nullptr;
            }

            // We are transferring the buffer to the new owner.
            // To avoid double-charge to the allocation quota we will free the "diff" amount here.
            this->GetRecycler()->ReportExternalMemoryFree(growSize);

            return finalizeGrowMemory(this->GetLibrary()->CreateWebAssemblyArrayBuffer(this->GetBuffer(), newBufferLength));
        }
#endif

        // No previous buffer case
        if (this->GetByteLength() == 0)
        {
            Assert(newBufferLength == growSize);
            // Creating a new buffer will do the external memory allocation report
            return finalizeGrowMemory(this->GetLibrary()->CreateWebAssemblyArrayBuffer(newBufferLength));
        }

        // Regular growing case
        {
            // Disable Interrupts while doing a ReAlloc to minimize chances to end up in a bad state
            AutoDisableInterrupt autoDisableInterrupt(this->GetScriptContext()->GetThreadContext(), false);

            byte* newBuffer = nullptr;
            const auto reallocFunc = [&]
            {
                newBuffer = ReallocZero(this->GetBuffer(), this->bufferLength, newBufferLength);
                if (newBuffer != nullptr)
                {
                    // Realloc freed this->buffer
                    // if anything goes wrong before we detach, we can't recover the state and should failfast
                    autoDisableInterrupt.RequireExplicitCompletion();
                }
                return !!newBuffer;
            };

            if (!this->GetRecycler()->DoExternalAllocation(growSize, reallocFunc))
            {
                return nullptr;
            }

            // We are transferring the buffer to the new owner.
            // To avoid double-charge to the allocation quota we will free the "diff" amount here.
            this->GetRecycler()->ReportExternalMemoryFree(growSize);

            WebAssemblyArrayBuffer* newArrayBuffer = finalizeGrowMemory(this->GetLibrary()->CreateWebAssemblyArrayBuffer(newBuffer, newBufferLength));
            // We've successfully Detached this buffer and created a new WebAssemblyArrayBuffer
            autoDisableInterrupt.Completed();
            return newArrayBuffer;
        }
    }
#endif

    ProjectionArrayBuffer::ProjectionArrayBuffer(uint32 length, DynamicType * type) :
        ArrayBuffer(length, type, CoTaskMemAlloc)
    {
    }

    ProjectionArrayBuffer::ProjectionArrayBuffer(byte* buffer, uint32 length, DynamicType * type) :
        ArrayBuffer(buffer, length, type)
    {
    }

    ProjectionArrayBuffer::ProjectionArrayBuffer(RefCountedBuffer* buffer, uint32 length, DynamicType * type) :
        ArrayBuffer(buffer, length, type)
    {
    }

    ProjectionArrayBuffer* ProjectionArrayBuffer::Create(uint32 length, DynamicType * type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        recycler->AddExternalMemoryUsage(length);
        return RecyclerNewFinalized(recycler, ProjectionArrayBuffer, length, type);
    }

    ProjectionArrayBuffer* ProjectionArrayBuffer::Create(byte* buffer, uint32 length, DynamicType * type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        ProjectionArrayBuffer* result = RecyclerNewFinalized(recycler, ProjectionArrayBuffer, buffer, length, type);
        // This is user passed [in] buffer, user should AddExternalMemoryUsage before calling jscript, but
        // I don't see we ask everyone to do this. Let's add the memory pressure here as well.
        recycler->AddExternalMemoryUsage(length);
        return result;

    }

    ProjectionArrayBuffer* ProjectionArrayBuffer::Create(RefCountedBuffer* buffer, uint32 length, DynamicType * type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();

        ProjectionArrayBuffer* result = RecyclerNewFinalized(recycler, ProjectionArrayBuffer, buffer, length, type);
        // This is user passed [in] buffer, user should AddExternalMemoryUsage before calling jscript, but
        // I don't see we ask everyone to do this. Let's add the memory pressure here as well.
        recycler->AddExternalMemoryUsage(length);
        return result;
    }

    void ProjectionArrayBuffer::Finalize(bool isShutdown)
    {
        if (this->bufferContent == nullptr || this->bufferContent->GetBuffer() == nullptr)
        {
            return;
        }

        RefCountedBuffer *content = this->bufferContent;
        this->bufferContent = nullptr;
        long refCount = content->Release();
        if (refCount == 0)
        {
            CoTaskMemFree(content->GetBuffer());
            // Recycler may not be available at Dispose. We need to
            // free the memory and report that it has been freed at the same
            // time. Otherwise, AllocationPolicyManager is unable to provide correct feedback
            Recycler* recycler = GetType()->GetLibrary()->GetRecycler();
            recycler->ReportExternalMemoryFree(bufferLength);
            HeapDelete(content);
        }
    }

    void ProjectionArrayBuffer::Dispose(bool isShutdown)
    {
        /* See ProjectionArrayBuffer::Finalize */
    }

    ArrayBufferContentForDelayedFreeBase* ProjectionArrayBuffer::CopyBufferContentForDelayedFree(RefCountedBuffer * content, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength)
    {
        // This heap object will be deleted when the Recycler::DelayedFreeArrayBuffer determines to remove this item
        return HeapNew(ArrayBufferContentForDelayedFree<FreeFn>, content, bufferLength, GetRecycler(), CoTaskMemFree);
    }

    ArrayBuffer* ExternalArrayBufferDetachedState::Create(JavascriptLibrary* library)
    {
        return library->CreateExternalArrayBuffer(buffer, bufferLength);
    }

    ExternalArrayBuffer::ExternalArrayBuffer(byte *buffer, uint32 length, DynamicType *type)
        : ArrayBuffer(buffer, length, type, true)
    {
    }

    ExternalArrayBuffer::ExternalArrayBuffer(RefCountedBuffer *buffer, uint32 length, DynamicType *type)
        : ArrayBuffer(buffer, length, type)
    {
    }

    ExternalArrayBuffer* ExternalArrayBuffer::Create(RefCountedBuffer* buffer, uint32 length, DynamicType * type)
    {
        // This type does not own the external memory, so don't AddExternalMemoryUsage like other ArrayBuffer types do
        return RecyclerNewFinalized(type->GetScriptContext()->GetRecycler(), ExternalArrayBuffer, buffer, length, type);
    }

    ArrayBufferDetachedStateBase* ExternalArrayBuffer::CreateDetachedState(RefCountedBuffer* buffer, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength)
    {
        return HeapNew(ExternalArrayBufferDetachedState, buffer, bufferLength);
    };

    void ExternalArrayBuffer::ReportExternalMemoryFree()
    {
        // This type does not own the external memory, so don't ReportExternalMemoryFree like other ArrayBuffer types do
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType ExternalArrayBuffer::GetSnapTag_TTD() const
    {
        //We re-map ExternalArrayBuffers to regular buffers since the 'real' host will be gone when we replay
        return TTD::NSSnapObjects::SnapObjectType::SnapArrayBufferObject;
    }

    void ExternalArrayBuffer::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapArrayBufferInfo* sabi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapArrayBufferInfo>();

        sabi->Length = this->GetByteLength();
        if(sabi->Length == 0)
        {
            sabi->Buff = nullptr;
        }
        else
        {
            sabi->Buff = alloc.SlabAllocateArray<byte>(sabi->Length);
            memcpy(sabi->Buff, this->GetBuffer(), sabi->Length);
        }

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapArrayBufferInfo*, TTD::NSSnapObjects::SnapObjectType::SnapArrayBufferObject>(objData, sabi);
    }
#endif

    ExternalArrayBufferDetachedState::ExternalArrayBufferDetachedState(RefCountedBuffer* buffer, uint32 bufferLength)
        : ArrayBufferDetachedStateBase(TypeIds_ArrayBuffer, buffer, bufferLength, ArrayBufferAllocationType::External)
    {}

    void ExternalArrayBufferDetachedState::ClearSelfOnly()
    {
        HeapDelete(this);
    }

    void NoOpFree(byte* data) { }

    void ExternalArrayBufferDetachedState::DiscardState()
    {
        // Don't actually free the data as it's externally managed, but do the
        // appropriate cleanup for our RefCountedBuffer.
        DiscardStateBase(NoOpFree);
    }

    void ExternalArrayBufferDetachedState::Discard()
    {
        ClearSelfOnly();
    }
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    bool ArrayBufferBase::Is(Var value)
    {
        return ArrayBuffer::Is(value) || SharedArrayBuffer::Is(value);
    }

    ArrayBufferBase* ArrayBufferBase::FromVar(Var value)
    {
        Assert(ArrayBufferBase::Is(value));
        return static_cast<ArrayBuffer *> (value);
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
        default:
            AssertMsg(false, "Unknown allocationType of ArrayBufferDetachedStateBase ");
        }

        return toReturn;
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
                if (Int8VirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Uint8Array:
                if (Uint8VirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Uint8ClampedArray:
                if (Uint8ClampedVirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Int16Array:
                if (Int16VirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Uint16Array:
                if (Uint16VirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Int32Array:
                if (Int32VirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Uint32Array:
                if (Uint32VirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Float32Array:
                if (Float32VirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Float64Array:
                if (Float64VirtualArray::Is(parent))
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
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
                break;

        case TypeIds_Int64Array:
        case TypeIds_Uint64Array:
        case TypeIds_CharArray:
        case TypeIds_BoolArray:
                TypedArrayBase::FromVar(parent)->ClearLengthAndBufferOnDetach();
            break;

        case TypeIds_DataView:
                DataView::FromVar(parent)->ClearLengthAndBufferOnDetach();
            break;

        default:
            AssertMsg(false, "We need an explicit case for any parent of ArrayBuffer.");
            break;
        }
    }

    void ArrayBuffer::Detach()
    {
        Assert(!this->isDetached);

        this->buffer = nullptr;
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

    ArrayBufferDetachedStateBase* ArrayBuffer::DetachAndGetState()
    {
        // Save the state before detaching
        AutoPtr<ArrayBufferDetachedStateBase> arrayBufferState(this->CreateDetachedState(this->buffer, this->bufferLength));
        Detach();
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

        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);

        if (!(callInfo.Flags & CallFlags_New) || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("ArrayBuffer"));
        }

        uint32 byteLength = 0;
        if (args.Info.Count > 1)
        {
            byteLength = ToIndex(args[1], JSERR_ArrayLengthConstructIncorrect, scriptContext, MaxArrayBufferLength);
        }

        RecyclableObject* newArr = scriptContext->GetLibrary()->CreateArrayBuffer(byteLength);
        Assert(ArrayBuffer::Is(newArr));
        if (byteLength > 0 && !ArrayBuffer::FromVar(newArr)->GetByteLength())
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
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), newArr, nullptr, scriptContext) :
            newArr;
    }

    // ArrayBuffer.prototype.byteLength as described in ES6 draft #20 section 24.1.4.1
    Var ArrayBuffer::EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !ArrayBuffer::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }

        ArrayBuffer* arrayBuffer = ArrayBuffer::FromVar(args[0]);
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
        if (DataView::Is(arg) || TypedArrayBase::Is(arg))
        {
            return library->GetTrue();
        }

        return library->GetFalse();
    }

    // ArrayBuffer.transfer as described in Luke Wagner's proposal: https://gist.github.com/lukewagner/2735af7eea411e18cf20
    Var ArrayBuffer::EntryTransfer(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        Assert(!(callInfo.Flags & CallFlags_New));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(ArrayBuffer_Constructor_transfer);

        if (args.Info.Count < 2 || !ArrayBuffer::Is(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }

        ArrayBuffer* arrayBuffer = ArrayBuffer::FromVar(args[1]);

        if (arrayBuffer->IsDetached())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("ArrayBuffer.transfer"));
        }

        uint32 newBufferLength = arrayBuffer->bufferLength;
        if (args.Info.Count >= 3)
        {
            newBufferLength = ToIndex(args[2], JSERR_ArrayLengthConstructIncorrect, scriptContext, MaxArrayBufferLength);

            // ToIndex above can call user script (valueOf) which can detach the buffer
            if (arrayBuffer->IsDetached())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("ArrayBuffer.transfer"));
            }
        }

        return arrayBuffer->TransferInternal(newBufferLength);
    }

    // ArrayBuffer.prototype.slice as described in ES6 draft #19 section 24.1.4.3.
    Var ArrayBuffer::EntrySlice(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Assert(!(callInfo.Flags & CallFlags_New));

        if (!ArrayBuffer::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        ArrayBuffer* arrayBuffer = ArrayBuffer::FromVar(args[0]);

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
            Var constructorVar = JavascriptOperators::SpeciesConstructor(arrayBuffer, scriptContext->GetLibrary()->GetArrayBufferConstructor(), scriptContext);

            JavascriptFunction* constructor = JavascriptFunction::FromVar(constructorVar);

            Js::Var constructorArgs[] = {constructor, JavascriptNumber::ToVar(byteLength, scriptContext)};
            Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
            Js::Var newVar = JavascriptOperators::NewScObject(constructor, Js::Arguments(constructorCallInfo, constructorArgs), scriptContext);

            if (!ArrayBuffer::Is(newVar)) // 24.1.4.3: 19.If new does not have an [[ArrayBufferData]] internal slot throw a TypeError exception.
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
            }

            newBuffer = ArrayBuffer::FromVar(newVar);

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
            AssertMsg(arrayBuffer->buffer != nullptr, "buffer must not be null when we copy from it");

            js_memcpy_s(newBuffer->buffer, byteLength, arrayBuffer->buffer + start, byteLength);
        }

        return newBuffer;
    }

    Var ArrayBuffer::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);

        Assert(args.Info.Count > 0);

        return args[0];
    }

    ArrayBuffer* ArrayBuffer::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "var must be an ArrayBuffer");

        return static_cast<ArrayBuffer *>(RecyclableObject::FromVar(aValue));
    }

    bool  ArrayBuffer::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_ArrayBuffer;
    }

    template <class Allocator>
    ArrayBuffer::ArrayBuffer(uint32 length, DynamicType * type, Allocator allocator) :
        ArrayBufferBase(type)
    {
        buffer = nullptr;
        bufferLength = 0;
        if (length > MaxArrayBufferLength)
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_FunctionArgument_Invalid);
        }
        else if (length > 0)
        {
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

            if (buffer != nullptr)
            {
                bufferLength = length;
                ZeroMemory(buffer, bufferLength);
            }
            else
            {
                JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
            }
        }
    }

    ArrayBuffer::ArrayBuffer(byte* buffer, uint32 length, DynamicType * type) :
        buffer(buffer), bufferLength(length), ArrayBufferBase(type)
    {
        if (length > MaxArrayBufferLength)
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_FunctionArgument_Invalid);
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

    ArrayBufferDetachedStateBase* JavascriptArrayBuffer::CreateDetachedState(BYTE* buffer, uint32 bufferLength)
    {
#if ENABLE_FAST_ARRAYBUFFER
        if (IsValidVirtualBufferLength(bufferLength))
        {
            return HeapNew(ArrayBufferDetachedState<FreeFn>, buffer, bufferLength, FreeMemAlloc, ArrayBufferAllocationType::MemAlloc);
        }
        else
        {
            return HeapNew(ArrayBufferDetachedState<FreeFn>, buffer, bufferLength, free, ArrayBufferAllocationType::Heap);
        }
#else
        return HeapNew(ArrayBufferDetachedState<FreeFn>, buffer, bufferLength, free, ArrayBufferAllocationType::Heap);
#endif
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
        return !PHASE_OFF1(Js::TypedArrayVirtualPhase) && IsValidAsmJsBufferLengthAlgo(length, true);
#else
        return false;
#endif
    }

    void JavascriptArrayBuffer::Finalize(bool isShutdown)
    {
        // In debugger scenario, ScriptAuthor can create scriptContext and delete scriptContext
        // explicitly. So for the builtin, while javascriptLibrary is still alive fine, the
        // matching scriptContext might have been deleted and the javascriptLibrary->scriptContext
        // field reset (but javascriptLibrary is still alive).
        // Use the recycler field off library instead of scriptcontext to avoid av.

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
        Recycler* recycler = GetType()->GetLibrary()->GetRecycler();
        recycler->ReportExternalMemoryFree(bufferLength);

        buffer = nullptr;
        bufferLength = 0;
    }

    void JavascriptArrayBuffer::Dispose(bool isShutdown)
    {
        /* See JavascriptArrayBuffer::Finalize */
    }

    // Copy memory from src to dst, truncate if dst smaller, zero extra memory
    // if dst larger
    static void MemCpyZero(__bcount(dstSize) BYTE* dst, size_t dstSize,
        __in_bcount(count) const BYTE* src, size_t count)
    {
        js_memcpy_s(dst, dstSize, src, min(dstSize, count));
        if (dstSize > count)
        {
            ZeroMemory(dst + count, dstSize - count);
        }
    }

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

    ArrayBuffer * JavascriptArrayBuffer::TransferInternal(uint32 newBufferLength)
    {
        ArrayBuffer* newArrayBuffer;
        Recycler* recycler = this->GetRecycler();

        if (this->bufferLength > 0)
        {
            ReportDifferentialAllocation(newBufferLength);
        }

        if (newBufferLength == 0 || this->bufferLength == 0)
        {
            newArrayBuffer = GetLibrary()->CreateArrayBuffer(newBufferLength);
            if (newBufferLength > 0 && !newArrayBuffer->GetByteLength())
            {
                JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
            }
        }
        else
        {
            BYTE * newBuffer = nullptr;
            if (IsValidVirtualBufferLength(this->bufferLength))
            {
                if (IsValidVirtualBufferLength(newBufferLength))
                {
                    // we are transferring between an optimized buffer using a length that can be optimized
                    if (newBufferLength < this->bufferLength)
                    {
#pragma prefast(suppress:6250, "Calling 'VirtualFree' without the MEM_RELEASE flag might free memory but not address descriptors (VADs).")
                        VirtualFree(this->buffer + newBufferLength, this->bufferLength - newBufferLength, MEM_DECOMMIT);
                    }
                    else if (newBufferLength > this->bufferLength)
                    {
                        LPVOID newMem = VirtualAlloc(this->buffer + this->bufferLength, newBufferLength - this->bufferLength, MEM_COMMIT, PAGE_READWRITE);
                        if (!newMem)
                        {
                            recycler->ReportExternalMemoryFailure(newBufferLength - this->bufferLength);
                            JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
                        }
                    }
                    newBuffer = this->buffer;
                }
                else
                {
                    // we are transferring from an optimized buffer, but the new length isn't compatible, so start over and copy to new memory
                    newBuffer = (BYTE*)malloc(newBufferLength);
                    if (!newBuffer)
                    {
                        recycler->ReportExternalMemoryFailure(newBufferLength - this->bufferLength);
                        JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
                    }
                    MemCpyZero(newBuffer, newBufferLength, this->buffer, this->bufferLength);
                }
            }
            else
            {
                if (IsValidVirtualBufferLength(newBufferLength))
                {
                    // we are transferring from an unoptimized buffer, but new length can be optimized, so move to that
                    newBuffer = (BYTE*)AsmJsVirtualAllocator(newBufferLength);
                    if (!newBuffer)
                    {
                        recycler->ReportExternalMemoryFailure(newBufferLength - this->bufferLength);
                        JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
                    }
                    MemCpyZero(newBuffer, newBufferLength, this->buffer, this->bufferLength);
                }
                else if (newBufferLength != this->bufferLength)
                {
                    // both sides will just be regular ArrayBuffer, so realloc
                    newBuffer = ReallocZero(this->buffer, this->bufferLength, newBufferLength);
                    if (!newBuffer)
                    {
                        recycler->ReportExternalMemoryFailure(newBufferLength - this->bufferLength);
                        JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
                    }
                }
                else
                {
                    newBuffer = this->buffer;
                }
            }
            newArrayBuffer = GetLibrary()->CreateArrayBuffer(newBuffer, newBufferLength);

        }
        AutoDiscardPTR<Js::ArrayBufferDetachedStateBase> state(DetachAndGetState());
        state->MarkAsClaimed();

        return newArrayBuffer;
    }


    template<typename Func>
    void Js::JavascriptArrayBuffer::ReportDifferentialAllocation(uint32 newBufferLength, Func reportFailureFn)
    {
        Recycler* recycler = this->GetRecycler();

        // Report differential external memory allocation.
        // If current bufferLength == 0, new ArrayBuffer creation records the allocation
        // so no need to do it here.
        if (newBufferLength != this->bufferLength)
        {
            // Expanding buffer
            if (newBufferLength > this->bufferLength)
            {
                if (!recycler->RequestExternalMemoryAllocation(newBufferLength - this->bufferLength))
                {
                    recycler->CollectNow<CollectOnTypedArrayAllocation>();
                    if (!recycler->RequestExternalMemoryAllocation(newBufferLength - this->bufferLength))
                    {
                        reportFailureFn();
                    }
                }
            }
            // Contracting buffer
            else
            {
                recycler->ReportExternalMemoryFree(this->bufferLength - newBufferLength);
            }
        }
    }


    void JavascriptArrayBuffer::ReportDifferentialAllocation(uint32 newBufferLength)
    {
        ScriptContext* scriptContext = GetScriptContext();
        ReportDifferentialAllocation(newBufferLength, [scriptContext] {
            JavascriptError::ThrowOutOfMemoryError(scriptContext);
        });
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


    template<typename Allocator>
    Js::WebAssemblyArrayBuffer::WebAssemblyArrayBuffer(uint32 length, DynamicType * type, Allocator allocator):
        JavascriptArrayBuffer(length, type, allocator)
    {
#ifndef ENABLE_FAST_ARRAYBUFFER
        CompileAssert(UNREACHED);
#endif
        Assert(allocator == WasmVirtualAllocator);
        // Make sure we always have a buffer even if the length is 0
        if (buffer == nullptr && length == 0)
        {
            // We want to allocate an empty buffer using virtual memory
            buffer = (BYTE*)allocator(0);
        }
        if (buffer == nullptr)
        {
            JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
        }
    }

    // Treat as a normal JavascriptArrayBuffer
    WebAssemblyArrayBuffer::WebAssemblyArrayBuffer(uint32 length, DynamicType * type) :
        JavascriptArrayBuffer(length, type, malloc)
    {
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
            // Only add external memory when we create a new internal buffer
            recycler->AddExternalMemoryUsage(length);
        }
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

    ArrayBufferDetachedStateBase* WebAssemblyArrayBuffer::CreateDetachedState(BYTE* buffer, uint32 bufferLength)
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
            // Detach the buffer from this ArrayBuffer
            this->Detach();
            return newArrayBuffer;
        };

        // We're not growing the buffer, just create a new WebAssemblyArrayBuffer and detach this
        if (growSize == 0)
        {
            return finalizeGrowMemory(this->GetLibrary()->CreateWebAssemblyArrayBuffer(this->buffer, this->bufferLength));
        }

#if ENABLE_FAST_ARRAYBUFFER
        // 8Gb Array case
        if (CONFIG_FLAG(WasmFastArray))
        {
            AssertOrFailFast(this->buffer);
            const auto virtualAllocFunc = [&]
            {
                return !!VirtualAlloc(this->buffer + this->bufferLength, growSize, MEM_COMMIT, PAGE_READWRITE);
            };
            if (!this->GetRecycler()->DoExternalAllocation(growSize, virtualAllocFunc))
            {
                return nullptr;
            }
            return finalizeGrowMemory(this->GetLibrary()->CreateWebAssemblyArrayBuffer(this->buffer, newBufferLength));
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
                newBuffer = ReallocZero(this->buffer, this->bufferLength, newBufferLength);
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

            WebAssemblyArrayBuffer* newArrayBuffer = finalizeGrowMemory(this->GetLibrary()->CreateWebAssemblyArrayBuffer(newBuffer, newBufferLength));
            // We've successfully Detached this buffer and created a new WebAssemblyArrayBuffer
            autoDisableInterrupt.Completed();
            return newArrayBuffer;
        }
    }

    ArrayBuffer * WebAssemblyArrayBuffer::TransferInternal(uint32 newBufferLength)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), WASMERR_CantDetach);
    }

    ProjectionArrayBuffer::ProjectionArrayBuffer(uint32 length, DynamicType * type) :
        ArrayBuffer(length, type, CoTaskMemAlloc)
    {
    }

    ProjectionArrayBuffer::ProjectionArrayBuffer(byte* buffer, uint32 length, DynamicType * type) :
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
        // This is user passed [in] buffer, user should AddExternalMemoryUsage before calling jscript, but
        // I don't see we ask everyone to do this. Let's add the memory pressure here as well.
        recycler->AddExternalMemoryUsage(length);
        return RecyclerNewFinalized(recycler, ProjectionArrayBuffer, buffer, length, type);
    }

    void ProjectionArrayBuffer::Dispose(bool isShutdown)
    {
        CoTaskMemFree(buffer);
    }

    ArrayBuffer * ProjectionArrayBuffer::TransferInternal(uint32 newBufferLength)
    {
        ArrayBuffer* newArrayBuffer;
        if (newBufferLength == 0 || this->bufferLength == 0)
        {
            newArrayBuffer = GetLibrary()->CreateProjectionArraybuffer(newBufferLength);
        }
        else
        {
            BYTE * newBuffer = (BYTE*)CoTaskMemRealloc(this->buffer, newBufferLength);
            if (!newBuffer)
            {
                JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
            }
            newArrayBuffer = GetLibrary()->CreateProjectionArraybuffer(newBuffer, newBufferLength);
        }

        AutoDiscardPTR<Js::ArrayBufferDetachedStateBase> state(DetachAndGetState());
        state->MarkAsClaimed();

        return newArrayBuffer;
    }

    ExternalArrayBuffer::ExternalArrayBuffer(byte *buffer, uint32 length, DynamicType *type)
        : ArrayBuffer(buffer, length, type)
    {
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
}

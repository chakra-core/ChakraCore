//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// Implementation for typed arrays based on ArrayBuffer.
// There is one nested ArrayBuffer for each typed array. Multiple typed array
// can share the same array buffer.
//----------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#define INSTANTIATE_BUILT_IN_ENTRYPOINTS(typeName) \
    template Var typeName::NewInstance(RecyclableObject* function, CallInfo callInfo, ...); \
    template Var typeName::EntrySet(RecyclableObject* function, CallInfo callInfo, ...);

namespace Js
{
    // explicitly instantiate these function for the built in entry point FunctionInfo
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Int8Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Uint8Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Uint8ClampedArray)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Int16Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Uint16Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Int32Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Uint32Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Float32Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Float64Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Int64Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(Uint64Array)
    INSTANTIATE_BUILT_IN_ENTRYPOINTS(BoolArray)

    template<> BOOL Uint8ClampedArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint8ClampedArray &&
               ( VirtualTableInfo<Uint8ClampedArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Uint8ClampedArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Uint8Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint8Array &&
              ( VirtualTableInfo<Uint8Array>::HasVirtualTable(aValue) ||
                VirtualTableInfo<CrossSiteObject<Uint8Array>>::HasVirtualTable(aValue)
              );
    }

    template<> BOOL Int8Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Int8Array &&
               ( VirtualTableInfo<Int8Array>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Int8Array>>::HasVirtualTable(aValue)
               );
    }


    template<> BOOL Int16Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Int16Array &&
               ( VirtualTableInfo<Int16Array>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Int16Array>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Uint16Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint16Array &&
              ( VirtualTableInfo<Uint16Array>::HasVirtualTable(aValue) ||
                VirtualTableInfo<CrossSiteObject<Uint16Array>>::HasVirtualTable(aValue)
              );
    }

    template<> BOOL Int32Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Int32Array &&
               ( VirtualTableInfo<Int32Array>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Int32Array>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Uint32Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint32Array &&
               ( VirtualTableInfo<Uint32Array>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Uint32Array>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Float32Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Float32Array &&
               ( VirtualTableInfo<Float32Array>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Float32Array>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Float64Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Float64Array &&
               ( VirtualTableInfo<Float64Array>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Float64Array>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Int64Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Int64Array &&
               ( VirtualTableInfo<Int64Array>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Int64Array>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Uint64Array::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint64Array &&
               ( VirtualTableInfo<Uint64Array>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Uint64Array>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL BoolArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_BoolArray &&
               ( VirtualTableInfo<BoolArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<BoolArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Uint8ClampedVirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint8ClampedArray &&
               ( VirtualTableInfo<Uint8ClampedVirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Uint8ClampedVirtualArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Uint8VirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint8Array &&
               ( VirtualTableInfo<Uint8VirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Uint8VirtualArray>>::HasVirtualTable(aValue)
               );
    }


    template<> BOOL Int8VirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Int8Array &&
               ( VirtualTableInfo<Int8VirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Int8VirtualArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Int16VirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Int16Array &&
               ( VirtualTableInfo<Int16VirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Int16VirtualArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Uint16VirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint16Array &&
               ( VirtualTableInfo<Uint16VirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Uint16VirtualArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Int32VirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Int32Array &&
               ( VirtualTableInfo<Int32VirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Int32VirtualArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Uint32VirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Uint32Array &&
               ( VirtualTableInfo<Uint32VirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Uint32VirtualArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Float32VirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Float32Array &&
               ( VirtualTableInfo<Float32VirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Float32VirtualArray>>::HasVirtualTable(aValue)
               );
    }

    template<> BOOL Float64VirtualArray::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Float64Array &&
               ( VirtualTableInfo<Float64VirtualArray>::HasVirtualTable(aValue) ||
                 VirtualTableInfo<CrossSiteObject<Float64VirtualArray>>::HasVirtualTable(aValue)
               );
    }

    template <typename TypeName, bool clamped, bool virtualAllocated>
    TypedArray<TypeName, clamped, virtualAllocated>* TypedArray<TypeName, clamped, virtualAllocated>::FromVar(Var aValue)
    {
        AssertMsg(TypedArray::Is(aValue), "invalid TypedArray");
        return static_cast<TypedArray<TypeName, clamped, virtualAllocated>*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint8ClampedArray* Uint8ClampedArray::FromVar(Var aValue)
    {
        AssertMsg(Uint8ClampedArray::Is(aValue), "invalid Uint8ClampedArray");
        return static_cast<Uint8ClampedArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint8Array* Uint8Array::FromVar(Var aValue)
    {
        AssertMsg(Uint8Array::Is(aValue), "invalid Uint8Array");
        return static_cast<Uint8Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Int8Array* Int8Array::FromVar(Var aValue)
    {
        AssertMsg(Int8Array::Is(aValue), "invalid Int8Array");
        return static_cast<Int8Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Int16Array* Int16Array::FromVar(Var aValue)
    {
        AssertMsg(Int16Array::Is(aValue), "invalid Int16Array");
        return static_cast<Int16Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint16Array* Uint16Array::FromVar(Var aValue)
    {
        AssertMsg(Uint16Array::Is(aValue), "invalid Uint16Array");
        return static_cast<Uint16Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Int32Array* Int32Array::FromVar(Var aValue)
    {
        AssertMsg(Int32Array::Is(aValue), "invalid Int32Array");
        return static_cast<Int32Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint32Array* Uint32Array::FromVar(Var aValue)
    {
        AssertMsg(Uint32Array::Is(aValue), "invalid Uint32Array");
        return static_cast<Uint32Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Float32Array* Float32Array::FromVar(Var aValue)
    {
        AssertMsg(Float32Array::Is(aValue), "invalid Float32Array");
        return static_cast<Float32Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Float64Array* Float64Array::FromVar(Var aValue)
    {
        AssertMsg(Float64Array::Is(aValue), "invalid Float64Array");
        return static_cast<Float64Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Int64Array* Int64Array::FromVar(Var aValue)
    {
        AssertMsg(Int64Array::Is(aValue), "invalid Int64Array");
        return static_cast<Int64Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint64Array* Uint64Array::FromVar(Var aValue)
    {
        AssertMsg(Uint64Array::Is(aValue), "invalid Uint64Array");
        return static_cast<Uint64Array*>(RecyclableObject::FromVar(aValue));
    }

    template<> Int8VirtualArray* Int8VirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Int8VirtualArray::Is(aValue), "invalid Int8Array");
        return static_cast<Int8VirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint8ClampedVirtualArray* Uint8ClampedVirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Uint8ClampedVirtualArray::Is(aValue), "invalid Uint8ClampedArray");
        return static_cast<Uint8ClampedVirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint8VirtualArray* Uint8VirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Uint8VirtualArray::Is(aValue), "invalid Uint8Array");
        return static_cast<Uint8VirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Int16VirtualArray* Int16VirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Int16VirtualArray::Is(aValue), "invalid Int16Array");
        return static_cast<Int16VirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint16VirtualArray* Uint16VirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Uint16VirtualArray::Is(aValue), "invalid Uint16Array");
        return static_cast<Uint16VirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Int32VirtualArray* Int32VirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Int32VirtualArray::Is(aValue), "invalid Int32Array");
        return static_cast<Int32VirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Uint32VirtualArray* Uint32VirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Uint32VirtualArray::Is(aValue), "invalid Uint32Array");
        return static_cast<Uint32VirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Float32VirtualArray* Float32VirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Float32VirtualArray::Is(aValue), "invalid Float32Array");
        return static_cast<Float32VirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> Float64VirtualArray* Float64VirtualArray::FromVar(Var aValue)
    {
        AssertMsg(Float64VirtualArray::Is(aValue), "invalid Float64Array");
        return static_cast<Float64VirtualArray*>(RecyclableObject::FromVar(aValue));
    }

    template<> BoolArray* BoolArray::FromVar(Var aValue)
    {
        AssertMsg(BoolArray::Is(aValue), "invalid BoolArray");
        return static_cast<BoolArray*>(RecyclableObject::FromVar(aValue));
    }

    TypedArrayBase::TypedArrayBase(ArrayBufferBase* arrayBuffer, uint32 offSet, uint mappedLength, uint elementSize, DynamicType* type) :
        ArrayBufferParent(type, mappedLength, arrayBuffer),
        byteOffset(offSet),
        BYTES_PER_ELEMENT(elementSize)
    {
    }

    inline JsUtil::List<Var, ArenaAllocator>* IteratorToList(RecyclableObject *iterator, ScriptContext *scriptContext, ArenaAllocator *alloc)
    {
        Assert(iterator != nullptr);

        Var nextValue;
        JsUtil::List<Var, ArenaAllocator>* retList = JsUtil::List<Var, ArenaAllocator>::New(alloc);

        while (JavascriptOperators::IteratorStepAndValue(iterator, scriptContext, &nextValue))
        {
            retList->Add(nextValue);
        }

        return retList;
    }

    Var TypedArrayBase::CreateNewInstanceFromIterator(RecyclableObject *iterator, ScriptContext *scriptContext, uint32 elementSize, PFNCreateTypedArray pfnCreateTypedArray)
    {
        TypedArrayBase *newArr = nullptr;

        DECLARE_TEMP_GUEST_ALLOCATOR(tempAlloc);

        ACQUIRE_TEMP_GUEST_ALLOCATOR(tempAlloc, scriptContext, _u("Runtime"));
        {
            JsUtil::List<Var, ArenaAllocator>* tempList = IteratorToList(iterator, scriptContext, tempAlloc);

            uint32 len = tempList->Count();
            uint32 byteLen;

            if (UInt32Math::Mul(len, elementSize, &byteLen))
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_InvalidTypedArrayLength);
            }

            ArrayBufferBase *arrayBuffer = scriptContext->GetLibrary()->CreateArrayBuffer(byteLen);
            newArr = static_cast<TypedArrayBase*>(pfnCreateTypedArray(arrayBuffer, 0, len, scriptContext->GetLibrary()));

            for (uint32 k = 0; k < len; k++)
            {
                Var kValue = tempList->Item(k);
                newArr->SetItem(k, kValue);
            }
        }
        RELEASE_TEMP_GUEST_ALLOCATOR(tempAlloc, scriptContext);

        return newArr;
    }

    Var TypedArrayBase::CreateNewInstance(Arguments& args, ScriptContext* scriptContext, uint32 elementSize, PFNCreateTypedArray pfnCreateTypedArray)
    {
        uint32 byteLength = 0;
        int32 offset = 0;
        int32 mappedLength = -1;
        uint32 elementCount = 0;
        ArrayBufferBase* arrayBuffer = nullptr;
        TypedArrayBase* typedArraySource = nullptr;
        RecyclableObject* jsArraySource = nullptr;
        bool fromExternalObject = false;

        // Handle first argument - see if that is ArrayBuffer/SharedArrayBuffer
        if (args.Info.Count > 1)
        {
            Var firstArgument = args[1];
            if (!Js::JavascriptOperators::IsObject(firstArgument))
            {
                elementCount = ArrayBuffer::ToIndex(firstArgument, JSERR_InvalidTypedArrayLength, scriptContext, ArrayBuffer::MaxArrayBufferLength / elementSize);
            }
            else
            {
                if (TypedArrayBase::Is(firstArgument))
                {
                    // Constructor(TypedArray array)
                    typedArraySource = static_cast<TypedArrayBase*>(firstArgument);
                    if (typedArraySource->IsDetachedBuffer())
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("[TypedArray]"));
                    }

                    elementCount = typedArraySource->GetLength();
                    if (elementCount >= ArrayBuffer::MaxArrayBufferLength / elementSize)
                    {
                        JavascriptError::ThrowRangeError(scriptContext, JSERR_InvalidTypedArrayLength);
                    }
                }
                else if (ArrayBufferBase::Is(firstArgument))
                {
                    // Constructor(ArrayBuffer buffer,
                    //  optional uint32 byteOffset, optional uint32 length)
                    arrayBuffer = ArrayBufferBase::FromVar(firstArgument);
                    if (arrayBuffer->IsDetached())
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("[TypedArray]"));
                    }
                }
                else
                {
                    // Use GetIteratorFunction instead of GetIterator to check if it is the built-in array iterator
                    RecyclableObject* iteratorFn = JavascriptOperators::GetIteratorFunction(firstArgument, scriptContext, true /* optional */);
                    if (iteratorFn != nullptr &&
                        (iteratorFn != scriptContext->GetLibrary()->EnsureArrayPrototypeValuesFunction() ||
                            !JavascriptArray::Is(firstArgument) || JavascriptLibrary::ArrayIteratorPrototypeHasUserDefinedNext(scriptContext)))
                    {
                        Var iterator = CALL_FUNCTION(scriptContext->GetThreadContext(), iteratorFn, CallInfo(Js::CallFlags_Value, 1), RecyclableObject::FromVar(firstArgument));

                        if (!JavascriptOperators::IsObject(iterator))
                        {
                            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
                        }
                        return CreateNewInstanceFromIterator(RecyclableObject::FromVar(iterator), scriptContext, elementSize, pfnCreateTypedArray);
                    }

                    if (!JavascriptConversion::ToObject(firstArgument, scriptContext, &jsArraySource))
                    {
                        // REVIEW: unclear why this JavascriptConversion::ToObject() call is being made.
                        // It ends up calling RecyclableObject::ToObject which at least Proxy objects can
                        // hit with non-trivial behavior.
                        Js::Throw::FatalInternalError();
                    }

                    ArrayBuffer *temp = nullptr;
                    HRESULT hr = scriptContext->GetHostScriptContext()->ArrayBufferFromExternalObject(jsArraySource, &temp);
                    arrayBuffer = static_cast<ArrayBufferBase *> (temp);
                    switch (hr)
                    {
                    case S_OK:
                        // We found an IBuffer
                        fromExternalObject = true;
                        OUTPUT_TRACE(TypedArrayPhase, _u("Projection ArrayBuffer query succeeded with HR=0x%08X\n"), hr);
                        // We have an ArrayBuffer now, so we can skip all the object probing.
                        break;

                    case S_FALSE:
                        // We didn't find an IBuffer - fall through
                        OUTPUT_TRACE(TypedArrayPhase, _u("Projection ArrayBuffer query aborted safely with HR=0x%08X (non-handled type)\n"), hr);
                        break;

                    default:
                        // Any FAILURE HRESULT or unexpected HRESULT
                        OUTPUT_TRACE(TypedArrayPhase, _u("Projection ArrayBuffer query failed with HR=0x%08X\n"), hr);
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidTypedArray_Constructor);
                        break;
                    }

                    if (!fromExternalObject)
                    {
                        Var lengthVar = JavascriptOperators::OP_GetLength(jsArraySource, scriptContext);
                        elementCount = ArrayBuffer::ToIndex(lengthVar, JSERR_InvalidTypedArrayLength, scriptContext, ArrayBuffer::MaxArrayBufferLength / elementSize);
                    }
                }
            }
        }

        // If we got an ArrayBuffer, we can continue to process arguments.
        if (arrayBuffer != nullptr)
        {
            byteLength = arrayBuffer->GetByteLength();
            if (args.Info.Count > 2)
            {
                offset = ArrayBuffer::ToIndex(args[2], JSERR_InvalidTypedArrayLength, scriptContext, byteLength, false);

                // we can start the mapping from the end of the incoming buffer, but with a map of 0 length.
                // User can't really do anything useful with the typed array but apparently this is allowed.
                if ((offset % elementSize) != 0)
                {
                    JavascriptError::ThrowRangeError(
                        scriptContext, JSERR_InvalidTypedArrayLength);
                }
            }

            if (args.Info.Count > 3 && !JavascriptOperators::IsUndefinedObject(args[3]))
            {
                mappedLength = ArrayBuffer::ToIndex(args[3], JSERR_InvalidTypedArrayLength, scriptContext, (byteLength - offset) / elementSize, false);
            }
            else
            {
                if ((byteLength - offset) % elementSize != 0)
                {
                    JavascriptError::ThrowRangeError(
                        scriptContext, JSERR_InvalidTypedArrayLength);
                }
                mappedLength = (byteLength - offset)/elementSize;
            }
        }
        else
        {
            // Null arrayBuffer - could be new constructor or copy constructor.
            byteLength = elementCount * elementSize;
            arrayBuffer = scriptContext->GetLibrary()->CreateArrayBuffer(byteLength);
        }

        if (arrayBuffer->IsDetached())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("[TypedArray]"));
        }

        if (mappedLength == -1)
        {
            mappedLength = (byteLength - offset)/elementSize;
        }

        // Create and set the array based on the source.
        TypedArrayBase* newArray  = static_cast<TypedArrayBase*>(pfnCreateTypedArray(arrayBuffer, offset, mappedLength, scriptContext->GetLibrary()));
        if (fromExternalObject)
        {
            // No need to copy externally provided buffer
            OUTPUT_TRACE(TypedArrayPhase, _u("Created a TypedArray from an external buffer source\n"));
        }
        else if (typedArraySource)
        {
            newArray->Set(typedArraySource, offset);
            OUTPUT_TRACE(TypedArrayPhase, _u("Created a TypedArray from a typed array source\n"));
        }
        else if (jsArraySource)
        {
            newArray->SetObjectNoDetachCheck(jsArraySource, newArray->GetLength(), offset);
            OUTPUT_TRACE(TypedArrayPhase, _u("Created a TypedArray from a JavaScript array source\n"));
        }

        return newArray;
    }

    Var TypedArrayBase::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        function->GetScriptContext()->GetThreadContext()->ProbeStack(Js::Constants::MinStackDefault, function->GetScriptContext());

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);

        JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidTypedArray_Constructor);
    }

    template <typename TypeName, bool clamped, bool virtualAllocated>
    Var TypedArray<TypeName, clamped, virtualAllocated>::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        function->GetScriptContext()->GetThreadContext()->ProbeStack(Js::Constants::MinStackDefault, function->GetScriptContext());

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && RecyclableObject::Is(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);

        if (!(callInfo.Flags & CallFlags_New) || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("[TypedArray]"));
        }

        Var object = TypedArrayBase::CreateNewInstance(args, scriptContext, sizeof(TypeName), TypedArray<TypeName, clamped, virtualAllocated>::Create);

#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            object = Js::JavascriptProxy::AutoProxyWrapper(object);
        }
#endif
        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), RecyclableObject::FromVar(object), nullptr, scriptContext) :
            object;
    };

    BOOL TypedArrayBase::HasOwnProperty(PropertyId propertyId)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(HasPropertyQuery(propertyId));
    }

    inline BOOL TypedArrayBase::CanonicalNumericIndexString(PropertyId propertyId, ScriptContext *scriptContext)
    {
        PropertyString *propertyString = scriptContext->GetPropertyString(propertyId);
        return CanonicalNumericIndexString(propertyString, scriptContext);
    }

    inline BOOL TypedArrayBase::CanonicalNumericIndexString(JavascriptString *propertyString, ScriptContext *scriptContext)
    {
        double result;
        return JavascriptConversion::CanonicalNumericIndexString(propertyString, &result, scriptContext);
    }

    PropertyQueryFlags TypedArrayBase::HasPropertyQuery(PropertyId propertyId)
    {
        uint32 index = 0;
        ScriptContext *scriptContext = GetScriptContext();
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            // All the slots within the length of the array are valid.
            return index < this->GetLength() ? PropertyQueryFlags::Property_Found : PropertyQueryFlags::Property_NotFound_NoProto;
        }

        if (!scriptContext->GetPropertyName(propertyId)->IsSymbol() && CanonicalNumericIndexString(propertyId, scriptContext))
        {
            return PropertyQueryFlags::Property_NotFound_NoProto;
        }

        return DynamicObject::HasPropertyQuery(propertyId);
    }

    BOOL TypedArrayBase::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        uint32 index = 0;
        if (GetScriptContext()->IsNumericPropertyId(propertyId, &index) && (index < this->GetLength()))
        {
            // All the slots within the length of the array are valid.
            return false;
        }
        return DynamicObject::DeleteProperty(propertyId, flags);
    }

    BOOL TypedArrayBase::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags)
    {
        PropertyRecord const *propertyRecord = nullptr;
        if (JavascriptOperators::ShouldTryDeleteProperty(this, propertyNameString, &propertyRecord))
        {
            Assert(propertyRecord);
            return DeleteProperty(propertyRecord->GetPropertyId(), flags);
        }

        return TRUE;
    }

    PropertyQueryFlags TypedArrayBase::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return TypedArrayBase::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    PropertyQueryFlags TypedArrayBase::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        uint32 index = 0;
        if (GetScriptContext()->IsNumericPropertyId(propertyId, &index))
        {
            *value = this->DirectGetItem(index);
            if (JavascriptOperators::GetTypeId(*value) == Js::TypeIds_Undefined)
            {
                return PropertyQueryFlags::Property_NotFound;
            }
            return PropertyQueryFlags::Property_Found;
        }

        if (!requestContext->GetPropertyName(propertyId)->IsSymbol() && CanonicalNumericIndexString(propertyId, requestContext))
        {
            *value = requestContext->GetLibrary()->GetUndefined();
            return PropertyQueryFlags::Property_NotFound_NoProto;
        }

        return DynamicObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    PropertyQueryFlags TypedArrayBase::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord*");

        if (CanonicalNumericIndexString(propertyNameString, requestContext))
        {
            *value = requestContext->GetLibrary()->GetUndefined();
            return PropertyQueryFlags::Property_NotFound_NoProto;
        }

        return DynamicObject::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext);
    }

    PropertyQueryFlags TypedArrayBase::HasItemQuery(uint32 index)
    {
        if (this->IsDetachedBuffer())
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
        }

        if (index < GetLength())
        {
            return PropertyQueryFlags::Property_Found;
        }

        return PropertyQueryFlags::Property_NotFound_NoProto;
    }

    PropertyQueryFlags TypedArrayBase::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        *value = DirectGetItem(index);
        return index < GetLength() ? PropertyQueryFlags::Property_Found : PropertyQueryFlags::Property_NotFound_NoProto;
    }

    PropertyQueryFlags TypedArrayBase::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        *value = DirectGetItem(index);
        return index < GetLength() ? PropertyQueryFlags::Property_Found : PropertyQueryFlags::Property_NotFound_NoProto;
    }

    BOOL TypedArrayBase::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        uint32 index;
        ScriptContext *scriptContext = GetScriptContext();

        if (GetScriptContext()->IsNumericPropertyId(propertyId, &index))
        {
            this->DirectSetItem(index, value);
            return true;
        }

        if (!scriptContext->GetPropertyName(propertyId)->IsSymbol() && CanonicalNumericIndexString(propertyId, scriptContext))
        {
            return FALSE;
        }
        else
        {
            return DynamicObject::SetProperty(propertyId, value, flags, info);
        }
    }

    BOOL TypedArrayBase::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord*");

        ScriptContext *requestContext = GetScriptContext();
        if (CanonicalNumericIndexString(propertyNameString, requestContext))
        {
            return FALSE; // Integer-index exotic object should not set property that is canonical numeric index string but came to this overload
        }

        return DynamicObject::SetProperty(propertyNameString, value, flags, info);
    }

    DescriptorFlags TypedArrayBase::GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (!requestContext->GetPropertyName(propertyId)->IsSymbol() && CanonicalNumericIndexString(propertyId, requestContext))
        {
            return None_NoProto;
        }

        return __super::GetSetter(propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags TypedArrayBase::GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (CanonicalNumericIndexString(propertyNameString, requestContext))
        {
            return None_NoProto;
        }

        return __super::GetSetter(propertyNameString, setterValue, info, requestContext);
    }

    DescriptorFlags TypedArrayBase::GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        if (index >= this->GetLength())
        {
            return None_NoProto;
        }

        return __super::GetItemSetter(index, setterValue, requestContext);
    }

    BOOL TypedArrayBase::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache)
    {
        return enumerator->Initialize(nullptr, this, this, flags, requestContext, forInCache);
    }

    JavascriptEnumerator * TypedArrayBase::GetIndexEnumerator(EnumeratorFlags flags, ScriptContext * requestContext)
    {
        return RecyclerNew(requestContext->GetRecycler(), TypedArrayIndexEnumerator, this, flags, requestContext);
    }

    BOOL TypedArrayBase::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        if (flags == PropertyOperation_None && index >= GetLength())
        {
            return false;
        }

        DirectSetItem(index, value);
        return true;
    }

    BOOL TypedArrayBase::IsEnumerable(PropertyId propertyId)
    {
        uint32 index = 0;
        if (GetScriptContext()->IsNumericPropertyId(propertyId, &index))
        {
            return true;
        }
        return DynamicObject::IsEnumerable(propertyId);
    }

    BOOL TypedArrayBase::IsConfigurable(PropertyId propertyId)
    {
        uint32 index = 0;
        if (GetScriptContext()->IsNumericPropertyId(propertyId, &index))
        {
            return false;
        }
        return DynamicObject::IsConfigurable(propertyId);
    }


    BOOL TypedArrayBase::InitProperty(Js::PropertyId propertyId, Js::Var value, PropertyOperationFlags flags, Js::PropertyValueInfo* info)
    {
        return SetProperty(propertyId, value, flags, info);
    }

    BOOL TypedArrayBase::IsWritable(PropertyId propertyId)
    {
        uint32 index = 0;
        if (GetScriptContext()->IsNumericPropertyId(propertyId, &index))
        {
            return true;
        }
        return DynamicObject::IsWritable(propertyId);
    }


    BOOL TypedArrayBase::SetEnumerable(PropertyId propertyId, BOOL value)
    {
        ScriptContext* scriptContext = GetScriptContext();
        uint32 index;

        // all numeric properties are enumerable
        if (scriptContext->IsNumericPropertyId(propertyId, &index) )
        {
            if (!value)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DefineProperty_NotConfigurable,
                    scriptContext->GetThreadContext()->GetPropertyName(propertyId)->GetBuffer());
            }
            return true;
        }

        return __super::SetEnumerable(propertyId, value);
    }

    BOOL TypedArrayBase::SetWritable(PropertyId propertyId, BOOL value)
    {
        ScriptContext* scriptContext = GetScriptContext();
        uint32 index;

        // default is writable
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            if (!value)
            {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DefineProperty_NotConfigurable,
                scriptContext->GetThreadContext()->GetPropertyName(propertyId)->GetBuffer());
            }
            return true;
        }

        return __super::SetWritable(propertyId, value);
    }

    BOOL TypedArrayBase::SetConfigurable(PropertyId propertyId, BOOL value)
    {
        ScriptContext* scriptContext = GetScriptContext();
        uint32 index;

        // default is not configurable
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            if (value)
            {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DefineProperty_NotConfigurable,
                scriptContext->GetThreadContext()->GetPropertyName(propertyId)->GetBuffer());
            }
            return true;
        }

        return __super::SetConfigurable(propertyId, value);
    }

    BOOL TypedArrayBase::SetAttributes(PropertyId propertyId, PropertyAttributes attributes)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        uint32 index;

        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            VerifySetItemAttributes(propertyId, attributes);
            return true;
        }

        return __super::SetAttributes(propertyId, attributes);
    }

    BOOL TypedArrayBase::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        uint32 index;

        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DefineProperty_NotConfigurable,
                GetScriptContext()->GetThreadContext()->GetPropertyName(propertyId)->GetBuffer());
        }

        return __super::SetAccessors(propertyId, getter, setter, flags);
    }

    BOOL TypedArrayBase::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        ScriptContext* scriptContext = GetScriptContext();

        uint32 index;

        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            VerifySetItemAttributes(propertyId, attributes);
            return SetItem(index, value);
        }

        if (!scriptContext->GetPropertyName(propertyId)->IsSymbol() && CanonicalNumericIndexString(propertyId, scriptContext))
        {
            return FALSE;
        }

        return __super::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    BOOL TypedArrayBase::SetItemWithAttributes(uint32 index, Var value, PropertyAttributes attributes)
    {
        VerifySetItemAttributes(Constants::NoProperty, attributes);
        return SetItem(index, value);
    }

    BOOL TypedArrayBase::Is(Var aValue)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aValue);
        return Is(typeId);
    }

    BOOL TypedArrayBase::Is(TypeId typeId)
    {
        return typeId >= TypeIds_TypedArrayMin && typeId <= TypeIds_TypedArrayMax;
    }

    TypedArrayBase* TypedArrayBase::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "must be a typed array");
        return static_cast<TypedArrayBase*>(aValue);
    }

    BOOL TypedArrayBase::IsDetachedTypedArray(Var aValue)
    {
        return Is(aValue) && FromVar(aValue)->IsDetachedBuffer();
    }

    void TypedArrayBase::Set(TypedArrayBase* source, uint32 offset)
    {
        uint32 sourceLength = source->GetLength();
        uint32 totalLength;

        if (UInt32Math::Add(offset, sourceLength, &totalLength) ||
            (totalLength > GetLength()))
        {
            JavascriptError::ThrowRangeError(
                GetScriptContext(), JSERR_InvalidTypedArrayLength);
        }

        if (source->IsDetachedBuffer())
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
        }

        // memmove buffer if views have same bit representation.
        // types of the same size are compatible, with the following exceptions:
        // - we cannot memmove between float and int arrays, due to different bit pattern
        // - we cannot memmove to a uint8 clamped array from an int8 array, due to negatives rounding to 0
        if (GetTypeId() == source->GetTypeId()
            || (GetBytesPerElement() == source->GetBytesPerElement()
            && !(Uint8ClampedArray::Is(this) && Int8Array::Is(source))
            && !Float32Array::Is(this) && !Float32Array::Is(source)
            && !Float64Array::Is(this) && !Float64Array::Is(source)))
        {
            const size_t offsetInBytes = offset * BYTES_PER_ELEMENT;
            memmove_s(buffer + offsetInBytes,
                      GetByteLength() - offsetInBytes,
                      source->GetByteBuffer(),
                      source->GetByteLength());
        }
        else
        {
            if (source->GetArrayBuffer() != GetArrayBuffer())
            {
                for (uint32 i = 0; i < sourceLength; i++)
                {
                    DirectSetItemNoDetachCheck(offset + i, source->DirectGetItemNoDetachCheck(i));
                }
            }
            else
            {
                // We can have the source and destination coming from the same buffer. element size, start offset, and
                // length for source and dest typed array can be different. Use a separate tmp buffer to copy the elements.
                Js::JavascriptArray* tmpArray = GetScriptContext()->GetLibrary()->CreateArray(sourceLength);
                for (uint32 i = 0; i < sourceLength; i++)
                {
                    tmpArray->SetItem(i, source->DirectGetItem(i), PropertyOperation_None);
                }
                for (uint32 i = 0; i < sourceLength; i++)
                {
                    DirectSetItem(offset + i, tmpArray->DirectGetItem(i));
                }
            }
            if (source->IsDetachedBuffer() || this->IsDetachedBuffer())
            {
                Throw::FatalInternalError();
            }
        }
    }

    uint32 TypedArrayBase::GetSourceLength(RecyclableObject* arraySource, uint32 targetLength, uint32 offset)
    {
        ScriptContext* scriptContext = GetScriptContext();
        uint32 sourceLength = JavascriptConversion::ToUInt32(JavascriptOperators::OP_GetProperty(arraySource, PropertyIds::length, scriptContext), scriptContext);
        uint32 totalLength;
        if (UInt32Math::Add(offset, sourceLength, &totalLength) ||
            (totalLength > targetLength))
        {
            JavascriptError::ThrowRangeError(
                scriptContext, JSERR_InvalidTypedArrayLength);
        }
        return sourceLength;
    }

    void TypedArrayBase::SetObjectNoDetachCheck(RecyclableObject* source, uint32 targetLength, uint32 offset)
    {
        ScriptContext* scriptContext = GetScriptContext();
        uint32 sourceLength = GetSourceLength(source, targetLength, offset);
        Assert(!this->IsDetachedBuffer());

        Var itemValue;
        Var undefinedValue = scriptContext->GetLibrary()->GetUndefined();
        for (uint32 i = 0; i < sourceLength; i++)
        {
            if (!source->GetItem(source, i, &itemValue, scriptContext))
            {
                itemValue = undefinedValue;
            }
            DirectSetItemNoDetachCheck(offset + i, itemValue);
        }

        if (this->IsDetachedBuffer())
        {
            // We cannot be detached when we are creating the typed array itself. Terminate if that happens.
            Throw::FatalInternalError();
        }
    }

    // Getting length from the source object can detach the typedarray, and thus set it's length as 0,
    // this is observable because RangeError will be thrown instead of a TypeError further down the line
    void TypedArrayBase::SetObject(RecyclableObject* source, uint32 targetLength, uint32 offset)
    {
        ScriptContext* scriptContext = GetScriptContext();
        uint32 sourceLength = GetSourceLength(source, targetLength, offset);

        Var itemValue;
        Var undefinedValue = scriptContext->GetLibrary()->GetUndefined();
        for (uint32 i = 0; i < sourceLength; i ++)
        {
            if (!source->GetItem(source, i, &itemValue, scriptContext))
            {
                itemValue = undefinedValue;
            }
            DirectSetItem(offset + i, itemValue);
        }
    }

    HRESULT TypedArrayBase::GetBuffer(Var instance, ArrayBuffer** outBuffer, uint32* outOffset, uint32* outLength)
    {
        HRESULT hr = NOERROR;
        if (Js::TypedArrayBase::Is(instance))
        {
            Js::TypedArrayBase* typedArrayBase = Js::TypedArrayBase::FromVar(instance);
            *outBuffer = typedArrayBase->GetArrayBuffer()->GetAsArrayBuffer();
            *outOffset = typedArrayBase->GetByteOffset();
            *outLength = typedArrayBase->GetByteLength();
        }
        else if (Js::ArrayBuffer::Is(instance))
        {
            Js::ArrayBuffer* buffer = Js::ArrayBuffer::FromVar(instance);
            *outBuffer = buffer;
            *outOffset = 0;
            *outLength = buffer->GetByteLength();
        }
        else if (Js::DataView::Is(instance))
        {
            Js::DataView* dView = Js::DataView::FromVar(instance);
            *outBuffer = dView->GetArrayBuffer()->GetAsArrayBuffer();
            *outOffset = dView->GetByteOffset();
            *outLength = dView->GetLength();
        }
        else
        {
            hr = E_INVALIDARG;
        }
        return hr;

    }

    void TypedArrayBase::ClearLengthAndBufferOnDetach()
    {
        AssertMsg(IsDetachedBuffer(), "Array buffer should be detached if we're calling this method");

        this->length = 0;
        this->buffer = nullptr;
    }

    Var TypedArrayBase::EntryGetterBuffer(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !TypedArrayBase::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        TypedArrayBase* typedArray = TypedArrayBase::FromVar(args[0]);
        ArrayBufferBase* arrayBuffer = typedArray->GetArrayBuffer();

        if (arrayBuffer == nullptr)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }

        return arrayBuffer;
    }

    Var TypedArrayBase::EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !TypedArrayBase::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        TypedArrayBase* typedArray = TypedArrayBase::FromVar(args[0]);
        ArrayBufferBase* arrayBuffer = typedArray->GetArrayBuffer();

        if (arrayBuffer == nullptr)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }
        else if (arrayBuffer->IsDetached())
        {
            return TaggedInt::ToVarUnchecked(0);
        }

        return JavascriptNumber::ToVar(typedArray->GetByteLength(), scriptContext);
    }

    Var TypedArrayBase::EntryGetterByteOffset(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !TypedArrayBase::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        TypedArrayBase* typedArray = TypedArrayBase::FromVar(args[0]);
        ArrayBufferBase* arrayBuffer = typedArray->GetArrayBuffer();

        if (arrayBuffer == nullptr)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }
        else if (arrayBuffer->IsDetached())
        {
            return TaggedInt::ToVarUnchecked(0);
        }

        return JavascriptNumber::ToVar(typedArray->GetByteOffset(), scriptContext);
    }

    Var TypedArrayBase::EntryGetterLength(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !TypedArrayBase::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        TypedArrayBase* typedArray = TypedArrayBase::FromVar(args[0]);
        ArrayBufferBase* arrayBuffer = typedArray->GetArrayBuffer();

        if (arrayBuffer == nullptr)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }
        else if (arrayBuffer->IsDetached())
        {
            return TaggedInt::ToVarUnchecked(0);
        }

        return JavascriptNumber::ToVar(typedArray->GetLength(), scriptContext);
    }

    Var TypedArrayBase::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);

        Assert(args.Info.Count > 0);

        return args[0];
    }

    // ES2017 22.2.3.32 get %TypedArray%.prototype[@@toStringTag]
    Var TypedArrayBase::EntryGetterSymbolToStringTag(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // 1. Let O be the this value.
        // 2. If Type(O) is not Object, return undefined.
        // 3. If O does not have a[[TypedArrayName]] internal slot, return undefined.
        if (args.Info.Count == 0 || !TypedArrayBase::Is(args[0]))
        {
            if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
            }
        }

        // 4. Let name be O.[[TypedArrayName]].
        // 5. Assert: name is a String value.
        // 6. Return name.
        Var name;
        switch (JavascriptOperators::GetTypeId(args[0]))
        {
        case TypeIds_Int8Array:
            name = scriptContext->GetPropertyString(PropertyIds::Int8Array);
            break;

        case TypeIds_Uint8Array:
            name = scriptContext->GetPropertyString(PropertyIds::Uint8Array);
            break;

        case TypeIds_Uint8ClampedArray:
            name = scriptContext->GetPropertyString(PropertyIds::Uint8ClampedArray);
            break;

        case TypeIds_Int16Array:
            name = scriptContext->GetPropertyString(PropertyIds::Int16Array);
            break;

        case TypeIds_Uint16Array:
            name = scriptContext->GetPropertyString(PropertyIds::Uint16Array);
            break;

        case TypeIds_Int32Array:
            name = scriptContext->GetPropertyString(PropertyIds::Int32Array);
            break;

        case TypeIds_Uint32Array:
            name = scriptContext->GetPropertyString(PropertyIds::Uint32Array);
            break;

        case TypeIds_Float32Array:
            name = scriptContext->GetPropertyString(PropertyIds::Float32Array);
            break;

        case TypeIds_Float64Array:
            name = scriptContext->GetPropertyString(PropertyIds::Float64Array);
            break;

        default:
            name = scriptContext->GetLibrary()->GetUndefinedDisplayString();
            break;
        }

        return name;
    }

    Var TypedArrayBase::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !TypedArrayBase::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        return CommonSet(args);
    }

    template <typename TypeName, bool clamped, bool virtualAllocated>
    Var TypedArray<TypeName, clamped, virtualAllocated>::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // This method is only called in pre-ES6 compat modes. In those modes, we need to throw an error
        // if the this argument is not the same type as our TypedArray template instance.
        if (args.Info.Count == 0 || !TypedArray::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        return CommonSet(args);
    }

    Var TypedArrayBase::CommonSet(Arguments& args)
    {
        TypedArrayBase* typedArrayBase = TypedArrayBase::FromVar(args[0]);
        ScriptContext* scriptContext = typedArrayBase->GetScriptContext();
        uint32 offset = 0;
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_TypedArray_NeedSource);
        }

        if (args.Info.Count > 2)
        {
            offset = ArrayBuffer::ToIndex(args[2], JSERR_InvalidTypedArrayLength, scriptContext, ArrayBuffer::MaxArrayBufferLength, false);
        }

        if (TypedArrayBase::Is(args[1]))
        {
            TypedArrayBase* typedArraySource = TypedArrayBase::FromVar(args[1]);
            if (typedArraySource->IsDetachedBuffer() || typedArrayBase->IsDetachedBuffer()) // If IsDetachedBuffer(targetBuffer) is true, then throw a TypeError exception.
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("[TypedArray].prototype.set"));
            }
            typedArrayBase->Set(typedArraySource, (uint32)offset);
        }
        else
        {
            RecyclableObject* sourceArray;
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(args[1]);
#endif
            if (!JavascriptConversion::ToObject(args[1], scriptContext, &sourceArray))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_TypedArray_NeedSource);
            }
            else if (typedArrayBase->IsDetachedBuffer())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("[TypedArray].prototype.set"));
            }
            typedArrayBase->SetObject(sourceArray, typedArrayBase->GetLength(), (uint32)offset);
        }
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var TypedArrayBase::EntrySubarray(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_subarray);

        if (args.Info.Count == 0 || !TypedArrayBase::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        return CommonSubarray(args);
    }

    uint32 TypedArrayBase::GetFromIndex(Var arg, uint32 length, ScriptContext *scriptContext)
    {
        uint32 index = JavascriptArray::GetFromIndex(arg, length, scriptContext);
        Assert(index <= length);
        return index;
    }

    Var TypedArrayBase::CommonSubarray(Arguments& args)
    {
        TypedArrayBase* typedArrayBase = TypedArrayBase::FromVar(args[0]);
        uint32 length = typedArrayBase->GetLength();
        ScriptContext* scriptContext = typedArrayBase->GetScriptContext();
        int32 begin = 0;
        int end = length;
        if (args.Info.Count > 1)
        {
            begin = TypedArrayBase::GetFromIndex(args[1], length, scriptContext);

            if (args.Info.Count > 2 && !JavascriptOperators::IsUndefined(args[2]))
            {
                end = TypedArrayBase::GetFromIndex(args[2], length, scriptContext);
            }
        }

        if (end < begin)
        {
            end = begin;
        }

        return typedArrayBase->Subarray(begin, end);
    }

    template <typename TypeName, bool clamped, bool virtualAllocated>
    Var TypedArray<TypeName, clamped, virtualAllocated>::Subarray(uint32 begin, uint32 end)
    {
        Assert(end >= begin);

        Var newTypedArray;
        ScriptContext* scriptContext = this->GetScriptContext();
        ArrayBufferBase* buffer = this->GetArrayBuffer();
        uint32 srcByteOffset = this->GetByteOffset();
        uint32 beginByteOffset = srcByteOffset + begin * BYTES_PER_ELEMENT;
        uint32 newLength = end - begin;

        if (scriptContext->GetConfig()->IsES6SpeciesEnabled())
        {
            JavascriptFunction* constructor =
                JavascriptFunction::FromVar(JavascriptOperators::SpeciesConstructor(this, TypedArrayBase::GetDefaultConstructor(this, scriptContext), scriptContext));

            Js::Var constructorArgs[] = { constructor, buffer, JavascriptNumber::ToVar(beginByteOffset, scriptContext), JavascriptNumber::ToVar(newLength, scriptContext) };
            Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
            newTypedArray = RecyclableObject::FromVar(TypedArrayBase::TypedArrayCreate(constructor, &Js::Arguments(constructorCallInfo, constructorArgs), newLength, scriptContext));
        }
        else
        {
            newTypedArray = TypedArray<TypeName, clamped, virtualAllocated>::Create(buffer, beginByteOffset, newLength, scriptContext->GetLibrary());
        }

        return newTypedArray;
    }

    // %TypedArray%.from as described in ES6.0 (draft 22) Section 22.2.2.1
    Var TypedArrayBase::EntryFrom(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].from"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_from);

        if (args.Info.Count < 1 || !JavascriptOperators::IsConstructor(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedFunction, _u("[TypedArray].from"));
        }

        RecyclableObject* constructor = RecyclableObject::FromVar(args[0]);
        RecyclableObject* items = nullptr;

        if (args.Info.Count < 2 || !JavascriptConversion::ToObject(args[1], scriptContext, &items))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("[TypedArray].from"));
        }

        bool mapping = false;
        JavascriptFunction* mapFn = nullptr;
        Var mapFnThisArg = nullptr;

        if (args.Info.Count >= 3)
        {
            if (!JavascriptFunction::Is(args[2]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].from"));
            }

            mapFn = JavascriptFunction::FromVar(args[2]);

            if (args.Info.Count >= 4)
            {
                mapFnThisArg = args[3];
            }
            else
            {
                mapFnThisArg = library->GetUndefined();
            }

            mapping = true;
        }

        Var newObj;
        RecyclableObject* iterator = JavascriptOperators::GetIterator(items, scriptContext, true /* optional */);

        if (iterator != nullptr)
        {
            DECLARE_TEMP_GUEST_ALLOCATOR(tempAlloc);

            ACQUIRE_TEMP_GUEST_ALLOCATOR(tempAlloc, scriptContext, _u("Runtime"));
            {
                // Create a temporary list to hold the items returned from the iterator.
                // We will then iterate over this list and append those items into the object we will return.
                // We have to collect the items into this temporary list because we need to call the
                // new object constructor with a length of items and we don't know what length will be
                // until we iterate across all the items.
                // Consider: Could be possible to fast-path this in order to avoid creating the temporary list
                //       for types we know such as TypedArray. We know the length of a TypedArray but we still
                //       have to be careful in case there is a proxy which could return anything from [[Get]]
                //       or the built-in @@iterator has been replaced.
                JsUtil::List<Var, ArenaAllocator>* tempList = IteratorToList(iterator, scriptContext, tempAlloc);

                uint32 len = tempList->Count();

                Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(len, scriptContext) };
                Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
                newObj = TypedArrayBase::TypedArrayCreate(constructor, &Js::Arguments(constructorCallInfo, constructorArgs), len, scriptContext);

                TypedArrayBase* newTypedArrayBase = nullptr;
                JavascriptArray* newArr = nullptr;

                if (TypedArrayBase::Is(newObj))
                {
                    newTypedArrayBase = TypedArrayBase::FromVar(newObj);
                }
                else if (JavascriptArray::Is(newObj))
                {
                    newArr = JavascriptArray::FromVar(newObj);
                }

                for (uint32 k = 0; k < len; k++)
                {
                    Var kValue = tempList->Item(k);

                    if (mapping)
                    {
                        Assert(mapFn != nullptr);
                        Assert(mapFnThisArg != nullptr);

                        Js::Var mapFnArgs[] = { mapFnThisArg, kValue, JavascriptNumber::ToVar(k, scriptContext) };
                        Js::CallInfo mapFnCallInfo(Js::CallFlags_Value, _countof(mapFnArgs));
                        kValue = mapFn->CallFunction(Js::Arguments(mapFnCallInfo, mapFnArgs));
                    }

                    // We're likely to have constructed a new TypedArray, but the constructor could return any object
                    if (newTypedArrayBase)
                    {
                        newTypedArrayBase->DirectSetItem(k, kValue);
                    }
                    else if (newArr)
                    {
                        newArr->SetItem(k, kValue, Js::PropertyOperation_ThrowIfNotExtensible);
                    }
                    else
                    {
                        JavascriptOperators::OP_SetElementI_UInt32(newObj, k, kValue, scriptContext, Js::PropertyOperation_ThrowIfNotExtensible);
                    }
                }
            }
            RELEASE_TEMP_GUEST_ALLOCATOR(tempAlloc, scriptContext);
        }
        else
        {
            Var lenValue = JavascriptOperators::OP_GetLength(items, scriptContext);
            uint32 len = JavascriptConversion::ToUInt32(lenValue, scriptContext);

            TypedArrayBase* itemsTypedArrayBase = nullptr;
            JavascriptArray* itemsArray = nullptr;

            if (TypedArrayBase::Is(items))
            {
                itemsTypedArrayBase = TypedArrayBase::FromVar(items);
            }
            else if (JavascriptArray::Is(items))
            {
                itemsArray = JavascriptArray::FromVar(items);
            }

            Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(len, scriptContext) };
            Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
            newObj = TypedArrayBase::TypedArrayCreate(constructor, &Js::Arguments(constructorCallInfo, constructorArgs), len, scriptContext);

            TypedArrayBase* newTypedArrayBase = nullptr;
            JavascriptArray* newArr = nullptr;

            if (TypedArrayBase::Is(newObj))
            {
                newTypedArrayBase = TypedArrayBase::FromVar(newObj);
            }
            else if (JavascriptArray::Is(newObj))
            {
                newArr = JavascriptArray::FromVar(newObj);
            }

            for (uint32 k = 0; k < len; k++)
            {
                Var kValue;

                // The items source could be anything, but we already know if it's a TypedArray or Array
                if (itemsTypedArrayBase)
                {
                    kValue = itemsTypedArrayBase->DirectGetItem(k);
                }
                else if (itemsArray)
                {
                    kValue = itemsArray->DirectGetItem(k);
                }
                else
                {
                    kValue = JavascriptOperators::OP_GetElementI_UInt32(items, k, scriptContext);
                }

                if (mapping)
                {
                    Assert(mapFn != nullptr);
                    Assert(mapFnThisArg != nullptr);

                    Js::Var mapFnArgs[] = { mapFnThisArg, kValue, JavascriptNumber::ToVar(k, scriptContext) };
                    Js::CallInfo mapFnCallInfo(Js::CallFlags_Value, _countof(mapFnArgs));
                    kValue = mapFn->CallFunction(Js::Arguments(mapFnCallInfo, mapFnArgs));
                }

                // If constructor built a TypedArray (likely) or Array (maybe likely) we can do a more direct set operation
                if (newTypedArrayBase)
                {
                    newTypedArrayBase->DirectSetItem(k, kValue);
                }
                else if (newArr)
                {
                    newArr->SetItem(k, kValue, Js::PropertyOperation_ThrowIfNotExtensible);
                }
                else
                {
                    JavascriptOperators::OP_SetElementI_UInt32(newObj, k, kValue, scriptContext, Js::PropertyOperation_ThrowIfNotExtensible);
                }
            }
        }

        return newObj;
    }

    Var TypedArrayBase::EntryOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_of);

        if (args.Info.Count < 1)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedFunction, _u("[TypedArray].of"));
        }

        return JavascriptArray::OfHelper(true, args, scriptContext);
    }

    Var TypedArrayBase::EntryCopyWithin(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_copyWithin);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.copyWithin"));
        uint32 length = typedArrayBase->GetLength();

        return JavascriptArray::CopyWithinHelper(nullptr, typedArrayBase, typedArrayBase, length, args, scriptContext);
    }

    Var TypedArrayBase::GetKeysEntriesValuesHelper(Arguments& args, ScriptContext *scriptContext, LPCWSTR apiName, JavascriptArrayIteratorKind kind)
    {
        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, apiName);
        return scriptContext->GetLibrary()->CreateArrayIterator(typedArrayBase, kind);
    }

    Var TypedArrayBase::EntryEntries(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_entries);

        return GetKeysEntriesValuesHelper(args, scriptContext, _u("[TypedArray].prototype.entries"), JavascriptArrayIteratorKind::KeyAndValue);
    }

    Var TypedArrayBase::EntryEvery(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.every"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_every);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.every"));
        return JavascriptArray::EveryHelper(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    Var TypedArrayBase::EntryFill(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_fill);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.fill"));
        return JavascriptArray::FillHelper(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    // %TypedArray%.prototype.filter as described in ES6.0 (draft 22) Section 22.2.3.9
    Var TypedArrayBase::EntryFilter(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.filter"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_filter);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.filter"));
        uint32 length = typedArrayBase->GetLength();

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].prototype.filter"));
        }

        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var thisArg = nullptr;

        if (args.Info.Count > 2)
        {
            thisArg = args[2];
        }
        else
        {
            thisArg = scriptContext->GetLibrary()->GetUndefined();
        }

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var element = nullptr;
        Var selected = nullptr;
        RecyclableObject* newObj = nullptr;

        DECLARE_TEMP_GUEST_ALLOCATOR(tempAlloc);

        ACQUIRE_TEMP_GUEST_ALLOCATOR(tempAlloc, scriptContext, _u("Runtime"));
        {
            // Create a temporary list to hold the items selected by the callback function.
            // We will then iterate over this list and append those items into the object we will return.
            // We have to collect the items into this temporary list because we need to call the
            // new object constructor with a length of items and we don't know what length will be
            // until we know how many items we will collect.
            JsUtil::List<Var, ArenaAllocator>* tempList = JsUtil::List<Var, ArenaAllocator>::New(tempAlloc);

            for (uint32 k = 0; k < length; k++)
            {
                // We know that the TypedArray.HasItem will be true because k < length and we can skip the check in the TypedArray version of filter.
                element = typedArrayBase->DirectGetItem(k);

                selected = CALL_FUNCTION(scriptContext->GetThreadContext(),
                    callBackFn, CallInfo(flags, 4), thisArg,
                    element,
                    JavascriptNumber::ToVar(k, scriptContext),
                    typedArrayBase);

                if (JavascriptConversion::ToBoolean(selected, scriptContext))
                {
                    tempList->Add(element);
                }
            }

            uint32 captured = tempList->Count();

            Var constructor = JavascriptOperators::SpeciesConstructor(
                typedArrayBase, TypedArrayBase::GetDefaultConstructor(args[0], scriptContext), scriptContext);

            Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(captured, scriptContext) };
            Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
            newObj = RecyclableObject::FromVar(TypedArrayBase::TypedArrayCreate(constructor, &Js::Arguments(constructorCallInfo, constructorArgs), captured, scriptContext));

            if (TypedArrayBase::Is(newObj))
            {
                // We are much more likely to have a TypedArray here than anything else
                TypedArrayBase* newArr = TypedArrayBase::FromVar(newObj);

                for (uint32 i = 0; i < captured; i++)
                {
                    newArr->DirectSetItem(i, tempList->Item(i));
                }
            }
            else
            {
                for (uint32 i = 0; i < captured; i++)
                {
                    JavascriptOperators::OP_SetElementI_UInt32(newObj, i, tempList->Item(i), scriptContext, PropertyOperation_ThrowIfNotExtensible);
                }
            }
        }
        RELEASE_TEMP_GUEST_ALLOCATOR(tempAlloc, scriptContext);

        return newObj;
    }

    Var TypedArrayBase::EntryFind(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.find"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_find);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.find"));

        return JavascriptArray::FindHelper<false>(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    Var TypedArrayBase::EntryFindIndex(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.findIndex"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_findIndex);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.findIndex"));

        return JavascriptArray::FindHelper<true>(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    // %TypedArray%.prototype.forEach as described in ES6.0 (draft 22) Section 22.2.3.12
    Var TypedArrayBase::EntryForEach(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.forEach"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_forEach);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.forEach"));
        uint32 length = typedArrayBase->GetLength();

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].prototype.forEach"));
        }

        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var thisArg;

        if (args.Info.Count > 2)
        {
            thisArg = args[2];
        }
        else
        {
            thisArg = scriptContext->GetLibrary()->GetUndefined();
        }

        for (uint32 k = 0; k < length; k++)
        {
            // No need for HasItem, as we have already established that this API can be called only on the TypedArray object. So Proxy scenario cannot happen.

            Var element = typedArrayBase->DirectGetItem(k);

            CALL_FUNCTION(scriptContext->GetThreadContext(),
                callBackFn, CallInfo(CallFlags_Value, 4),
                thisArg,
                element,
                JavascriptNumber::ToVar(k, scriptContext),
                typedArrayBase);
        }

        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var TypedArrayBase::EntryIndexOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_indexOf);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.indexOf"));
        uint32 length = typedArrayBase->GetLength();

        Var search = nullptr;
        uint32 fromIndex;
        if (!JavascriptArray::GetParamForIndexOf(length, args, search, fromIndex, scriptContext))
        {
            return TaggedInt::ToVarUnchecked(-1);
        }

        return JavascriptArray::TemplatedIndexOfHelper<false>(typedArrayBase, search, fromIndex, length, scriptContext);
    }

    Var TypedArrayBase::EntryIncludes(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_includes);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.includes"));
        uint32 length = typedArrayBase->GetLength();

        Var search = nullptr;
        uint32 fromIndex;
        if (!JavascriptArray::GetParamForIndexOf(length, args, search, fromIndex, scriptContext))
        {
            return scriptContext->GetLibrary()->GetFalse();
        }

        return JavascriptArray::TemplatedIndexOfHelper<true>(typedArrayBase, search, fromIndex, length, scriptContext);
    }


    Var TypedArrayBase::EntryJoin(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_join);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.join"));
        uint32 length = typedArrayBase->GetLength();

        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptString* separator = nullptr;

        if (args.Info.Count > 1 && !JavascriptOperators::IsUndefined(args[1]))
        {
            separator = JavascriptConversion::ToString(args[1], scriptContext);
        }
        else
        {
            separator = library->GetCommaDisplayString();
        }

        if (length == 0)
        {
            return library->GetEmptyString();
        }
        else if (length == 1)
        {
            return JavascriptConversion::ToString(typedArrayBase->DirectGetItem(0), scriptContext);
        }

        bool hasSeparator = (separator->GetLength() != 0);

        charcount_t estimatedAppendSize = min(
            static_cast<charcount_t>((64 << 20) / sizeof(void *)), // 64 MB worth of pointers
            static_cast<charcount_t>(length + (hasSeparator ? length - 1 : 0)));

        CompoundString* const cs = CompoundString::NewWithPointerCapacity(estimatedAppendSize, library);

        Assert(length >= 2);

        JavascriptString* element = JavascriptConversion::ToString(typedArrayBase->DirectGetItem(0), scriptContext);

        cs->Append(element);

        for (uint32 i = 1; i < length; i++)
        {
            if (hasSeparator)
            {
                cs->Append(separator);
            }

            // Since i < length, we can be certain that the TypedArray contains an item at index i and we don't have to check for undefined
            element = JavascriptConversion::ToString(typedArrayBase->DirectGetItem(i), scriptContext);

            cs->Append(element);
        }

        return cs;
    }

    Var TypedArrayBase::EntryKeys(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_keys);

        return GetKeysEntriesValuesHelper(args, scriptContext, _u("[TypedArray].prototype.keys"), JavascriptArrayIteratorKind::Key);
    }

    Var TypedArrayBase::EntryLastIndexOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_lastIndexOf);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.lastIndexOf"));
        uint32 length = typedArrayBase->GetLength();

        Var search = nullptr;
        int64 fromIndex;
        if (!JavascriptArray::GetParamForLastIndexOf(length, args, search, fromIndex, scriptContext))
        {
            return TaggedInt::ToVarUnchecked(-1);
        }

        return JavascriptArray::LastIndexOfHelper(typedArrayBase, search, fromIndex, scriptContext);
    }

    Var TypedArrayBase::EntryMap(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.map"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_map);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.map"));

        return JavascriptArray::MapHelper(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    Var TypedArrayBase::EntryReduce(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.reduce"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_reduce);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.reduce"));
        return JavascriptArray::ReduceHelper(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    Var TypedArrayBase::EntryReduceRight(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.reduceRight"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_reduceRight);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.reduceRight"));
        return JavascriptArray::ReduceRightHelper(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    Var TypedArrayBase::EntryReverse(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_reverse);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.reverse"));
        return JavascriptArray::ReverseHelper(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), scriptContext);
    }

    Var TypedArrayBase::EntrySlice(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.slice"));
        return JavascriptArray::SliceHelper(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    Var TypedArrayBase::EntrySome(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.some"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_some);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.some"));

        return JavascriptArray::SomeHelper(nullptr, typedArrayBase, typedArrayBase, typedArrayBase->GetLength(), args, scriptContext);
    }

    template<typename T> int __cdecl TypedArrayCompareElementsHelper(void* context, const void* elem1, const void* elem2)
    {
        const T* element1 = static_cast<const T*>(elem1);
        const T* element2 = static_cast<const T*>(elem2);

        Assert(element1 != nullptr);
        Assert(element2 != nullptr);
        Assert(context != nullptr);

        const T x = *element1;
        const T y = *element2;

        if (NumberUtilities::IsNan((double)x))
        {
            if (NumberUtilities::IsNan((double)y))
            {
                return 0;
            }

            return 1;
        }
        else
        {
            if (NumberUtilities::IsNan((double)y))
            {
                return -1;
            }
        }

        void **contextArray = (void **)context;
        if (contextArray[1] != nullptr)
        {
            RecyclableObject* compFn = RecyclableObject::FromVar(contextArray[1]);
            ScriptContext* scriptContext = compFn->GetScriptContext();
            Var undefined = scriptContext->GetLibrary()->GetUndefined();
            double dblResult;
            Var retVal = CALL_FUNCTION(scriptContext->GetThreadContext(),
                compFn, CallInfo(CallFlags_Value, 3),
                undefined,
                JavascriptNumber::ToVarWithCheck((double)x, scriptContext),
                JavascriptNumber::ToVarWithCheck((double)y, scriptContext));

            Assert(TypedArrayBase::Is(contextArray[0]));
            if (TypedArrayBase::IsDetachedTypedArray(contextArray[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("[TypedArray].prototype.sort"));
            }

            if (TaggedInt::Is(retVal))
            {
                return TaggedInt::ToInt32(retVal);
            }

            if (JavascriptNumber::Is_NoTaggedIntCheck(retVal))
            {
                dblResult = JavascriptNumber::GetValue(retVal);
            }
            else
            {
                dblResult = JavascriptConversion::ToNumber_Full(retVal, scriptContext);

                // ToNumber may execute user-code which can cause the array to become detached
                if (TypedArrayBase::IsDetachedTypedArray(contextArray[0]))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("[TypedArray].prototype.sort"));
                }
            }

            if (dblResult < 0)
            {
                return -1;
            }
            else if (dblResult > 0)
            {
                return 1;
            }

            return 0;
        }
        else
        {
            if (x < y)
            {
                return -1;
            }
            else if (x > y)
            {
                return 1;
            }

            return 0;
        }
    }

    Var TypedArrayBase::EntrySort(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("[TypedArray].prototype.sort"));

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_sort);

        TypedArrayBase* typedArrayBase = ValidateTypedArray(args, scriptContext, _u("[TypedArray].prototype.sort"));
        uint32 length = typedArrayBase->GetLength();

        // If TypedArray has no length, we don't have any work to do.
        if (length == 0)
        {
            return typedArrayBase;
        }

        RecyclableObject* compareFn = nullptr;

        if (args.Info.Count > 1)
        {
            if (!JavascriptConversion::IsCallable(args[1]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].prototype.sort"));
            }

            compareFn = RecyclableObject::FromVar(args[1]);
        }

        // Get the elements comparison function for the type of this TypedArray
        void* elementCompare = reinterpret_cast<void*>(typedArrayBase->GetCompareElementsFunction());

        Assert(elementCompare);

        // Cast compare to the correct function type
        int(__cdecl*elementCompareFunc)(void*, const void*, const void*) = (int(__cdecl*)(void*, const void*, const void*))elementCompare;

        void * contextToPass[] = { typedArrayBase, compareFn };

        // We can always call qsort_s with the same arguments. If user compareFn is non-null, the callback will use it to do the comparison.
        qsort_s(typedArrayBase->GetByteBuffer(), length, typedArrayBase->GetBytesPerElement(), elementCompareFunc, contextToPass);


        return typedArrayBase;
    }

    Var TypedArrayBase::EntryValues(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(TypedArray_Prototype_values);

        return GetKeysEntriesValuesHelper(args, scriptContext, _u("[TypedArray].prototype.values"), JavascriptArrayIteratorKind::Value);
    }

    BOOL TypedArrayBase::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        ENTER_PINNED_SCOPE(JavascriptString, toStringResult);
        toStringResult = JavascriptObject::ToStringInternal(this, requestContext);
        stringBuilder->Append(toStringResult->GetString(), toStringResult->GetLength());
        LEAVE_PINNED_SCOPE();
        return TRUE;
    }

    BOOL TypedArrayBase::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        switch(GetTypeId())
        {
        case TypeIds_Int8Array:
            stringBuilder->AppendCppLiteral(_u("Object, (Int8Array)"));
            break;

        case TypeIds_Uint8Array:
            stringBuilder->AppendCppLiteral(_u("Object, (Uint8Array)"));
            break;

        case TypeIds_Uint8ClampedArray:
            stringBuilder->AppendCppLiteral(_u("Object, (Uint8ClampedArray)"));
            break;

        case TypeIds_Int16Array:
            stringBuilder->AppendCppLiteral(_u("Object, (Int16Array)"));
            break;

        case TypeIds_Uint16Array:
            stringBuilder->AppendCppLiteral(_u("Object, (Uint16Array)"));
            break;

        case TypeIds_Int32Array:
            stringBuilder->AppendCppLiteral(_u("Object, (Int32Array)"));
            break;

        case TypeIds_Uint32Array:
            stringBuilder->AppendCppLiteral(_u("Object, (Uint32Array)"));
            break;

        case TypeIds_Float32Array:
            stringBuilder->AppendCppLiteral(_u("Object, (Float32Array)"));
            break;

        case TypeIds_Float64Array:
            stringBuilder->AppendCppLiteral(_u("Object, (Float64Array)"));
            break;

        default:
            Assert(false);
            stringBuilder->AppendCppLiteral(_u("Object"));
            break;
        }

        return TRUE;
    }

    bool TypedArrayBase::TryGetLengthForOptimizedTypedArray(const Var var, uint32 *const lengthRef, TypeId *const typeIdRef)
    {
        Assert(var);
        Assert(lengthRef);
        Assert(typeIdRef);

        if(!RecyclableObject::Is(var))
        {
            return false;
        }

        const TypeId typeId = RecyclableObject::FromVar(var)->GetTypeId();
        switch(typeId)
        {
            case TypeIds_Int8Array:
            case TypeIds_Uint8Array:
            case TypeIds_Uint8ClampedArray:
            case TypeIds_Int16Array:
            case TypeIds_Uint16Array:
            case TypeIds_Int32Array:
            case TypeIds_Uint32Array:
            case TypeIds_Float32Array:
            case TypeIds_Float64Array:
                Assert(ValueType::FromTypeId(typeId,false).IsOptimizedTypedArray());
                *lengthRef = FromVar(var)->GetLength();
                *typeIdRef = typeId;
                return true;
        }

        Assert(!ValueType::FromTypeId(typeId,false).IsOptimizedTypedArray());
        return false;
    }

    _Use_decl_annotations_ BOOL TypedArrayBase::ValidateIndexAndDirectSetItem(Js::Var index, Js::Var value, bool * isNumericIndex)
    {
        bool skipSetItem = false;
        uint32 indexToSet = ValidateAndReturnIndex(index, &skipSetItem, isNumericIndex);

        // If index is not numeric, goto [[Set]] property path
        if (*isNumericIndex)
        {
            return skipSetItem ?
                DirectSetItemNoSet(indexToSet, value) :
                DirectSetItem(indexToSet, value);
        }
        else
        {
            return TRUE;
        }
    }

    // Validate the index used for typed arrays with below rules:
    // 1. if numeric string, let sIndex = ToNumber(index) :
    //    a. if sIndex is not integer, skip set operation
    //    b. if sIndex == -0, skip set operation
    //    c. if sIndex < 0 or sIndex >= length, skip set operation
    //    d. else return sIndex and perform set operation
    // 2. if tagged int, let nIndex = untag(index) :
    //    a. if nIndex < 0 or nIndex >= length, skip set operation
    // 3. else:
    //    a. if index is not integer, skip set operation
    //    b. if index < 0 or index >= length, skip set operation
    //    NOTE: if index == -0, it is treated as 0 and perform set operation
    //          as per 7.1.12.1 of ES6 spec 7.1.12.1 ToString Applied to the Number Type
    _Use_decl_annotations_ uint32 TypedArrayBase::ValidateAndReturnIndex(Js::Var index, bool * skipOperation, bool * isNumericIndex)
    {
        *skipOperation = false;
        *isNumericIndex = true;
        uint32 length = GetLength();

        if (TaggedInt::Is(index))
        {
            int32 indexInt = TaggedInt::ToInt32(index);
            *skipOperation = (indexInt < 0 || (uint32)indexInt >= length);
            return (uint32)indexInt;
        }
        else
        {
            double dIndexValue = 0;
            if (JavascriptString::Is(index))
            {
                if (JavascriptConversion::CanonicalNumericIndexString(JavascriptString::FromVar(index), &dIndexValue, GetScriptContext()))
                {
                    if (JavascriptNumber::IsNegZero(dIndexValue))
                    {
                        *skipOperation = true;
                        return 0;
                    }
                    // If this is numeric index embedded in string, perform regular numeric index checks below
                }
                else
                {
                    // not numeric index, go the [[Set]] path to add as string property
                    *isNumericIndex = false;
                    return 0;
                }
            }
            else
            {
                // JavascriptNumber::Is_NoTaggedIntCheck(index)
                dIndexValue = JavascriptNumber::GetValue(index);
            }

            // OK to lose data because we want to verify ToInteger()
            uint32 uint32Index = (uint32)dIndexValue;

            // IsInteger()
            if ((double)uint32Index != dIndexValue)
            {
                *skipOperation = true;
            }
            // index >= length
            else if (uint32Index >= GetLength())
            {
                *skipOperation = true;
            }
            return uint32Index;
        }
    }

    // static
    Var TypedArrayBase::GetDefaultConstructor(Var object, ScriptContext* scriptContext)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(object);
        Var defaultConstructor = nullptr;
        switch (typeId)
        {
        case TypeId::TypeIds_Int8Array:
            defaultConstructor = scriptContext->GetLibrary()->GetInt8ArrayConstructor();
            break;
        case TypeId::TypeIds_Uint8Array:
            defaultConstructor = scriptContext->GetLibrary()->GetUint8ArrayConstructor();
            break;
        case TypeId::TypeIds_Uint8ClampedArray:
            defaultConstructor = scriptContext->GetLibrary()->GetUint8ClampedArrayConstructor();
            break;
        case TypeId::TypeIds_Int16Array:
            defaultConstructor = scriptContext->GetLibrary()->GetInt16ArrayConstructor();
            break;
        case TypeId::TypeIds_Uint16Array:
            defaultConstructor = scriptContext->GetLibrary()->GetUint16ArrayConstructor();
            break;
        case TypeId::TypeIds_Int32Array:
            defaultConstructor = scriptContext->GetLibrary()->GetInt32ArrayConstructor();
            break;
        case TypeId::TypeIds_Uint32Array:
            defaultConstructor = scriptContext->GetLibrary()->GetUint32ArrayConstructor();
            break;
        case TypeId::TypeIds_Float32Array:
            defaultConstructor = scriptContext->GetLibrary()->GetFloat32ArrayConstructor();
            break;
        case TypeId::TypeIds_Float64Array:
            defaultConstructor = scriptContext->GetLibrary()->GetFloat64ArrayConstructor();
            break;
        default:
            Assert(false);
        }
        return defaultConstructor;
    }

    Var TypedArrayBase::FindMinOrMax(Js::ScriptContext * scriptContext, TypeId typeId, bool findMax)
    {
        if (this->IsDetachedBuffer()) // 9.4.5.8 IntegerIndexedElementGet
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
        }

        switch (typeId)
        {
        case TypeIds_Int8Array:
            return this->FindMinOrMax<int8, false>(scriptContext, findMax);

        case TypeIds_Uint8Array:
        case TypeIds_Uint8ClampedArray:
            return this->FindMinOrMax<uint8, false>(scriptContext, findMax);

        case TypeIds_Int16Array:
            return this->FindMinOrMax<int16, false>(scriptContext, findMax);

        case TypeIds_Uint16Array:
            return this->FindMinOrMax<uint16, false>(scriptContext, findMax);

        case TypeIds_Int32Array:
            return this->FindMinOrMax<int32, false>(scriptContext, findMax);

        case TypeIds_Uint32Array:
            return this->FindMinOrMax<uint32, false>(scriptContext, findMax);

        case TypeIds_Float32Array:
            return this->FindMinOrMax<float, true>(scriptContext, findMax);

        case TypeIds_Float64Array:
            return this->FindMinOrMax<double, true>(scriptContext, findMax);

        default:
            AssertMsg(false, "Unsupported array for fast path");
            return nullptr;
        }
    }

    template<typename T, bool checkNaNAndNegZero>
    Var TypedArrayBase::FindMinOrMax(Js::ScriptContext * scriptContext, bool findMax)
    {
        T* typedBuffer = (T*)this->buffer;
        uint len = this->GetLength();

        Assert(sizeof(T)+GetByteOffset() <= GetArrayBuffer()->GetByteLength());
        T currentRes = typedBuffer[0];
        for (uint i = 0; i < len; i++)
        {
            Assert((i + 1) * sizeof(T)+GetByteOffset() <= GetArrayBuffer()->GetByteLength());
            T compare = typedBuffer[i];
            if (checkNaNAndNegZero && JavascriptNumber::IsNan(double(compare)))
            {
                return scriptContext->GetLibrary()->GetNaN();
            }
            if (findMax ? currentRes < compare : currentRes > compare ||
                (checkNaNAndNegZero && compare == 0 && Js::JavascriptNumber::IsNegZero(double(currentRes))))
            {
                currentRes = compare;
            }
        }
        return Js::JavascriptNumber::ToVarNoCheck(currentRes, scriptContext);
    }

    // static
    TypedArrayBase * TypedArrayBase::ValidateTypedArray(Arguments &args, ScriptContext *scriptContext, LPCWSTR apiName)
    {
        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        return ValidateTypedArray(args[0], scriptContext, apiName);
    }

    // static
    TypedArrayBase* TypedArrayBase::ValidateTypedArray(Var aValue, ScriptContext *scriptContext, LPCWSTR apiName)
    {
        if (!TypedArrayBase::Is(aValue))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray);
        }

        TypedArrayBase *typedArrayBase = TypedArrayBase::FromVar(aValue);

        if (typedArrayBase->IsDetachedBuffer())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, apiName);
        }

        return typedArrayBase;
    }

    // static
    Var TypedArrayBase::TypedArrayCreate(Var constructor, Arguments *args, uint32 length, ScriptContext *scriptContext)
    {
        Var newObj = JavascriptOperators::NewScObject(constructor, *args, scriptContext);

        TypedArrayBase::ValidateTypedArray(newObj, scriptContext, nullptr);

        // ECMA262 22.2.4.6 TypedArrayCreate line 3. "If argumentList is a List of a single Number" (args[0] == constructor)
        if (args->Info.Count == 2 && TypedArrayBase::FromVar(newObj)->GetLength() < length)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidTypedArrayLength);
        }

        return newObj;
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType TypedArrayBase::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapTypedArrayObject;
    }

    void TypedArrayBase::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapTypedArrayInfo* stai = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapTypedArrayInfo>();
        stai->ArrayBufferAddr = TTD_CONVERT_VAR_TO_PTR_ID(this->GetArrayBuffer());
        stai->ByteOffset = this->GetByteOffset();
        stai->Length = this->GetLength();

        TTD_PTR_ID* depArray = alloc.SlabAllocateArray<TTD_PTR_ID>(1);
        depArray[0] = TTD_CONVERT_VAR_TO_PTR_ID(this->GetArrayBuffer());

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapTypedArrayInfo*, TTD::NSSnapObjects::SnapObjectType::SnapTypedArrayObject>(objData, stai, alloc, 1, depArray);
    }
#endif

    template<>
    inline BOOL Int8Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToInt8);
    }

    template<>
    inline BOOL Int8VirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToInt8);
    }

    template<>
    inline BOOL Int8Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToInt8);
    }

    template<>
    inline BOOL Int8VirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToInt8);
    }

    template<>
    inline Var Int8Array::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

#define DIRECT_SET_NO_DETACH_CHECK(TypedArrayName, convertFn) \
    template<> \
    inline BOOL TypedArrayName##::DirectSetItemNoDetachCheck(__in uint32 index, __in Var value) \
    { \
        return BaseTypedDirectSetItemNoDetachCheck(index, value, convertFn); \
    }

    DIRECT_SET_NO_DETACH_CHECK(Int8Array, JavascriptConversion::ToInt8);
    DIRECT_SET_NO_DETACH_CHECK(Int8VirtualArray, JavascriptConversion::ToInt8);
    DIRECT_SET_NO_DETACH_CHECK(Uint8Array, JavascriptConversion::ToUInt8);
    DIRECT_SET_NO_DETACH_CHECK(Uint8VirtualArray, JavascriptConversion::ToUInt8);
    DIRECT_SET_NO_DETACH_CHECK(Int16Array, JavascriptConversion::ToInt16);
    DIRECT_SET_NO_DETACH_CHECK(Int16VirtualArray, JavascriptConversion::ToInt16);
    DIRECT_SET_NO_DETACH_CHECK(Uint16Array, JavascriptConversion::ToUInt16);
    DIRECT_SET_NO_DETACH_CHECK(Uint16VirtualArray, JavascriptConversion::ToUInt16);
    DIRECT_SET_NO_DETACH_CHECK(Int32Array, JavascriptConversion::ToInt32);
    DIRECT_SET_NO_DETACH_CHECK(Int32VirtualArray, JavascriptConversion::ToInt32);
    DIRECT_SET_NO_DETACH_CHECK(Uint32Array, JavascriptConversion::ToUInt32);
    DIRECT_SET_NO_DETACH_CHECK(Uint32VirtualArray, JavascriptConversion::ToUInt32);
    DIRECT_SET_NO_DETACH_CHECK(Float32Array, JavascriptConversion::ToFloat);
    DIRECT_SET_NO_DETACH_CHECK(Float32VirtualArray, JavascriptConversion::ToFloat);
    DIRECT_SET_NO_DETACH_CHECK(Float64Array, JavascriptConversion::ToNumber);
    DIRECT_SET_NO_DETACH_CHECK(Float64VirtualArray, JavascriptConversion::ToNumber);
    DIRECT_SET_NO_DETACH_CHECK(Int64Array, JavascriptConversion::ToInt64);
    DIRECT_SET_NO_DETACH_CHECK(Uint64Array, JavascriptConversion::ToUInt64);
    DIRECT_SET_NO_DETACH_CHECK(Uint8ClampedArray, JavascriptConversion::ToUInt8Clamped);
    DIRECT_SET_NO_DETACH_CHECK(Uint8ClampedVirtualArray, JavascriptConversion::ToUInt8Clamped);
    DIRECT_SET_NO_DETACH_CHECK(BoolArray, JavascriptConversion::ToBool);

#define DIRECT_GET_NO_DETACH_CHECK(TypedArrayName) \
    template<> \
    inline Var TypedArrayName##::DirectGetItemNoDetachCheck(__in uint32 index) \
    { \
        return BaseTypedDirectGetItemNoDetachCheck(index); \
    }

#define DIRECT_GET_VAR_CHECK_NO_DETACH_CHECK(TypedArrayName) \
    template<> \
    inline Var TypedArrayName##::DirectGetItemNoDetachCheck(__in uint32 index) \
    { \
        return DirectGetItemVarCheckNoDetachCheck(index); \
    }

    DIRECT_GET_NO_DETACH_CHECK(Int8Array);
    DIRECT_GET_NO_DETACH_CHECK(Int8VirtualArray);
    DIRECT_GET_NO_DETACH_CHECK(Uint8Array);
    DIRECT_GET_NO_DETACH_CHECK(Uint8VirtualArray);
    DIRECT_GET_NO_DETACH_CHECK(Int16Array);
    DIRECT_GET_NO_DETACH_CHECK(Int16VirtualArray);
    DIRECT_GET_NO_DETACH_CHECK(Uint16Array);
    DIRECT_GET_NO_DETACH_CHECK(Uint16VirtualArray);
    DIRECT_GET_NO_DETACH_CHECK(Int32Array);
    DIRECT_GET_NO_DETACH_CHECK(Int32VirtualArray);
    DIRECT_GET_NO_DETACH_CHECK(Uint32Array);
    DIRECT_GET_NO_DETACH_CHECK(Uint32VirtualArray);
    DIRECT_GET_NO_DETACH_CHECK(Int64Array);
    DIRECT_GET_NO_DETACH_CHECK(Uint64Array);
    DIRECT_GET_NO_DETACH_CHECK(Uint8ClampedArray);
    DIRECT_GET_NO_DETACH_CHECK(Uint8ClampedVirtualArray);
    DIRECT_GET_VAR_CHECK_NO_DETACH_CHECK(Float32Array);
    DIRECT_GET_VAR_CHECK_NO_DETACH_CHECK(Float32VirtualArray);
    DIRECT_GET_VAR_CHECK_NO_DETACH_CHECK(Float64Array);
    DIRECT_GET_VAR_CHECK_NO_DETACH_CHECK(Float64VirtualArray);

#define TypedArrayBeginStub(type) \
        Assert(GetArrayBuffer() || GetArrayBuffer()->GetBuffer()); \
        Assert(index < GetLength()); \
        ScriptContext *scriptContext = GetScriptContext(); \
        type *buffer = (type*)this->buffer + index;

#ifdef _WIN32
#define InterlockedExchangeAdd8 _InterlockedExchangeAdd8
#define InterlockedExchangeAdd16 _InterlockedExchangeAdd16

#define InterlockedAnd8 _InterlockedAnd8
#define InterlockedAnd16 _InterlockedAnd16

#define InterlockedOr8 _InterlockedOr8
#define InterlockedOr16 _InterlockedOr16

#define InterlockedXor8 _InterlockedXor8
#define InterlockedXor16 _InterlockedXor16

#define InterlockedCompareExchange8 _InterlockedCompareExchange8
#define InterlockedCompareExchange16 _InterlockedCompareExchange16

#define InterlockedExchange8 _InterlockedExchange8
#define InterlockedExchange16 _InterlockedExchange16
#endif

#define InterlockedExchangeAdd32 InterlockedExchangeAdd
#define InterlockedAnd32 InterlockedAnd
#define InterlockedOr32 InterlockedOr
#define InterlockedXor32 InterlockedXor
#define InterlockedCompareExchange32 InterlockedCompareExchange
#define InterlockedExchange32 InterlockedExchange

#define TypedArrayAddOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedAdd(__in uint32 index, __in Var second) \
    { \
        TypedArrayBeginStub(type); \
        type result = (type)InterlockedExchangeAdd##bit((convertType*)buffer, (convertType)convertFn(second, scriptContext)); \
        return JavascriptNumber::ToVar(result, scriptContext); \
    }

#define TypedArrayAndOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedAnd(__in uint32 index, __in Var second) \
    { \
        TypedArrayBeginStub(type); \
        type result = (type)InterlockedAnd##bit((convertType*)buffer, (convertType)convertFn(second, scriptContext)); \
        return JavascriptNumber::ToVar(result, scriptContext); \
    }

#define TypedArrayCompareExchangeOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedCompareExchange(__in uint32 index, __in Var comparand, __in Var replacementValue) \
    { \
        TypedArrayBeginStub(type); \
        type result = (type)InterlockedCompareExchange##bit((convertType*)buffer, (convertType)convertFn(replacementValue, scriptContext), (convertType)convertFn(comparand, scriptContext)); \
        return JavascriptNumber::ToVar(result, scriptContext); \
    }

#define TypedArrayExchangeOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedExchange(__in uint32 index, __in Var second) \
    { \
        TypedArrayBeginStub(type); \
        type result = (type)InterlockedExchange##bit((convertType*)buffer, (convertType)convertFn(second, scriptContext)); \
        return JavascriptNumber::ToVar(result, scriptContext); \
    }

#define TypedArrayLoadOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedLoad(__in uint32 index) \
    { \
        TypedArrayBeginStub(type); \
        MemoryBarrier(); \
        type result = (type)*buffer; \
        return JavascriptNumber::ToVar(result, scriptContext); \
    }

#define TypedArrayOrOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedOr(__in uint32 index, __in Var second) \
    { \
        TypedArrayBeginStub(type); \
        type result = (type)InterlockedOr##bit((convertType*)buffer, (convertType)convertFn(second, scriptContext)); \
        return JavascriptNumber::ToVar(result, scriptContext); \
    }

    // Currently the TypedStore is just using the InterlockedExchange to store the value in the buffer.
    // TODO The InterlockedExchange will have the sequential consistency any way, not sure why do we need the Memory barrier or std::atomic::store to perform this.

#define TypedArrayStoreOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedStore(__in uint32 index, __in Var second) \
    { \
        TypedArrayBeginStub(type); \
        double d = JavascriptConversion::ToInteger(second, scriptContext); \
        convertType s = (convertType)JavascriptConversion::ToUInt32(d); \
        InterlockedExchange##bit((convertType*)buffer, s); \
        return JavascriptNumber::ToVarWithCheck(d, scriptContext); \
    }

#define TypedArraySubOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedSub(__in uint32 index, __in Var second) \
    { \
        TypedArrayBeginStub(type); \
        type result = (type)InterlockedExchangeAdd##bit((convertType*)buffer, - (convertType)convertFn(second, scriptContext)); \
        return JavascriptNumber::ToVar(result, scriptContext); \
    }

#define TypedArrayXorOp(TypedArrayName, bit, type, convertType, convertFn) \
    template<> \
    inline Var TypedArrayName##::TypedXor(__in uint32 index, __in Var second) \
    { \
        TypedArrayBeginStub(type); \
        type result = (type)InterlockedXor##bit((convertType*)buffer, (convertType)convertFn(second, scriptContext)); \
        return JavascriptNumber::ToVar(result, scriptContext); \
    }

#define GenerateNotSupportedStub1(TypedArrayName, fnName) \
    template<> \
    inline Var TypedArrayName##::Typed##fnName(__in uint32 accessIndex) \
    { \
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray); \
    }

#define GenerateNotSupportedStub2(TypedArrayName, fnName) \
    template<> \
    inline Var TypedArrayName##::Typed##fnName(__in uint32 accessIndex, __in Var value) \
    { \
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray); \
    }

#define GenerateNotSupportedStub3(TypedArrayName, fnName) \
    template<> \
    inline Var TypedArrayName##::Typed##fnName(__in uint32 accessIndex, __in Var first, __in Var value) \
    { \
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray); \
    }

#define GENERATE_FOREACH_TYPEDARRAY(TYPEDARRAY_DEF, NOTSUPPORTEDSTUB, OP) \
        TYPEDARRAY_DEF(Int8Array, 8, int8, char, JavascriptConversion::ToInt8); \
        TYPEDARRAY_DEF(Int8VirtualArray, 8, int8, char, JavascriptConversion::ToInt8); \
        TYPEDARRAY_DEF(Uint8Array, 8, uint8, char, JavascriptConversion::ToUInt8); \
        TYPEDARRAY_DEF(Uint8VirtualArray, 8, uint8, char, JavascriptConversion::ToUInt8); \
        TYPEDARRAY_DEF(Int16Array, 16, int16, short, JavascriptConversion::ToInt16); \
        TYPEDARRAY_DEF(Int16VirtualArray, 16, int16, short, JavascriptConversion::ToInt16); \
        TYPEDARRAY_DEF(Uint16Array, 16, uint16, short, JavascriptConversion::ToUInt16); \
        TYPEDARRAY_DEF(Uint16VirtualArray, 16, uint16, short, JavascriptConversion::ToUInt16); \
        TYPEDARRAY_DEF(Int32Array, 32, int32, LONG, JavascriptConversion::ToInt32); \
        TYPEDARRAY_DEF(Int32VirtualArray, 32, int32, LONG, JavascriptConversion::ToInt32); \
        TYPEDARRAY_DEF(Uint32Array, 32, uint32, LONG, JavascriptConversion::ToUInt32); \
        TYPEDARRAY_DEF(Uint32VirtualArray, 32, uint32, LONG, JavascriptConversion::ToUInt32); \
        NOTSUPPORTEDSTUB(Float32Array, OP); \
        NOTSUPPORTEDSTUB(Float32VirtualArray, OP); \
        NOTSUPPORTEDSTUB(Float64Array, OP); \
        NOTSUPPORTEDSTUB(Float64VirtualArray, OP); \
        NOTSUPPORTEDSTUB(Int64Array, OP); \
        NOTSUPPORTEDSTUB(Uint64Array, OP); \
        NOTSUPPORTEDSTUB(Uint8ClampedArray, OP); \
        NOTSUPPORTEDSTUB(Uint8ClampedVirtualArray, OP); \
        NOTSUPPORTEDSTUB(BoolArray, OP);

    GENERATE_FOREACH_TYPEDARRAY(TypedArrayAddOp, GenerateNotSupportedStub2, Add)
    GENERATE_FOREACH_TYPEDARRAY(TypedArrayAndOp, GenerateNotSupportedStub2, And)
    GENERATE_FOREACH_TYPEDARRAY(TypedArrayCompareExchangeOp, GenerateNotSupportedStub3, CompareExchange)
    GENERATE_FOREACH_TYPEDARRAY(TypedArrayExchangeOp, GenerateNotSupportedStub2, Exchange)
    GENERATE_FOREACH_TYPEDARRAY(TypedArrayLoadOp, GenerateNotSupportedStub1, Load)
    GENERATE_FOREACH_TYPEDARRAY(TypedArrayOrOp, GenerateNotSupportedStub2, Or)
    GENERATE_FOREACH_TYPEDARRAY(TypedArrayStoreOp, GenerateNotSupportedStub2, Store)
    GENERATE_FOREACH_TYPEDARRAY(TypedArraySubOp, GenerateNotSupportedStub2, Sub)
    GENERATE_FOREACH_TYPEDARRAY(TypedArrayXorOp, GenerateNotSupportedStub2, Xor)

    template<>
    VTableValue Int8Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableInt8Array;
    }

    template<>
    inline Var Int8VirtualArray::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Int8VirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableInt8VirtualArray;
    }

    template<>
    inline BOOL Uint8Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt8);
    }

    template<>
    inline BOOL Uint8Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt8);
    }

    template<>
    inline Var Uint8Array::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint8Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint8Array;
    }

    template<>
    inline BOOL Uint8VirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt8);
    }

    template<>
    inline BOOL Uint8VirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt8);
    }

    template<>
    inline Var Uint8VirtualArray::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint8VirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint8VirtualArray;
    }

    template<>
    inline BOOL Uint8ClampedArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt8Clamped);
    }

    template<>
    inline BOOL Uint8ClampedArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt8Clamped);
    }

    template<>
    inline Var Uint8ClampedArray::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint8ClampedArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint8ClampedArray;
    }

    template<>
    inline BOOL Uint8ClampedVirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt8Clamped);
    }

    template<>
    inline BOOL Uint8ClampedVirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt8Clamped);
    }

    template<>
    inline Var Uint8ClampedVirtualArray::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint8ClampedVirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint8ClampedVirtualArray;
    }

    template<>
    inline BOOL Int16Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToInt16);
    }

    template<>
    inline BOOL Int16Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToInt16);
    }

    template<>
    inline Var Int16Array::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Int16Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableInt16Array;
    }

    template<>
    inline BOOL Int16VirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToInt16);
    }

    template<>
    inline BOOL Int16VirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToInt16);
    }

    template<>
    inline Var Int16VirtualArray::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Int16VirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableInt16VirtualArray;
    }

    template<>
    inline BOOL Uint16Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt16);
    }

    template<>
    inline BOOL Uint16Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt16);
    }

    template<>
    inline Var Uint16Array::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint16Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint16Array;
    }

    template<>
    inline BOOL Uint16VirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt16);
    }

    template<>
    inline BOOL Uint16VirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt16);
    }

    template<>
    inline Var Uint16VirtualArray::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint16VirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint16VirtualArray;
    }

    template<>
    inline BOOL Int32Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToInt32);
    }

    template<>
    inline BOOL Int32Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToInt32);
    }

    template<>
    inline Var Int32Array::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Int32Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableInt32Array;
    }

    template<>
    inline BOOL Int32VirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToInt32);
    }

    template<>
    inline BOOL Int32VirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToInt32);
    }

    template<>
    inline Var Int32VirtualArray::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Int32VirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableInt32VirtualArray;
    }

    template<>
    inline BOOL Uint32Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt32);
    }

    template<>
    inline BOOL Uint32Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt32);
    }

    template<>
    inline Var Uint32Array::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint32Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint32Array;
    }

    template<>
    inline BOOL Uint32VirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt32);
    }

    template<>
    inline BOOL Uint32VirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt32);
    }

    template<>
    inline Var Uint32VirtualArray::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint32VirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint32VirtualArray;
    }

    template<>
    inline BOOL Float32Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToFloat);
    }

    template<>
    inline BOOL Float32Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToFloat);
    }

    template<>
    Var Float32Array::DirectGetItem(__in uint32 index)
    {
        return TypedDirectGetItemWithCheck(index);
    }

    template<>
    VTableValue Float32Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableFloat32Array;
    }

    template<>
    inline BOOL Float32VirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToFloat);
    }

    template<>
    inline BOOL Float32VirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToFloat);
    }

    template<>
    Var Float32VirtualArray::DirectGetItem(__in uint32 index)
    {
        return TypedDirectGetItemWithCheck(index);
    }

    template<>
    VTableValue Float32VirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableFloat32VirtualArray;
    }

    template<>
    inline BOOL Float64Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToNumber);
    }

    template<>
    inline BOOL Float64Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToNumber);
    }

    template<>
    inline Var Float64Array::DirectGetItem(__in uint32 index)
    {
        return TypedDirectGetItemWithCheck(index);
    }

    template<>
    VTableValue Float64Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableFloat64Array;
    }

    template<>
    inline BOOL Float64VirtualArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToNumber);
    }

    template<>
    inline BOOL Float64VirtualArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToNumber);
    }

    template<>
    inline Var Float64VirtualArray::DirectGetItem(__in uint32 index)
    {
        return TypedDirectGetItemWithCheck(index);
    }

    template<>
    VTableValue Float64VirtualArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableFloat64VirtualArray;
    }

    template<>
    inline BOOL Int64Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToInt64);
    }

    template<>
    inline BOOL Int64Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToInt64);
    }

    template<>
    inline Var Int64Array::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Int64Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableInt64Array;
    }

    template<>
    inline BOOL Uint64Array::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToUInt64);
    }

    template<>
    inline BOOL Uint64Array::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToUInt64);
    }

    template<>
    inline Var Uint64Array::DirectGetItem(__in uint32 index)
    {
        return BaseTypedDirectGetItem(index);
    }

    template<>
    VTableValue Uint64Array::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableUint64Array;
    }

    template<>
    inline BOOL BoolArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItem(index, value, JavascriptConversion::ToBool);
    }

    template<>
    inline BOOL BoolArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        return BaseTypedDirectSetItemNoSet(index, value, JavascriptConversion::ToBool);
    }

    template<>
    inline Var BoolArray::DirectGetItem(__in uint32 index)
    {
        if (index < GetLength())
        {
            Assert((index+1)* sizeof(bool) +GetByteOffset() <= GetArrayBuffer()->GetByteLength());
            bool* typedBuffer = (bool*)buffer;
            return typedBuffer[index] ? GetLibrary()->GetTrue() : GetLibrary()->GetFalse();
        }
        return GetLibrary()->GetUndefined();
    }

    template<>
    inline Var BoolArray::DirectGetItemNoDetachCheck(__in uint32 index)
    {
        Assert((index + 1)* sizeof(bool) + GetByteOffset() <= GetArrayBuffer()->GetByteLength());
        bool* typedBuffer = (bool*)buffer;
        return typedBuffer[index] ? GetLibrary()->GetTrue() : GetLibrary()->GetFalse();
    }

    template<>
    VTableValue BoolArray::DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableBoolArray;
    }


    Var CharArray::Create(ArrayBufferBase* arrayBuffer, uint32 byteOffSet, uint32 mappedLength, JavascriptLibrary* javascriptLibrary)
    {
        CharArray* arr;
        uint32 totalLength, mappedByteLength;
        if (UInt32Math::Mul(mappedLength, sizeof(char16), &mappedByteLength) ||
            UInt32Math::Add(byteOffSet, mappedByteLength, &totalLength) ||
            (totalLength > arrayBuffer->GetByteLength()))
        {
            JavascriptError::ThrowRangeError(arrayBuffer->GetScriptContext(), JSERR_InvalidTypedArrayLength);
        }
        arr = RecyclerNew(javascriptLibrary->GetRecycler(), CharArray, arrayBuffer, byteOffSet, mappedLength, javascriptLibrary->GetCharArrayType());
        return arr;
    }

    BOOL CharArray::Is(Var value)
    {
        return JavascriptOperators::GetTypeId(value) == Js::TypeIds_CharArray;
    }

    Var CharArray::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        AssertMsg(FALSE, "not supported in char array");
        JavascriptError::ThrowTypeError(function->GetScriptContext(), JSERR_This_NeedTypedArray);
    }

    Var CharArray::EntrySubarray(RecyclableObject* function, CallInfo callInfo, ...)
    {
        AssertMsg(FALSE, "not supported in char array");
        JavascriptError::ThrowTypeError(function->GetScriptContext(), JSERR_This_NeedTypedArray);
    }

    inline Var CharArray::Subarray(uint32 begin, uint32 end)
    {
        AssertMsg(FALSE, "not supported in char array");
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_This_NeedTypedArray);
    }

    inline CharArray* CharArray::FromVar(Var aValue)
    {
        AssertMsg(CharArray::Is(aValue), "invalid CharArray");
        return static_cast<CharArray*>(RecyclableObject::FromVar(aValue));
    }

    inline BOOL CharArray::DirectSetItem(__in uint32 index, __in Js::Var value)
    {
        ScriptContext* scriptContext = GetScriptContext();
        // A typed array is Integer Indexed Exotic object, so doing a get translates to 9.4.5.9 IntegerIndexedElementSet
        LPCWSTR asString = (Js::JavascriptConversion::ToString(value, scriptContext))->GetSz();

        if (this->IsDetachedBuffer())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray);
        }

        if (index >= GetLength())
        {
            return FALSE;
        }

        AssertMsg(index < GetLength(), "Trying to set out of bound index for typed array.");
        Assert((index + 1)* sizeof(char16) + GetByteOffset() <= GetArrayBuffer()->GetByteLength());
        char16* typedBuffer = (char16*)buffer;

        if (asString != NULL && ::wcslen(asString) == 1)
        {
            typedBuffer[index] = asString[0];
        }
        else
        {
            Js::JavascriptError::MapAndThrowError(scriptContext, E_INVALIDARG);
        }

        return TRUE;
    }

    inline BOOL CharArray::DirectSetItemNoSet(__in uint32 index, __in Js::Var value)
    {
        ScriptContext* scriptContext = GetScriptContext();
        // A typed array is Integer Indexed Exotic object, so doing a get translates to 9.4.5.9 IntegerIndexedElementSet
        Js::JavascriptConversion::ToString(value, scriptContext);

        if (this->IsDetachedBuffer())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray);
        }

        return FALSE;
    }

    inline BOOL CharArray::DirectSetItemNoDetachCheck(__in uint32 index, __in Js::Var value)
    {
        return DirectSetItem(index, value);
    }

    inline Var CharArray::DirectGetItemNoDetachCheck(__in uint32 index)
    {
        return DirectGetItem(index);
    }

    Var CharArray::TypedAdd(__in uint32 index, Var second)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    Var CharArray::TypedAnd(__in uint32 index, Var second)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    Var CharArray::TypedCompareExchange(__in uint32 index, Var comparand, Var replacementValue)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    Var CharArray::TypedExchange(__in uint32 index, Var second)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    Var CharArray::TypedLoad(__in uint32 index)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    Var CharArray::TypedOr(__in uint32 index, Var second)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    Var CharArray::TypedStore(__in uint32 index, Var second)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    Var CharArray::TypedSub(__in uint32 index, Var second)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    Var CharArray::TypedXor(__in uint32 index, Var second)
    {
        JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidOperationOnTypedArray);
    }

    inline Var CharArray::DirectGetItem(__in uint32 index)
    {
        // A typed array is Integer Indexed Exotic object, so doing a get translates to 9.4.5.8 IntegerIndexedElementGet
        if (this->IsDetachedBuffer())
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
        }
        if (index < GetLength())
        {
            Assert((index + 1)* sizeof(char16)+GetByteOffset() <= GetArrayBuffer()->GetByteLength());
            char16* typedBuffer = (char16*)buffer;
            return GetLibrary()->GetCharStringCache().GetStringForChar(typedBuffer[index]);
        }
        return GetLibrary()->GetUndefined();
    }

    Var CharArray::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        function->GetScriptContext()->GetThreadContext()->ProbeStack(Js::Constants::MinStackDefault, function->GetScriptContext());

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Assert(!(callInfo.Flags & CallFlags_New) || args[0] == nullptr);
        Var object = TypedArrayBase::CreateNewInstance(args, scriptContext, sizeof(char16), CharArray::Create);
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            object = Js::JavascriptProxy::AutoProxyWrapper(object);
        }
#endif
        return object;
    }
}

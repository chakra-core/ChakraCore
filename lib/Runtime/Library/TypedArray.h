//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//  Implements typed array.
//----------------------------------------------------------------------------
#pragma once

namespace Js
{
    typedef Var (*PFNCreateTypedArray)(Js::ArrayBufferBase* arrayBuffer, uint32 offSet, uint32 mappedLength, Js::JavascriptLibrary* javascriptLibrary);

    template<typename T> int __cdecl TypedArrayCompareElementsHelper(void* context, const void* elem1, const void* elem2);

    class TypedArrayBase : public ArrayBufferParent
    {
        friend ArrayBuffer;
        friend ArrayBufferBase;

    protected:
        DEFINE_VTABLE_CTOR_ABSTRACT(TypedArrayBase, ArrayBufferParent);

    public:
        static Var GetDefaultConstructor(Var object, ScriptContext* scriptContext);

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Set;
            static FunctionInfo Subarray;

            static FunctionInfo From;
            static FunctionInfo Of;
            static FunctionInfo CopyWithin;
            static FunctionInfo Entries;
            static FunctionInfo Every;
            static FunctionInfo Fill;
            static FunctionInfo Filter;
            static FunctionInfo Find;
            static FunctionInfo FindIndex;
            static FunctionInfo ForEach;
            static FunctionInfo IndexOf;
            static FunctionInfo Includes;
            static FunctionInfo Join;
            static FunctionInfo Keys;
            static FunctionInfo LastIndexOf;
            static FunctionInfo Map;
            static FunctionInfo Reduce;
            static FunctionInfo ReduceRight;
            static FunctionInfo Reverse;
            static FunctionInfo Slice;
            static FunctionInfo Some;
            static FunctionInfo Sort;
            static FunctionInfo Values;

            static FunctionInfo GetterBuffer;
            static FunctionInfo GetterByteLength;
            static FunctionInfo GetterByteOffset;
            static FunctionInfo GetterLength;
            static FunctionInfo GetterSymbolToStringTag;
            static FunctionInfo GetterSymbolSpecies;
        };

        TypedArrayBase(ArrayBufferBase* arrayBuffer, uint byteOffset, uint mappedLength, uint elementSize, DynamicType* type);

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntrySet(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySubarray(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFrom(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryOf(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryCopyWithin(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEntries(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEvery(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFill(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFilter(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFind(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFindIndex(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryForEach(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIndexOf(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIncludes(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJoin(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryKeys(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryLastIndexOf(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryMap(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReduce(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReduceRight(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReverse(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySlice(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySome(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySort(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryValues(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryGetterBuffer(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterByteOffset(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterLength(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterSymbolToStringTag(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...);

        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags HasPropertyQuery(Js::PropertyId propertyId) override;
        virtual BOOL HasOwnProperty(Js::PropertyId propertyId) override;
        virtual PropertyQueryFlags GetPropertyQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Js::Var originalInstance, Js::JavascriptString* propertyNameString, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext) override;
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override;
        virtual BOOL DeleteItem(uint32 index, Js::PropertyOperationFlags flags) override { return false; }
        virtual PropertyQueryFlags GetItemQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * requestContext) override;
        virtual BOOL SetItem(uint32 index, Js::Var value, Js::PropertyOperationFlags flags = PropertyOperation_None) override;
        virtual BOOL SetProperty(Js::PropertyId propertyId, Js::Var value, Js::PropertyOperationFlags flags, Js::PropertyValueInfo* info) override;
        virtual BOOL SetProperty(Js::JavascriptString* propertyNameString, Js::Var value, Js::PropertyOperationFlags flags, Js::PropertyValueInfo* info) override;
        virtual BOOL DeleteProperty(Js::PropertyId propertyId, Js::PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, Js::PropertyOperationFlags flags) override;
        virtual PropertyQueryFlags GetItemReferenceQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * requestContext) override;
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache = nullptr) override;
        virtual JavascriptEnumerator * GetIndexEnumerator(EnumeratorFlags flags, ScriptContext * requestContext) override;

        virtual BOOL IsEnumerable(PropertyId propertyId)  override;
        virtual BOOL IsConfigurable(PropertyId propertyId)  override;
        virtual BOOL IsWritable(PropertyId propertyId)  override;
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags) override;

        virtual BOOL InitProperty(Js::PropertyId propertyId, Js::Var value, PropertyOperationFlags flags = PropertyOperation_None, Js::PropertyValueInfo* info = NULL) override;
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        static BOOL Is(Var aValue);
        static BOOL Is(TypeId typeId);
        static TypedArrayBase* FromVar(Var aValue);
        // Returns false if this is not a TypedArray or it's not detached
        static BOOL IsDetachedTypedArray(Var aValue);
        static HRESULT GetBuffer(Var aValue, ArrayBuffer** outBuffer, uint32* outOffset, uint32* outLength);
        static TypedArrayBase * ValidateTypedArray(Var aValue, ScriptContext *scriptContext, LPCWSTR apiName);
        static TypedArrayBase * ValidateTypedArray(Arguments &args, ScriptContext *scriptContext, LPCWSTR apiName);
        static Var TypedArrayCreate(Var constructor, Arguments *args, uint32 length, ScriptContext *scriptContext);

        virtual BOOL DirectSetItem(__in uint32 index, __in Js::Var value) = 0;
        virtual BOOL DirectSetItemNoSet(__in uint32 index, __in Js::Var value) = 0;
        virtual Var  DirectGetItem(__in uint32 index) = 0;
        virtual BOOL DirectSetItemNoDetachCheck(__in uint32 index, __in Js::Var value) = 0;
        virtual Var  DirectGetItemNoDetachCheck(__in uint32 index) = 0;

        virtual Var TypedAdd(__in uint32 index, Var second) = 0;
        virtual Var TypedAnd(__in uint32 index, Var second) = 0;
        virtual Var TypedLoad(__in uint32 index) = 0;
        virtual Var TypedOr(__in uint32 index, Var second) = 0;
        virtual Var TypedStore(__in uint32 index, Var second) = 0;
        virtual Var TypedSub(__in uint32 index, Var second) = 0;
        virtual Var TypedXor(__in uint32 index, Var second) = 0;
        virtual Var TypedExchange(__in uint32 index, Var second) = 0;
        virtual Var TypedCompareExchange(__in uint32 index, Var comparand, Var replacementValue) = 0;

        uint32 GetByteLength() const { return length * BYTES_PER_ELEMENT; }
        uint32 GetByteOffset() const { return byteOffset; }
        uint32 GetBytesPerElement() const { return BYTES_PER_ELEMENT; }
        byte*  GetByteBuffer() const { return buffer; };
        bool IsDetachedBuffer() const { return this->GetArrayBuffer()->IsDetached(); }
        void ClearLengthAndBufferOnDetach();
        static Var CommonSet(Arguments& args);
        static Var CommonSubarray(Arguments& args);

        void SetObject(RecyclableObject* arraySource, uint32 targetLength, uint32 offset = 0);
        void SetObjectNoDetachCheck(RecyclableObject* arraySource, uint32 targetLength, uint32 offset = 0);
        void Set(TypedArrayBase* typedArraySource, uint32 offset = 0);

        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        static bool TryGetLengthForOptimizedTypedArray(const Var var, uint32 *const lengthRef, TypeId *const typeIdRef);
        BOOL ValidateIndexAndDirectSetItem(__in Js::Var index, __in Js::Var value, __out bool * isNumericIndex);
        uint32 ValidateAndReturnIndex(__in Js::Var index, __out bool * skipOperation, __out bool * isNumericIndex);

        // objectArray support
        virtual BOOL SetItemWithAttributes(uint32 index, Var value, PropertyAttributes attributes) override;

        Var FindMinOrMax(Js::ScriptContext * scriptContext, TypeId typeId, bool findMax);
        template<typename T, bool checkNaNAndNegZero> Var FindMinOrMax(Js::ScriptContext * scriptContext, bool findMax);

        static Var GetKeysEntriesValuesHelper(Arguments& args, ScriptContext *scriptContext, LPCWSTR apiName, JavascriptArrayIteratorKind kind);

        static uint32 GetFromIndex(Var arg, uint32 length, ScriptContext *scriptContext);

    private:
        uint32 GetSourceLength(RecyclableObject* arraySource, uint32 targetLength, uint32 offset);

    protected:
        static Var CreateNewInstanceFromIterator(RecyclableObject *iterator, ScriptContext *scriptContext, uint32 elementSize, PFNCreateTypedArray pfnCreateTypedArray);
        static Var CreateNewInstance(Arguments& args, ScriptContext* scriptContext, uint32 elementSize, PFNCreateTypedArray pfnCreateTypedArray );
        static bool ArrayIteratorPrototypeHasUserDefinedNext(ScriptContext *scriptContext);
        static BOOL CanonicalNumericIndexString(PropertyId propertyId, ScriptContext *scriptContext);
        static BOOL CanonicalNumericIndexString(JavascriptString *propertyString, ScriptContext *scriptContext);

        typedef int(__cdecl* CompareElementsFunction)(void*, const void*, const void*);
        virtual CompareElementsFunction GetCompareElementsFunction() = 0;

        virtual Var Subarray(uint32 begin, uint32 end) = 0;
        Field(int32) BYTES_PER_ELEMENT;
        Field(uint32) byteOffset;
        FieldNoBarrier(BYTE*) buffer;   // beginning of mapped array.

    public:
        static uint32 GetOffsetOfBuffer()  { return offsetof(TypedArrayBase, buffer); }
        static uint32 GetOffsetOfLength()  { return offsetof(TypedArrayBase, length); }

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <typename TypeName, bool clamped = false, bool virtualAllocated = false>
    class TypedArray : public TypedArrayBase
    {
    protected:
        DEFINE_VTABLE_CTOR(TypedArray, TypedArrayBase);
        virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext)
        {
            Assert(this->GetScriptContext() != scriptContext);
            AssertMsg(VirtualTableInfo<TypedArray>::HasVirtualTable(this), "Derived class need to define marshal to script context");

            VirtualTableInfo<Js::CrossSiteObject<TypedArray<TypeName, clamped, virtualAllocated>>>::SetVirtualTable(this);
            ArrayBufferBase* arrayBuffer = this->GetArrayBuffer();
            if (arrayBuffer && !arrayBuffer->IsCrossSiteObject())
            {
                arrayBuffer->MarshalToScriptContext(scriptContext);
            }
        }

#if ENABLE_TTD
        virtual void MarshalCrossSite_TTDInflate()
        {
            AssertMsg(VirtualTableInfo<TypedArray>::HasVirtualTable(this), "Derived class need to define marshal");
            VirtualTableInfo<Js::CrossSiteObject<TypedArray<TypeName, clamped, virtualAllocated>>>::SetVirtualTable(this);
        }
#endif

        TypedArray(DynamicType *type): TypedArrayBase(nullptr, 0, 0, sizeof(TypeName), type) { buffer = nullptr; }

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Set;
        };

        TypedArray(ArrayBufferBase* arrayBuffer, uint32 byteOffset, uint32 mappedLength, DynamicType* type);

        static Var Create(ArrayBufferBase* arrayBuffer, uint32 byteOffSet, uint32 mappedLength, JavascriptLibrary* javascriptLibrary);
        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntrySet(RecyclableObject* function, CallInfo callInfo, ...);

        Var Subarray(uint32 begin, uint32 end);

        static BOOL Is(Var aValue);
        static TypedArray<TypeName, clamped, virtualAllocated>* FromVar(Var aValue);

        inline Var BaseTypedDirectGetItem(__in uint32 index)
        {
            if (this->IsDetachedBuffer()) // 9.4.5.8 IntegerIndexedElementGet
            {
                JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
            }

            if (index < GetLength())
            {
                Assert((index + 1)* sizeof(TypeName)+GetByteOffset() <= GetArrayBuffer()->GetByteLength());
                TypeName* typedBuffer = (TypeName*)buffer;
                return JavascriptNumber::ToVar(typedBuffer[index], GetScriptContext());
            }
            return GetLibrary()->GetUndefined();
        }

        inline Var TypedDirectGetItemWithCheck(__in uint32 index)
        {
            if (this->IsDetachedBuffer()) // 9.4.5.8 IntegerIndexedElementGet
            {
                JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
            }

            if (index < GetLength())
            {
                Assert((index + 1)* sizeof(TypeName)+GetByteOffset() <= GetArrayBuffer()->GetByteLength());
                TypeName* typedBuffer = (TypeName*)buffer;
                return JavascriptNumber::ToVarWithCheck(typedBuffer[index], GetScriptContext());
            }
            return GetLibrary()->GetUndefined();
        }

        inline Var BaseTypedDirectGetItemNoDetachCheck(__in uint32 index)
        {
            Assert(!IsDetachedBuffer());
            Assert(index < GetLength());
            Assert((index + 1)* sizeof(TypeName) + GetByteOffset() <= GetArrayBuffer()->GetByteLength());
            TypeName* typedBuffer = (TypeName*)buffer;
            return JavascriptNumber::ToVar(typedBuffer[index], GetScriptContext());
        }

        inline Var DirectGetItemVarCheckNoDetachCheck(__in uint32 index)
        {
            Assert(!IsDetachedBuffer());
            Assert(index < GetLength());
            Assert((index + 1)* sizeof(TypeName) + GetByteOffset() <= GetArrayBuffer()->GetByteLength());
            TypeName* typedBuffer = (TypeName*)buffer;
            return JavascriptNumber::ToVarWithCheck(typedBuffer[index], GetScriptContext());
        }

        inline BOOL DirectSetItemAtRange(TypedArray *fromArray, __in int32 iSrcStart, __in int32 iDstStart, __in uint32 length, TypeName(*convFunc)(Var value, ScriptContext* scriptContext))
        {
            TypeName* dstBuffer = (TypeName*)buffer;
            TypeName* srcBuffer = (TypeName*)fromArray->buffer;
            Assert(srcBuffer && dstBuffer);
            Assert(length <= ArrayBuffer::MaxArrayBufferLength / sizeof(TypeName));
            // caller checks that src and dst index are the same
            Assert(iSrcStart == iDstStart);

            if (this->IsDetachedBuffer() || fromArray->IsDetachedBuffer())
            {
                JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
            }

            // Fixup destination start in case it's negative
            uint32 start = iDstStart;
            if (iDstStart < 0)
            {
                if ((int64)(length) + iDstStart < 0)
                {
                    // nothing to do, all index are no-op
                    return true;
                }

                length += iDstStart;
                start = 0;
            }

            uint32 dstLength = UInt32Math::Add(start, length) < GetLength() ? length : GetLength() > start ? GetLength() - start : 0;
            uint32 srcLength = start + length < fromArray->GetLength() ? length : (fromArray->GetLength() > start ? fromArray->GetLength() - start : 0);

            // length is the minimum of length, srcLength and dstLength
            length = length < srcLength ? (length < dstLength ? length : dstLength) : (srcLength < dstLength ? srcLength : dstLength);

            const size_t byteSize = sizeof(TypeName) * length;
            Assert(byteSize >= length); // check for overflow
            js_memcpy_s(dstBuffer + start, dstLength * sizeof(TypeName), srcBuffer + start, byteSize);

            if (dstLength > length)
            {
                TypeName undefinedValue = convFunc(GetLibrary()->GetUndefined(), GetScriptContext());
                for (uint32 i = length; i < dstLength; i++)
                {
                    dstBuffer[i] = undefinedValue;
                }
            }

            return true;
        }

        inline BOOL DirectSetItemAtRange(__in int32 start, __in uint32 length, __in Js::Var value, TypeName(*convFunc)(Var value, ScriptContext* scriptContext))
        {
            if (CrossSite::IsCrossSiteObjectTyped(this))
            {
                return false;
            }
            TypeName typedValue = convFunc(value, GetScriptContext());

            if (this->IsDetachedBuffer()) // 9.4.5.9 IntegerIndexedElementSet
            {
                JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
            }
            uint32 newStart = start, newLength = length;

            if (start < 0)
            {
                if ((int64)(length) + start < 0)
                {
                    // nothing to do, all index are no-op
                    return true;
                }
                newStart = 0;
                // fixup the length with the change
                newLength += start;
            }
            if (newStart >= GetLength())
            {
                // If we want to start copying past the length of the array, all index are no-op
                return true;
            }
            if (UInt32Math::Add(newStart, newLength) > GetLength())
            {
                newLength = GetLength() - newStart;
            }

            TypeName* typedBuffer = (TypeName*)buffer;

            if (typedValue == 0 || sizeof(TypeName) == 1)
            {
                const size_t byteSize = sizeof(TypeName) * newLength;
                Assert(byteSize >= newLength); // check for overflow
                memset(typedBuffer + newStart, (int)typedValue, byteSize);
            }
            else
            {
                for (uint32 i = 0; i < newLength; i++)
                {
                    typedBuffer[newStart + i] = typedValue;
                }
            }

            return TRUE;
        }

        inline BOOL BaseTypedDirectSetItem(__in uint32 index, __in Js::Var value, TypeName (*convFunc)(Var value, ScriptContext* scriptContext))
        {
            // This call can potentially invoke user code, and may end up detaching the underlying array (this).
            // Therefore it was brought out and above the IsDetached check
            TypeName typedValue = convFunc(value, GetScriptContext());

            if (this->IsDetachedBuffer()) // 9.4.5.9 IntegerIndexedElementSet
            {
                JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
            }

            if (index >= GetLength())
            {
                return FALSE;
            }

            AssertMsg(index < GetLength(), "Trying to set out of bound index for typed array.");
            Assert((index + 1)* sizeof(TypeName) + GetByteOffset() <= GetArrayBuffer()->GetByteLength());
            TypeName* typedBuffer = (TypeName*)buffer;

            typedBuffer[index] = typedValue;

            return TRUE;
        }

        inline BOOL BaseTypedDirectSetItemNoSet(__in uint32 index, __in Js::Var value, TypeName (*convFunc)(Var value, ScriptContext* scriptContext))
        {
            // This call can potentially invoke user code, and may end up detaching the underlying array (this).
            // Therefore it was brought out and above the IsDetached check
            convFunc(value, GetScriptContext());

            if (this->IsDetachedBuffer()) // 9.4.5.9 IntegerIndexedElementSet
            {
                JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_DetachedTypedArray);
            }

            return FALSE;
        }

        inline BOOL BaseTypedDirectSetItemNoDetachCheck(__in uint32 index, __in Js::Var value, TypeName(*convFunc)(Var value, ScriptContext* scriptContext))
        {
            TypeName typedValue = convFunc(value, GetScriptContext());

            // The caller of the function made sure that no IsDetached check required.
            // The caller of the function also made sure that no length check required.

            Assert(!IsDetachedBuffer());
            AssertMsg(index < GetLength(), "Trying to set out of bound index for typed array.");
            Assert((index + 1)* sizeof(TypeName) + GetByteOffset() <= GetArrayBuffer()->GetByteLength());
            TypeName* typedBuffer = (TypeName*)buffer;

            typedBuffer[index] = typedValue;

            return TRUE;
        }


        virtual BOOL DirectSetItem(__in uint32 index, __in Js::Var value) override sealed;
        virtual BOOL DirectSetItemNoSet(__in uint32 index, __in Js::Var value) override sealed;
        virtual Var  DirectGetItem(__in uint32 index) override sealed;
        virtual BOOL DirectSetItemNoDetachCheck(__in uint32 index, __in Js::Var value) override sealed;
        virtual Var  DirectGetItemNoDetachCheck(__in uint32 index) override sealed;
        virtual Var TypedAdd(__in uint32 index, Var second) override;
        virtual Var TypedAnd(__in uint32 index, Var second) override;
        virtual Var TypedLoad(__in uint32 index) override;
        virtual Var TypedOr(__in uint32 index, Var second) override;
        virtual Var TypedStore(__in uint32 index, Var second) override;
        virtual Var TypedSub(__in uint32 index, Var second) override;
        virtual Var TypedXor(__in uint32 index, Var second) override;
        virtual Var TypedExchange(__in uint32 index, Var second) override;
        virtual Var TypedCompareExchange(__in uint32 index, Var comparand, Var replacementValue) override;

        static BOOL DirectSetItem(__in TypedArray* arr, __in uint32 index, __in Js::Var value)
        {
            AssertMsg(arr != nullptr, "Array shouldn't be nullptr.");

            return arr->DirectSetItem(index, value);
        }

    protected:
        CompareElementsFunction GetCompareElementsFunction()
        {
            return &TypedArrayCompareElementsHelper<TypeName>;
        }

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF();
    };

    // in windows build environment, char16 is not an intrinsic type, and we cannot do the type
    // specialization
    class CharArray : public TypedArrayBase
    {
    protected:
        DEFINE_VTABLE_CTOR(CharArray, TypedArrayBase);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(CharArray);

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Set;
            static FunctionInfo Subarray;
        };

        static Var EntrySet(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySubarray(RecyclableObject* function, CallInfo callInfo, ...);

        CharArray(ArrayBufferBase* arrayBuffer, uint32 byteOffset, uint32 mappedLength, DynamicType* type) :
        TypedArrayBase(arrayBuffer, byteOffset, mappedLength, sizeof(char16), type)
        {
            AssertMsg(arrayBuffer->GetByteLength() >= byteOffset, "invalid offset");
            AssertMsg(mappedLength*sizeof(char16)+byteOffset <= GetArrayBuffer()->GetByteLength(), "invalid length");
            buffer = arrayBuffer->GetBuffer() + byteOffset;
        }

        static Var Create(ArrayBufferBase* arrayBuffer, uint32 byteOffSet, uint32 mappedLength, JavascriptLibrary* javascriptLibrary);
        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static BOOL Is(Var aValue);

        Var Subarray(uint32 begin, uint32 end);
        static CharArray* FromVar(Var aValue);

        virtual BOOL DirectSetItem(__in uint32 index, __in Js::Var value) override;
        virtual BOOL DirectSetItemNoSet(__in uint32 index, __in Js::Var value) override;
        virtual Var  DirectGetItem(__in uint32 index) override;
        virtual BOOL DirectSetItemNoDetachCheck(__in uint32 index, __in Js::Var value) override;
        virtual Var  DirectGetItemNoDetachCheck(__in uint32 index) override;

        virtual Var TypedAdd(__in uint32 index, Var second) override;
        virtual Var TypedAnd(__in uint32 index, Var second) override;
        virtual Var TypedLoad(__in uint32 index) override;
        virtual Var TypedOr(__in uint32 index, Var second) override;
        virtual Var TypedStore(__in uint32 index, Var second) override;
        virtual Var TypedSub(__in uint32 index, Var second) override;
        virtual Var TypedXor(__in uint32 index, Var second) override;
        virtual Var TypedExchange(__in uint32 index, Var second) override;
        virtual Var TypedCompareExchange(__in uint32 index, Var comparand, Var replacementValue) override;

    protected:
        CompareElementsFunction GetCompareElementsFunction()
        {
            return &TypedArrayCompareElementsHelper<char16>;
        }

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableCharArray;
        }
    };


    template <typename TypeName, bool clamped, bool virtualAllocated>
    TypedArray<TypeName, clamped, virtualAllocated>::TypedArray(ArrayBufferBase* arrayBuffer, uint32 byteOffset, uint32 mappedLength, DynamicType* type) :
        TypedArrayBase(arrayBuffer, byteOffset, mappedLength, sizeof(TypeName), type)
    {
        AssertMsg(arrayBuffer->GetByteLength() >= byteOffset, "invalid offset");
        AssertMsg(mappedLength*sizeof(TypeName)+byteOffset <= arrayBuffer->GetByteLength(), "invalid length");
        buffer = arrayBuffer->GetBuffer() + byteOffset;
        if (arrayBuffer->IsValidVirtualBufferLength(arrayBuffer->GetByteLength()) &&
             (byteOffset == 0) &&
             (mappedLength == (arrayBuffer->GetByteLength() / sizeof(TypeName)))
           )
        {
            // update the vtable
            switch (type->GetTypeId())
            {
            case TypeIds_Int8Array:
                VirtualTableInfo<Int8VirtualArray>::SetVirtualTable(this);
                break;
            case TypeIds_Uint8Array:
                VirtualTableInfo<Uint8VirtualArray>::SetVirtualTable(this);
                break;
            case TypeIds_Uint8ClampedArray:
                VirtualTableInfo<Uint8ClampedVirtualArray>::SetVirtualTable(this);
                break;
            case TypeIds_Int16Array:
                VirtualTableInfo<Int16VirtualArray>::SetVirtualTable(this);
                break;
            case TypeIds_Uint16Array:
                VirtualTableInfo<Uint16VirtualArray>::SetVirtualTable(this);
                break;
            case TypeIds_Int32Array:
                VirtualTableInfo<Int32VirtualArray>::SetVirtualTable(this);
                break;
            case TypeIds_Uint32Array:
                VirtualTableInfo<Uint32VirtualArray>::SetVirtualTable(this);
                break;
            case TypeIds_Float32Array:
                VirtualTableInfo<Float32VirtualArray>::SetVirtualTable(this);
                break;
            case TypeIds_Float64Array:
                VirtualTableInfo<Float64VirtualArray>::SetVirtualTable(this);
                break;
            default:
                break;
            }
        }
    }

    template <typename TypeName, bool clamped, bool virtualAllocated>
    Var TypedArray<TypeName, clamped, virtualAllocated>::Create(ArrayBufferBase* arrayBuffer, uint32 byteOffSet, uint32 mappedLength, JavascriptLibrary* javascriptLibrary)
    {
        uint32 totalLength, mappedByteLength;

        if (UInt32Math::Mul(mappedLength, sizeof(TypeName), &mappedByteLength) ||
            UInt32Math::Add(byteOffSet, mappedByteLength, &totalLength) ||
            (totalLength > arrayBuffer->GetByteLength()))
        {
            JavascriptError::ThrowRangeError(arrayBuffer->GetScriptContext(), JSERR_InvalidTypedArrayLength);
        }

        DynamicType *type = javascriptLibrary->GetTypedArrayType<TypeName, clamped>(0);
        return RecyclerNew(javascriptLibrary->GetRecycler(), TypedArray, arrayBuffer, byteOffSet, mappedLength, type);
    }

#if defined(__clang__)
// hack for clang message: "...add an explicit instantiation declaration to .."
#define __EXPLICIT_INSTANTINATE_TA(x) x;\
            template<> FunctionInfo Js::x::EntryInfo::NewInstance;\
            template<> FunctionInfo Js::x::EntryInfo::Set
#else // MSVC
#define __EXPLICIT_INSTANTINATE_TA(x) x
#endif

    typedef TypedArray<int8>        __EXPLICIT_INSTANTINATE_TA(Int8Array);
    typedef TypedArray<uint8,false> __EXPLICIT_INSTANTINATE_TA(Uint8Array);
    typedef TypedArray<uint8,true>  __EXPLICIT_INSTANTINATE_TA(Uint8ClampedArray);
    typedef TypedArray<int16>       __EXPLICIT_INSTANTINATE_TA(Int16Array);
    typedef TypedArray<uint16>      __EXPLICIT_INSTANTINATE_TA(Uint16Array);
    typedef TypedArray<int32>       __EXPLICIT_INSTANTINATE_TA(Int32Array);
    typedef TypedArray<uint32>      __EXPLICIT_INSTANTINATE_TA(Uint32Array);
    typedef TypedArray<float>       __EXPLICIT_INSTANTINATE_TA(Float32Array);
    typedef TypedArray<double>      __EXPLICIT_INSTANTINATE_TA(Float64Array);
    typedef TypedArray<int64>       __EXPLICIT_INSTANTINATE_TA(Int64Array);
    typedef TypedArray<uint64>      __EXPLICIT_INSTANTINATE_TA(Uint64Array);
    typedef TypedArray<bool>        __EXPLICIT_INSTANTINATE_TA(BoolArray);
    typedef TypedArray<int8, false, true>   __EXPLICIT_INSTANTINATE_TA(Int8VirtualArray);
    typedef TypedArray<uint8, false, true>  __EXPLICIT_INSTANTINATE_TA(Uint8VirtualArray);
    typedef TypedArray<uint8, true, true>   __EXPLICIT_INSTANTINATE_TA(Uint8ClampedVirtualArray);
    typedef TypedArray<int16, false, true>  __EXPLICIT_INSTANTINATE_TA(Int16VirtualArray);
    typedef TypedArray<uint16, false, true> __EXPLICIT_INSTANTINATE_TA(Uint16VirtualArray);
    typedef TypedArray<int32, false, true>  __EXPLICIT_INSTANTINATE_TA(Int32VirtualArray);
    typedef TypedArray<uint32, false, true> __EXPLICIT_INSTANTINATE_TA(Uint32VirtualArray);
    typedef TypedArray<float, false, true>  __EXPLICIT_INSTANTINATE_TA(Float32VirtualArray);
    typedef TypedArray<double, false, true> __EXPLICIT_INSTANTINATE_TA(Float64VirtualArray);

#undef __EXPLICIT_INSTANTINATE_TA
}

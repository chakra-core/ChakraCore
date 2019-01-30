//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace Js
{
    //
    // SerializationCloner helps clone a Var to a stream location.
    //
    template <class Writer>
    class SerializationCloner:
        public ClonerBase<Var, scaposition_t, TypeId, SerializationCloner<Writer> >
    {
    public:
        using typename ClonerBase<Var, scaposition_t, TypeId, SerializationCloner<Writer> >::Dst;
        using typename ClonerBase<Var, scaposition_t, TypeId, SerializationCloner<Writer> >::Src;
        using typename ClonerBase<Var, scaposition_t, TypeId, SerializationCloner<Writer> >::SrcTypeId;

    private:
        //AutoCOMPtr<ISCAContext> m_pSCAContext;
        Writer* m_writer;
        // AutoCOMPtr<ISCASerializable> m_pSCASerializable; // Temporary, QI from a host object
        JsUtil::List<SharedContents*, HeapAllocator>* m_sharedContentsrList;

    private:
        void WriteTypeId(SCATypeId typeId) const
        {
            Write(static_cast<uint32>(typeId));
        }

        void Write(uint32 data) const
        {
            m_writer->Write(data);
        }

        void Write(const char16* str, charcount_t len) const;
        void Write(const BYTE* bytes, uint32 len) const;

        //
        // Write a TypedArray or a DataView layout: [SCATypeId] [ArrayBuffer] [byteOffset] [length]
        //
        template <class T, bool clamped, bool isVirtual = false>
        void WriteTypedArray(Src src) const
        {
            typedef TypedArrayTrace<T, clamped, isVirtual> trace_type;
            typedef typename trace_type::TypedArrayType array_type;

            WriteTypeId(trace_type::GetSCATypeId());
            array_type* arr = VarTo<array_type>(src);

            this->GetEngine()->Clone(arr->GetArrayBuffer());

            Write(arr->GetByteOffset());
            Write(arr->GetLength());
        }

        static bool IsTypedArray(SrcTypeId typeId);
        void WriteTypedArray(SrcTypeId typeId, Src src) const;

        static bool IsSparseArray(JavascriptArray* arr);
        void WriteDenseArrayIndexProperties(JavascriptArray* arr);
        void WriteSparseArrayIndexProperties(JavascriptArray* arr);

        //
        // Write array index named properties.
        //
        template <class ArrayItemAccessor>
        void WriteArrayIndexProperties(JavascriptArray* arr)
        {
            ScriptContext* scriptContext = this->GetScriptContext();
            uint32 length = arr->GetLength();

            for (uint32 i = 0; i < length; i++)
            {
                Var value = nullptr;
                if (ArrayItemAccessor::GetItem(arr, i, &value, scriptContext))
                {
                    this->GetEngine()->Clone(value);
                }
                else
                {
                    WriteTypeId(SCA_None);
                }
            }
        }

        //
        // Write sparse array index named properties using an array index enumerator.
        //
        template <class IndexEnumerator, class ArrayItemAccessor>
        void WriteSparseArrayIndexProperties(typename IndexEnumerator::ArrayType* arr)
        {
            ScriptContext* scriptContext = this->GetScriptContext();
            IndexEnumerator e(arr);
            Var value = nullptr;

            while (e.MoveNext())
            {
                uint32 i = e.GetIndex();
                if (ArrayItemAccessor::GetItem(arr, i, &value, scriptContext))
                {
                    Write(i);
                    this->GetEngine()->Clone(value);
                }
            }

            Write(static_cast<uint32>(SCA_PROPERTY_TERMINATOR));
        }

        //
        // Write a SCAProperties section: {SCAPropertyName SCAValue} SCAPropertiesTerminator
        //
        template <class Enumerator>
        void WriteObjectProperties(Enumerator* enumerator)
        {
            while (enumerator->MoveNext())
            {
                // Write property name
                Write(enumerator->GetNameString(), enumerator->GetNameLength());

                // Write property value
                this->GetEngine()->Clone(enumerator->GetValue());
            }

            // Write property terminator
            m_writer->Write(static_cast<uint32>(SCA_PROPERTY_TERMINATOR));
        }

        // Direct item accessor used on non-crosssite JavascriptArray for WriteArrayIndexProperties.
        class JavascriptArrayDirectItemAccessor
        {
        public:
            static BOOL GetItem(JavascriptArray* arr, uint32 index, Var* value, ScriptContext* scriptContext)
            {
                return arr->GetItem(arr, index, value, scriptContext);
            }
        };

        // Item accessor used on cross-site JavascriptArray for WriteArrayIndexProperties.
        class JavascriptArrayItemAccessor
        {
        public:
            static BOOL GetItem(JavascriptArray* arr, uint32 index, Var* value, ScriptContext* scriptContext)
            {
                return JavascriptOperators::GetOwnItem(arr, index, value, scriptContext);
            }
        };

        // Item accessor used on ES5Array for WriteArrayIndexProperties.
        class JavascriptArrayEnumerableItemAccessor
        {
        public:
            static BOOL GetItem(JavascriptArray* arr, uint32 index, Var* value, ScriptContext* scriptContext)
            {
                return arr->IsItemEnumerable(index)
                    && JavascriptOperators::GetOwnItem(arr, index, value, scriptContext);
            }
        };

    public:
        SerializationCloner(ScriptContext* scriptContext, Writer* writer, JsUtil::List<SharedContents*, HeapAllocator>* sharedContentsrList)
            : ClonerBase<Var, scaposition_t, TypeId, SerializationCloner<Writer> >(scriptContext), m_writer(writer), m_sharedContentsrList(sharedContentsrList)
        {
        }

        static bool ShouldLookupReference()
        {
            // Always lookup reference when cloning Vars
            return true;
        }

        SrcTypeId GetTypeId(Src src) const
        {
            return JavascriptOperators::GetTypeId(src);
        }

        bool TryClonePrimitive(SrcTypeId typeId, Src src, Dst* dst);
        bool TryCloneObject(SrcTypeId typeId, Src src, Dst* dst, SCADeepCloneType* deepClone);
        void CloneProperties(SrcTypeId typeId, Src src, Dst dst);
        void CloneHostObjectProperties(SrcTypeId typeId, Src src, Dst dst);
        void CloneMap(Src src, Dst dst);
        void CloneSet(Src src, Dst dst);
        void CloneObjectReference(Src src, Dst dst);
        bool CanBeTransferred(SrcTypeId typeId) { return typeId == TypeIds_ArrayBuffer; }
    };

    //
    // Object property enumerator for WriteObjectProperties.
    //
    class ObjectPropertyEnumerator: public ScriptContextHolder
    {
    private:
        RecyclableObject* m_obj;
        JavascriptStaticEnumerator* m_innerEnumerator;
        JavascriptString* m_name;
        Var m_value;

    public:
        ObjectPropertyEnumerator(ScriptContext* scriptContext, RecyclableObject* obj, JavascriptStaticEnumerator* enumerator)
            : ScriptContextHolder(scriptContext),
            m_obj(obj),
            m_innerEnumerator(enumerator),
            m_name(NULL),
            m_value(NULL)
        {
        }

        bool MoveNext();

        const char16* GetNameString() const
        {
            return m_name->GetString();
        }

        charcount_t GetNameLength() const
        {
            return m_name->GetLength();
        }

        Var GetValue() const
        {
            return m_value;
        }
    };

    //
    // Template and specializations for TypedArray/DataView SCA.
    //
    template <class T, bool clamped = false, bool isVirtual = false> struct TypedArrayTrace
    {
        // Generic TypedArrayTrace is empty. Specializations provide implementation.
    };

    // Base implementation shared by data type TypedArrays.
    template <class T, bool clamped = false, bool isVirtual = false> struct TypedArrayTraceBase
    {
        typedef TypedArray<T,clamped, isVirtual> TypedArrayType;
        static Var CreateTypedArray(ArrayBufferBase* arrayBuffer, uint32 byteOffset, uint32 length,
            ScriptContext* scriptContext)
        {
            JavascriptLibrary* lib = scriptContext->GetLibrary();
            return TypedArrayType::Create(arrayBuffer, byteOffset, length, lib);
        }
    };
    template<> struct TypedArrayTrace<int8>: TypedArrayTraceBase<int8>
    {
        static SCATypeId GetSCATypeId() { return SCA_Int8Array; }
    };
    template<> struct TypedArrayTrace<uint8, false>: TypedArrayTraceBase<uint8, false>
    {
        static SCATypeId GetSCATypeId() { return SCA_Uint8Array; }
    };
    template<> struct TypedArrayTrace<uint8, true>: TypedArrayTraceBase<uint8, true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Uint8ClampedArray; }
    };
    template<> struct TypedArrayTrace<int16>: TypedArrayTraceBase<int16>
    {
        static SCATypeId GetSCATypeId() { return SCA_Int16Array; }
    };
    template<> struct TypedArrayTrace<uint16>: TypedArrayTraceBase<uint16>
    {
        static SCATypeId GetSCATypeId() { return SCA_Uint16Array; }
    };
    template<> struct TypedArrayTrace<int32>: TypedArrayTraceBase<int32>
    {
        static SCATypeId GetSCATypeId() { return SCA_Int32Array; }
    };
    template<> struct TypedArrayTrace<uint32>: TypedArrayTraceBase<uint32>
    {
        static SCATypeId GetSCATypeId() { return SCA_Uint32Array; }
    };
    template<> struct TypedArrayTrace<float>: TypedArrayTraceBase<float>
    {
        static SCATypeId GetSCATypeId() { return SCA_Float32Array; }
    };
    template<> struct TypedArrayTrace<double>: TypedArrayTraceBase<double>
    {
        static SCATypeId GetSCATypeId() { return SCA_Float64Array; }
    };
    template<> struct TypedArrayTrace<int8, false,true>: TypedArrayTraceBase<int8,false,true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Int8Array; }
    };
    template<> struct TypedArrayTrace<uint8, false,true>: TypedArrayTraceBase<uint8, false,true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Uint8Array; }
    };
    template<> struct TypedArrayTrace<uint8, true,true>: TypedArrayTraceBase<uint8, true,true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Uint8ClampedArray; }
    };
    template<> struct TypedArrayTrace<int16,false,true>: TypedArrayTraceBase<int16,false,true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Int16Array; }
    };
    template<> struct TypedArrayTrace<uint16,false,true>: TypedArrayTraceBase<uint16,false,true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Uint16Array; }
    };
    template<> struct TypedArrayTrace<int32,false,true>: TypedArrayTraceBase<int32,false,true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Int32Array; }
    };
    template<> struct TypedArrayTrace<uint32,false,true>: TypedArrayTraceBase<uint32,false,true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Uint32Array; }
    };
    template<> struct TypedArrayTrace<float, false, true>: TypedArrayTraceBase<float, false, true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Float32Array; }
    };
    template<> struct TypedArrayTrace<double, false, true>: TypedArrayTraceBase<double, false, true>
    {
        static SCATypeId GetSCATypeId() { return SCA_Float64Array; }
    };
    template<> struct TypedArrayTrace<DataView>
    {
        typedef DataView TypedArrayType;
        static Var CreateTypedArray(ArrayBufferBase* arrayBuffer, uint32 byteOffset, uint32 length,
            ScriptContext* scriptContext)
        {
            JavascriptLibrary* lib = scriptContext->GetLibrary();
            return lib->CreateDataView(arrayBuffer, byteOffset, length);
        }
        static SCATypeId GetSCATypeId() { return SCA_DataView; }
    };

    class SCASerializationEngine
    {
        typedef SerializationCloner<StreamWriter> StreamSerializationCloner;

    public:
        static void Serialize(Var root, StreamWriter* writer, Var* transferableVars, size_t cTransferableVars,
            JsUtil::List<Js::SharedContents*, HeapAllocator>* sharedContentsList);
    };
}

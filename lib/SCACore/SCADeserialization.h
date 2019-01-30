//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace Js
{
    class SCAPropBag;

    //
    // DeserializationCloner helps clone a Var from a stream location.
    //
    template <class Reader>
    class DeserializationCloner:
        public ClonerBase<scaposition_t, Var, SCATypeId, DeserializationCloner<Reader> >
    {
    public:
        using typename ClonerBase<scaposition_t, Var, SCATypeId, DeserializationCloner<Reader> >::Dst;
        using typename ClonerBase<scaposition_t, Var, SCATypeId, DeserializationCloner<Reader> >::Src;
        using typename ClonerBase<scaposition_t, Var, SCATypeId, DeserializationCloner<Reader> >::SrcTypeId;
    private:
        //AutoCOMPtr<ISCAHost> m_pSCAHost;
        //AutoCOMPtr<ISCAContext> m_pSCAContext;
        Reader* m_reader;
        mutable char16* m_buffer;
        mutable charcount_t m_bufferLength;


    private:
        SCATypeId ReadTypeId() const
        {
            uint32 typeId;
            m_reader->Read(&typeId);
            return static_cast<SCATypeId>(typeId);
        }

        void Read(uint32* value) const
        {
            m_reader->Read(value);
        }

        const char16* TryReadString(charcount_t* len, bool reuseBuffer) const;
        const char16* ReadString(charcount_t* len) const;
        void Read(BYTE* buf, uint32 len) const;

        //
        // Read a TypedArray or DataView.
        //
        template <class T, bool clamped>
        void ReadTypedArray(Dst* dst) const
        {
            typedef TypedArrayTrace<T,clamped> trace_type;

            Dst arrayBuffer;
            this->GetEngine()->Clone(m_reader->GetPosition(), &arrayBuffer);
            if (!arrayBuffer || !VarIs<ArrayBufferBase>(arrayBuffer))
            {
                this->ThrowSCADataCorrupt();
            }

            uint32 byteOffset, length;
            Read(&byteOffset);
            Read(&length);
            *dst = trace_type::CreateTypedArray(
                VarTo<ArrayBufferBase>(arrayBuffer), byteOffset, length, this->GetScriptContext());
        }

        void ReadTypedArray(SrcTypeId typeId, Dst* dst) const;

        //
        // Read a SCAProperties section: {SCAPropertyName SCAValue} SCAPropertiesTerminator
        //
        void ReadObjectPropertiesIntoObject(RecyclableObject* m_obj)
        {
            ScriptContext* scriptContext = this->GetScriptContext();

            for(;;)
            {
                charcount_t len = 0;
                const char16* name = TryReadString(&len, /*reuseBuffer*/ true);
                if (!name)
                {
                    break;
                }

                Js::PropertyRecord const * propertyRecord;
                scriptContext->GetOrAddPropertyRecord(name, len, &propertyRecord);

                // NOTE: 'Clone' may reenter here and use the buffer that backs the 'name'.
                //       That is ok, since we do not need it past this point.
                //       The propertyRecord keeps its own copy od the data.

                Var value;
                this->GetEngine()->Clone(m_reader->GetPosition(), &value);
                if (!value)
                {
                    this->ThrowSCADataCorrupt();
                }

                m_obj->SetProperty(propertyRecord->GetPropertyId(), value, PropertyOperation_None, NULL); //Note: no prototype check
            }
        }

        void ReadObjectPropertiesIntoBag(SCAPropBag* m_propbag)
        {
            for (;;)
            {
                charcount_t len = 0;
                // NOTE: we will not reuse buffer here since the propbag may retain the string.
                const char16* name = TryReadString(&len, /*reuseBuffer*/ false);
                if (!name)
                {
                    break;
                }

                Var value;
                this->GetEngine()->Clone(m_reader->GetPosition(), &value);
                if (!value)
                {
                    this->ThrowSCADataCorrupt();
                }

                HRESULT hr = m_propbag->InternalAddNoCopy(name, len, value);
                m_propbag->ThrowIfFailed(hr);
            }
        }

    public:
        DeserializationCloner(ScriptContext* scriptContext, Reader* reader)
            : ClonerBase<scaposition_t, Var, SCATypeId, DeserializationCloner<Reader> >(scriptContext), m_reader(reader), 
                         m_buffer(nullptr), m_bufferLength(0)
        {
        }

        static bool ShouldLookupReference()
        {
            // Never lookup reference when cloning from stream location. DeserializationCloner
            // handles object reference lookup explictly when seeing a reference SCATypeId.
            return false;
        }

        SrcTypeId GetTypeId(Src src) const
        {
            Assert(m_reader->GetPosition() == src); // Only allow the current position
            return ReadTypeId();
        }

        void ThrowSCAUnsupported() const
        {
            // Unexpected SCATypeId indicates data corruption.
            this->ThrowSCADataCorrupt();
        }

        bool TryClonePrimitive(SrcTypeId typeId, Src src, Dst* dst);
        bool TryCloneObject(SrcTypeId typeId, Src src, Dst* dst, SCADeepCloneType* deepClone);
        void CloneProperties(SrcTypeId typeId, Src src, Dst dst);
        void CloneHostObjectProperties(SrcTypeId typeId, Src src, Dst dst);
        void CloneMap(Src src, Dst dst);
        void CloneSet(Src src, Dst dst);
        void CloneObjectReference(Src src, Dst dst);
    };

    class SCADeserializationEngine
    {
        typedef DeserializationCloner<StreamReader> StreamDeserializationCloner;

    public:
        static Var Deserialize(StreamReader* reader, Var* transferableVars, size_t cTransferableVars);
    };
}

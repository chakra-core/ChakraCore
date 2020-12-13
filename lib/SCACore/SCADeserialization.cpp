//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "SCACorePch.h"

namespace Js
{
    template <class Reader>
    bool DeserializationCloner<Reader>::TryClonePrimitive(SrcTypeId typeId, Src src, Dst* dst)
    {
        if (!IsSCAPrimitive(typeId))
        {
            return false;
        }

        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptLibrary* lib = scriptContext->GetLibrary();

        switch (typeId)
        {
        case SCA_None:
            *dst = NULL;
            break;

        case SCA_Reference: // Handle reference explictly as a primitive
            {
                scaposition_t pos;
                m_reader->Read(&pos);
                if (!this->GetEngine()->TryGetClonedObject(pos, dst))
                {
                    this->ThrowSCADataCorrupt();
                }
            }
            break;

        case SCA_NullValue:
            *dst = lib->GetNull();
            break;

        case SCA_UndefinedValue:
            *dst = lib->GetUndefined();
            break;

        case SCA_TrueValue:
            *dst = lib->GetTrue();
            break;

        case SCA_FalseValue:
            *dst = lib->GetFalse();
            break;

        case SCA_Int32Value:
            {
                int32 n;
                m_reader->Read(&n);
                *dst = JavascriptNumber::ToVar(n, scriptContext);
            }
            break;

        case SCA_DoubleValue:
            {
                double dbl;
                m_reader->Read(&dbl);
                *dst = JavascriptNumber::ToVarWithCheck(dbl, scriptContext);
            }
            break;

        case SCA_Int64Value:
            {
                __int64 n;
                m_reader->Read(&n);
                *dst = JavascriptInt64Number::ToVar(n, scriptContext);
            }
            break;

        case SCA_Uint64Value:
            {
                unsigned __int64 n;
                m_reader->Read(&n);
                *dst = JavascriptUInt64Number::ToVar(n, scriptContext);
            }
            break;

        default:
            return false; // Not a recognized primitive type
        }

        return true;
    }

    template <class Reader>
    bool DeserializationCloner<Reader>::TryCloneObject(SrcTypeId typeId, Src src, Dst* dst, SCADeepCloneType* deepClone)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptLibrary* lib = scriptContext->GetLibrary();
        *deepClone = SCADeepCloneType::None;
        bool isObject = true;

        if (typeId == SCA_Transferable)
        {
            scaposition_t pos;
            m_reader->Read(&pos);

            *dst = this->GetEngine()->ClaimTransferable(pos, lib);
            if (*dst == nullptr)
            {
                this->ThrowSCADataCorrupt();
            }

            return true;
        }

        if (IsSCAHostObject(typeId))
        {
            *dst = m_reader->ReadHostObject();
            *deepClone = SCADeepCloneType::HostObject;
            return true;
        }

        switch (typeId)
        {
        case SCA_StringValue: // Clone string value as object type to resolve multiple references
            {               
                charcount_t len;
                const char16* buf = ReadString(&len);
                *dst = Js::JavascriptString::NewWithBuffer(buf, len, scriptContext);
                isObject = false;
            }
            break;

        case SCA_BooleanTrueObject:
            *dst = lib->CreateBooleanObject(TRUE);
            break;

        case SCA_BooleanFalseObject:
            *dst = lib->CreateBooleanObject(FALSE);
            break;

        case SCA_DateObject:
            {
                double dbl;
                m_reader->Read(&dbl);
                *dst = lib->CreateDate(dbl);
            }
            break;

        case SCA_NumberObject:
            {
                double dbl;
                m_reader->Read(&dbl);
                *dst = lib->CreateNumberObjectWithCheck(dbl);
            }
            break;

        case SCA_StringObject:
            {
                charcount_t len;
                const char16* buf = ReadString(&len);
                *dst = lib->CreateStringObject(buf, len);
            }
            break;

        case SCA_RegExpObject:
            {
                charcount_t len;
                const char16* buf = ReadString(&len);

                DWORD flags;
                m_reader->Read(&flags);
                *dst = JavascriptRegExp::CreateRegEx(buf, len,
                    static_cast<UnifiedRegex::RegexFlags>(flags), scriptContext);
            }
            break;

        case SCA_Object:
            {
                *dst = lib->CreateObject();
                *deepClone = SCADeepCloneType::Object;
            }
            break;

        case SCA_Map:
            {
                *dst = JavascriptMap::New(scriptContext);
                *deepClone = SCADeepCloneType::Map;
            }
            break;

        case SCA_Set:
            {
                *dst = JavascriptSet::New(scriptContext);
                *deepClone = SCADeepCloneType::Set;
            }
            break;

        case SCA_DenseArray:
        case SCA_SparseArray:
            {
                uint32 length;
                Read(&length);
                *dst = lib->CreateArray(length);
                *deepClone = SCADeepCloneType::Object;
            }
            break;

        case SCA_ArrayBuffer:
            {
                uint32 len;
                m_reader->Read(&len);
                ArrayBuffer* arrayBuffer = lib->CreateArrayBuffer(len);
                Read(arrayBuffer->GetBuffer(), arrayBuffer->GetByteLength());
                *dst = arrayBuffer;
            }
            break;

        case SCA_SharedArrayBuffer:
            {
                SharedContents * sharedContents;
                m_reader->Read((intptr_t*)&sharedContents);

                SharedArrayBuffer* arrayBuffer = lib->CreateSharedArrayBuffer(sharedContents);
                Assert(arrayBuffer->IsWebAssemblyArrayBuffer() == sharedContents->IsWebAssembly());
                *dst = arrayBuffer;
            }
            break;

//#ifdef ENABLE_WASM
//        case SCA_WebAssemblyModule:
//        {
//            uint32 len;
//            m_reader->Read(&len);
//            byte* buffer = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), byte, len);
//            Read(buffer, len);
//            WebAssemblySource wasmSrc(buffer, len, true, scriptContext);
//            *dst = WebAssemblyModule::CreateModule(scriptContext, &wasmSrc);
//            break;
//        }
//        case SCA_WebAssemblyMemory:
//        {
//            uint32 initialLength = 0;
//            uint32 maximumLength = 0;
//            uint32 isShared = 0;
//            m_reader->Read(&initialLength);
//            m_reader->Read(&maximumLength);
//
//#ifdef ENABLE_WASM_THREADS
//            m_reader->Read(&isShared);
//            if (isShared)
//            {
//                SharedContents * sharedContents;
//                m_reader->Read((intptr_t*)&sharedContents);
//                *dst = WebAssemblyMemory::CreateFromSharedContents(initialLength, maximumLength, sharedContents, scriptContext);
//            }
//            else
//#endif
//            {
//                uint32 len;
//                m_reader->Read(&len);
//                WebAssemblyMemory* mem = WebAssemblyMemory::CreateForExistingBuffer(initialLength, maximumLength, len, scriptContext);
//                Read(mem->GetBuffer()->GetBuffer(), len);
//                *dst = mem;
//            }
//            break;
//        }
//#endif

        case SCA_Uint8ClampedArray:
            // If Khronos Interop is not enabled, we don't have Uint8ClampedArray available.
            // This is a scenario where the source buffer was created in a newer document mode 
            // but needs to be deserialized in an older document mode. 
            // What we want to do is return the buffer as a CanvasPixelArray instead of 
            // Uint8ClampedArray since the older document mode knows what CanvasPixelArray is but
            // not what Uint8ClampedArray is.
            // We don't support pixelarray in edge anymore.
            // Intentionally fall through to default (TypedArray) label

        default:
            if (IsSCATypedArray(typeId) || typeId == SCA_DataView)
            {
                ReadTypedArray(typeId, dst);
                break;
            }
            return false; // Not a supported object type
        }

#ifdef ENABLE_JS_ETW
        if (EventEnabledJSCRIPT_RECYCLER_ALLOCATE_OBJECT() && isObject)
        {
            EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*dst);
        }
#endif
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            *dst = JavascriptProxy::AutoProxyWrapper(*dst);
        }
#endif
        return true;
    }

    template <class Reader>
    void DeserializationCloner<Reader>::CloneHostObjectProperties(SrcTypeId typeId, Src src, Dst dst)
    {
        // We have already created host obect.
    }

    template <class Reader>
    void DeserializationCloner<Reader>::CloneProperties(SrcTypeId typeId, Src src, Dst dst)
    {
        // ScriptContext* scriptContext = GetScriptContext();
        RecyclableObject* obj = VarTo<RecyclableObject>(dst);

        if (obj->IsExternal()) // Read host object properties
        {
            Assert(false);
        }
        else // Read native object properties
        {
            // Read array index named properties
            if (typeId == SCA_DenseArray)
            {
                JavascriptArray* arr = JavascriptArray::FromAnyArray(obj); // (might be ES5Array if -ForceES5Array)
                uint32 length = arr->GetLength();
                for (uint32 i = 0; i < length; i++)
                {
                    Dst value = NULL;
                    this->GetEngine()->Clone(m_reader->GetPosition(), &value);
                    if (value)
                    {
                        arr->DirectSetItemAt(i, value); //Note: no prototype check
                    }
                }
            }
            else if (typeId == SCA_SparseArray)
            {
                JavascriptArray* arr = JavascriptArray::FromAnyArray(obj); // (might be ES5Array if -ForceES5Array)
                while (true)
                {
                    uint32 i;
                    Read(&i);
                    if (i == SCA_PROPERTY_TERMINATOR)
                    {
                        break;
                    }

                    Dst value = NULL;
                    this->GetEngine()->Clone(m_reader->GetPosition(), &value);
                    if (value == NULL)
                    {
                        this->ThrowSCADataCorrupt();
                    }

                    arr->DirectSetItemAt(i, value); //Note: no prototype check
                }
            }

            // Read non-index named properties
            ReadObjectPropertiesIntoObject(obj);
        }
    }

    template <class Reader>
    void DeserializationCloner<Reader>::CloneMap(Src src, Dst dst)
    {
        JavascriptMap* map = VarTo<JavascriptMap>(dst);

        int32 size;
        m_reader->Read(&size);

        for (int i = 0; i < size; i++)
        {
            Var key;
            Var value;

            this->GetEngine()->Clone(m_reader->GetPosition(), &key);
            if (!key)
            {
                this->ThrowSCADataCorrupt();
            }

            this->GetEngine()->Clone(m_reader->GetPosition(), &value);
            if (!value)
            {
                this->ThrowSCADataCorrupt();
            }

            map->Set(key, value);
        }
    }

    template <class Reader>
    void DeserializationCloner<Reader>::CloneSet(Src src, Dst dst)
    {
        JavascriptSet* set = VarTo<JavascriptSet>(dst);

        int32 size;
        m_reader->Read(&size);

        for (int i = 0; i < size; i++)
        {
            Var value;

            this->GetEngine()->Clone(m_reader->GetPosition(), &value);
            if (!value)
            {
                this->ThrowSCADataCorrupt();
            }

            set->Add(value);
        }
    }

    template <class Reader>
    void DeserializationCloner<Reader>::CloneObjectReference(Src src, Dst dst)
    {
        Assert(FALSE); // Should never call this. Object reference handled explictly.
    }
    
    //
    // Try to read a SCAString layout in the form of: [byteLen] [string content] [padding].
    // SCAString is also used for property name in object layout. In case of property terminator,
    // SCA_PROPERTY_TERMINATOR will appear at the place of [byteLen]. Return false in this case.
    //
    // If buffer is not null and the size is appropriate, will try reusing it
    //
    template <class Reader>
    const char16* DeserializationCloner<Reader>::TryReadString(charcount_t* len, bool reuseBuffer) const
    {
        // m_buffer is allocated on GC heap and stored in a regular field.
        // that is ok since 'this' is always a stack instance.
        Assert(ThreadContext::IsOnStack(this));

        uint32 byteLen;
        m_reader->Read(&byteLen);

        if (byteLen == SCA_PROPERTY_TERMINATOR)
        {
            return nullptr;
        }
        else if (byteLen == 0)
        {
            *len = 0;
            return _u("");
        }
        else
        {
            charcount_t newLen = byteLen / sizeof(char16);
            char16* buf;

            if (reuseBuffer)
            {
                if (this->m_bufferLength < newLen)
                {
                    Recycler* recycler = this->GetScriptContext()->GetRecycler();
                    this->m_buffer = RecyclerNewArrayLeaf(recycler, char16, newLen + 1);
                    this->m_bufferLength = newLen;
                }

                buf = this->m_buffer;
            }
            else
            {
                Recycler* recycler = this->GetScriptContext()->GetRecycler();
                buf = RecyclerNewArrayLeaf(recycler, char16, newLen + 1);
            }

            m_reader->Read(buf, byteLen);
            buf[newLen] = NULL;
            *len = newLen;

            uint32 unalignedLen = byteLen % sizeof(uint32);
            if (unalignedLen)
            {
                uint32 padding;
                m_reader->Read(&padding, sizeof(uint32) - unalignedLen);
            }

            return buf;
        }
    }

    //
    // Read a SCAString value from layout: [byteLen] [string content] [padding].
    // Throw if seeing SCA_PROPERTY_TERMINATOR.
    //
    template <class Reader>
    const char16* DeserializationCloner<Reader>::ReadString(charcount_t* len) const
    {
        const char16* str = TryReadString(len, false);

        if (str == nullptr)
        {
            this->ThrowSCADataCorrupt();
        }

        return str;
    }

    //
    // Read bytes data: [bytes] [padding]
    //
    template <class Reader>
    void DeserializationCloner<Reader>::Read(BYTE* buf, uint32 len) const
    {
        m_reader->Read(buf, len);

        uint32 unalignedLen = len % sizeof(uint32);
        if (unalignedLen)
        {
            uint32 padding;
            m_reader->Read(&padding, sizeof(uint32) - unalignedLen);
        }
    }

    //
    // Read a TypedArray or DataView.
    //
    template <class Reader>
    void DeserializationCloner<Reader>::ReadTypedArray(SrcTypeId typeId, Dst* dst) const
    {
        switch (typeId)
        {
        case SCA_Int8Array:
            ReadTypedArray<int8, false>(dst);
            break;

        case SCA_Uint8Array:
            ReadTypedArray<uint8, false>(dst);
            break;

        case SCA_Uint8ClampedArray:
            ReadTypedArray<uint8, true>(dst);
            break;

        case SCA_Int16Array:
            ReadTypedArray<int16, false>(dst);
            break;

        case SCA_Uint16Array:
            ReadTypedArray<uint16, false>(dst);
            break;

        case SCA_Int32Array:
            ReadTypedArray<int32, false>(dst);
            break;

        case SCA_Uint32Array:
            ReadTypedArray<uint32, false>(dst);
            break;

        case SCA_Float32Array:
            ReadTypedArray<float, false>(dst);
            break;

        case SCA_Float64Array:
            ReadTypedArray<double, false>(dst);
            break;

        case SCA_DataView:
            ReadTypedArray<DataView, false>(dst);
            break;

        default:
            Assert(false);
            break;
        }
    }

    template class DeserializationCloner<StreamReader>;

    Var SCADeserializationEngine::Deserialize(StreamReader* reader, Var* transferableVars, size_t cTransferableVars)
    {
        ScriptContext* scriptContext = reader->GetScriptContext();
        StreamDeserializationCloner cloner(scriptContext, reader);

        // Read version
        uint32 version;
        reader->Read(&version);
        if (GetSCAMajor(version) > SCA_FORMAT_MAJOR)
        {
            cloner.ThrowSCANewVersion();
        }
        Var value = SCAEngine<scaposition_t, Var, StreamDeserializationCloner>::Clone(reader->GetPosition(), &cloner, transferableVars, cTransferableVars);
        if (!value)
        {
            cloner.ThrowSCADataCorrupt();
        }

        return value;
    }
}

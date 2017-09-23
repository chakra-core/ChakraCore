//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    CompileAssert(sizeof(ES5Array) == sizeof(JavascriptArray));

    ES5ArrayType::ES5ArrayType(DynamicType* type)
        : DynamicType(type->GetScriptContext(), TypeIds_ES5Array, type->GetPrototype(), type->GetEntryPoint(), type->GetTypeHandler(), false, false)
    {
    }

    bool ES5Array::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_ES5Array;
    }

    ES5Array* ES5Array::FromVar(Var instance)
    {
        Assert(Is(instance));
        return static_cast<ES5Array*>(instance);
    }

    DynamicType* ES5Array::DuplicateType()
    {
        return RecyclerNew(GetScriptContext()->GetRecycler(), ES5ArrayType, this->GetDynamicType());
    }

    bool ES5Array::IsLengthWritable() const
    {
        return GetTypeHandler()->IsLengthWritable();
    }

    PropertyQueryFlags ES5Array::HasPropertyQuery(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return PropertyQueryFlags::Property_Found;
        }

        // Skip JavascriptArray override
        return DynamicObject::HasPropertyQuery(propertyId);
    }

    BOOL ES5Array::IsWritable(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return IsLengthWritable();
        }

        return __super::IsWritable(propertyId);
    }

    BOOL ES5Array::SetEnumerable(PropertyId propertyId, BOOL value)
    {
        // Skip JavascriptArray override
        return DynamicObject::SetEnumerable(propertyId, value);
    }

    BOOL ES5Array::SetWritable(PropertyId propertyId, BOOL value)
    {
        // Skip JavascriptArray override
        return DynamicObject::SetWritable(propertyId, value);
    }

    BOOL ES5Array::SetConfigurable(PropertyId propertyId, BOOL value)
    {
        // Skip JavascriptArray override
        return DynamicObject::SetConfigurable(propertyId, value);
    }

    BOOL ES5Array::SetAttributes(PropertyId propertyId, PropertyAttributes attributes)
    {
        // Skip JavascriptArray override
        return DynamicObject::SetAttributes(propertyId, attributes);
    }

    PropertyQueryFlags ES5Array::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        if (GetPropertyBuiltIns(propertyId, value, &result))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(result);
        }

        // Skip JavascriptArray override
        return DynamicObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    PropertyQueryFlags ES5Array::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, &result))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(result);
        }

        // Skip JavascriptArray override
        return DynamicObject::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext);
    }

    bool ES5Array::GetPropertyBuiltIns(PropertyId propertyId, Var* value, BOOL* result)
    {
        if (propertyId == PropertyIds::length)
        {
            *value = JavascriptNumber::ToVar(this->GetLength(), GetScriptContext());
            *result = true;
            return true;
        }

        return false;
    }

    PropertyQueryFlags ES5Array::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return ES5Array::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    // Convert a Var to array length, throw RangeError if value is not valid for array length.
    uint32 ES5Array::ToLengthValue(Var value, ScriptContext* scriptContext)
    {
        if (TaggedInt::Is(value))
        {
            int32 newLen = TaggedInt::ToInt32(value);
            if (newLen < 0)
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthAssignIncorrect);
            }
            return static_cast<uint32>(newLen);
        }
        else
        {
            uint32 newLen = JavascriptConversion::ToUInt32(value, scriptContext);
            if (newLen != JavascriptConversion::ToNumber(value, scriptContext))
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthAssignIncorrect);
            }
            return newLen;
        }
    }

    DescriptorFlags ES5Array::GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags result;
        if (GetSetterBuiltIns(propertyId, info, &result))
        {
            return result;
        }

        return DynamicObject::GetSetter(propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags ES5Array::GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags result;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetSetterBuiltIns(propertyRecord->GetPropertyId(), info, &result))
        {
            return result;
        }

        return DynamicObject::GetSetter(propertyNameString, setterValue, info, requestContext);
    }

    bool ES5Array::GetSetterBuiltIns(PropertyId propertyId, PropertyValueInfo* info, DescriptorFlags* result)
    {
        if (propertyId == PropertyIds::length)
        {
            PropertyValueInfo::SetNoCache(info, this);
            *result =  IsLengthWritable() ? WritableData : Data;
            return true;
        }

        return false;
    }

    BOOL ES5Array::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags propertyOperationFlags, PropertyValueInfo* info)
    {
        BOOL result;
        if (SetPropertyBuiltIns(propertyId, value, propertyOperationFlags, &result))
        {
            return result;
        }

        return __super::SetProperty(propertyId, value, propertyOperationFlags, info);
    }

    BOOL ES5Array::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags propertyOperationFlags, PropertyValueInfo* info)
    {
        BOOL result;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && SetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, propertyOperationFlags, &result))
        {
            return result;
        }

        return __super::SetProperty(propertyNameString, value, propertyOperationFlags, info);
    }

    bool ES5Array::SetPropertyBuiltIns(PropertyId propertyId, Var value, PropertyOperationFlags propertyOperationFlags, BOOL* result)
    {
        ScriptContext* scriptContext = GetScriptContext();

        if (propertyId == PropertyIds::length)
        {
            if (!GetTypeHandler()->IsLengthWritable())
            {
                *result = false; // reject
                return true;
            }

            uint32 newLen = ToLengthValue(value, scriptContext);
            uint32 assignedLen = GetTypeHandler()->SetLength(this, newLen, propertyOperationFlags);
            if (newLen != assignedLen)
            {
                scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
            }
            *result = true;
            return true;
        }

        return false;
    }

    BOOL ES5Array::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        if (propertyId == PropertyIds::length)
        {
            Assert(attributes == PropertyWritable);
            Assert(IsWritable(propertyId) && !IsConfigurable(propertyId) && !IsEnumerable(propertyId));

            uint32 newLen = ToLengthValue(value, GetScriptContext());
            GetTypeHandler()->SetLength(this, newLen, PropertyOperation_None);
            return true;
        }

        return __super::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    BOOL ES5Array::DeleteItem(uint32 index, PropertyOperationFlags flags)
    {
        // Skip JavascriptArray override
        return DynamicObject::DeleteItem(index, flags);
    }

    PropertyQueryFlags ES5Array::HasItemQuery(uint32 index)
    {
        // Skip JavascriptArray override
        return DynamicObject::HasItemQuery(index);
    }

    PropertyQueryFlags ES5Array::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        // Skip JavascriptArray override
        return DynamicObject::GetItemQuery(originalInstance, index, value, requestContext);
    }

    PropertyQueryFlags ES5Array::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        // Skip JavascriptArray override
        return DynamicObject::GetItemReferenceQuery(originalInstance, index, value, requestContext);
    }

    DescriptorFlags ES5Array::GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext)
    {
        // Skip JavascriptArray override
        return DynamicObject::GetItemSetter(index, setterValue, requestContext);
    }

    BOOL ES5Array::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        // Skip JavascriptArray override
        return DynamicObject::SetItem(index, value, flags);
    }

    BOOL ES5Array::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        // Skip JavascriptArray override
        return DynamicObject::SetAccessors(propertyId, getter, setter, flags);
    }

    BOOL ES5Array::PreventExtensions()
    {
        // Skip JavascriptArray override
        return DynamicObject::PreventExtensions();
    }

    BOOL ES5Array::Seal()
    {
        // Skip JavascriptArray override
        return DynamicObject::Seal();
    }

    BOOL ES5Array::Freeze()
    {
        // Skip JavascriptArray override
        return DynamicObject::Freeze();
    }

    BOOL ES5Array::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache)
    {
        return enumerator->Initialize(nullptr, this, this, flags, requestContext, forInCache);
    }

    JavascriptEnumerator * ES5Array::GetIndexEnumerator(EnumeratorFlags flags, ScriptContext* requestContext)
    {
        // ES5Array does not support compat mode, ignore preferSnapshotSemantics
        return RecyclerNew(GetScriptContext()->GetRecycler(), ES5ArrayIndexEnumerator, this, flags, requestContext);
    }

    BOOL ES5Array::IsItemEnumerable(uint32 index)
    {
        return GetTypeHandler()->IsItemEnumerable(this, index);
    }

    BOOL ES5Array::SetItemWithAttributes(uint32 index, Var value, PropertyAttributes attributes)
    {
        return GetTypeHandler()->SetItemWithAttributes(this, index, value, attributes);
    }

    BOOL ES5Array::SetItemAttributes(uint32 index, PropertyAttributes attributes)
    {
        return GetTypeHandler()->SetItemAttributes(this, index, attributes);
    }

    BOOL ES5Array::SetItemAccessors(uint32 index, Var getter, Var setter)
    {
        return GetTypeHandler()->SetItemAccessors(this, index, getter, setter);
    }

    BOOL ES5Array::IsObjectArrayFrozen()
    {
        return GetTypeHandler()->IsObjectArrayFrozen(this);
    }

    BOOL ES5Array::IsValidDescriptorToken(void * descriptorValidationToken) const
    {
        return GetTypeHandler()->IsValidDescriptorToken(descriptorValidationToken);
    }
    uint32 ES5Array::GetNextDescriptor(uint32 key, IndexPropertyDescriptor** descriptor, void ** descriptorValidationToken)
    {
        return GetTypeHandler()->GetNextDescriptor(key, descriptor, descriptorValidationToken);
    }

    BOOL ES5Array::GetDescriptor(uint32 index, Js::IndexPropertyDescriptor **ppDescriptor)
    {
        return GetTypeHandler()->GetDescriptor(index, ppDescriptor);
    }

#if ENABLE_TTD
    void ES5Array::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        this->JavascriptArray::MarkVisitKindSpecificPtrs(extractor);

        uint32 length = this->GetLength();
        uint32 descriptorIndex = Js::JavascriptArray::InvalidIndex;
        IndexPropertyDescriptor* descriptor = nullptr;
        void* descriptorValidationToken = nullptr;

        do
        {
            descriptorIndex = this->GetNextDescriptor(descriptorIndex, &descriptor, &descriptorValidationToken);
            if(descriptorIndex == Js::JavascriptArray::InvalidIndex || descriptorIndex >= length)
            {
                break;
            }

            if((descriptor->Attributes & PropertyDeleted) != PropertyDeleted)
            {
                if(descriptor->Getter != nullptr)
                {
                    extractor->MarkVisitVar(descriptor->Getter);
                }

                if(descriptor->Setter != nullptr)
                {
                    extractor->MarkVisitVar(descriptor->Setter);
                }
            }

        } while(true);
    }

    TTD::NSSnapObjects::SnapObjectType ES5Array::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapES5ArrayObject;
    }

    void ES5Array::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapArrayInfo<TTD::TTDVar>* sai = TTD::NSSnapObjects::ExtractArrayValues<TTD::TTDVar>(this, alloc);

        TTD::NSSnapObjects::SnapES5ArrayInfo* es5ArrayInfo = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapES5ArrayInfo>();

        //
        //TODO: reserving memory for entire length might be a problem if we have very large sparse arrays.
        //

        uint32 length = this->GetLength();

        es5ArrayInfo->IsLengthWritable = this->IsLengthWritable();
        es5ArrayInfo->GetterSetterCount = 0;
        es5ArrayInfo->GetterSetterEntries = alloc.SlabReserveArraySpace<TTD::NSSnapObjects::SnapES5ArrayGetterSetterEntry>(length + 1); //ensure we don't do a 0 reserve

        uint32 descriptorIndex = Js::JavascriptArray::InvalidIndex;
        IndexPropertyDescriptor* descriptor = nullptr;
        void* descriptorValidationToken = nullptr;

        do
        {
            descriptorIndex = this->GetNextDescriptor(descriptorIndex, &descriptor, &descriptorValidationToken);
            if(descriptorIndex == Js::JavascriptArray::InvalidIndex || descriptorIndex >= length)
            {
                break;
            }

            if((descriptor->Attributes & PropertyDeleted) != PropertyDeleted)
            {
                TTD::NSSnapObjects::SnapES5ArrayGetterSetterEntry* entry = es5ArrayInfo->GetterSetterEntries + es5ArrayInfo->GetterSetterCount;
                es5ArrayInfo->GetterSetterCount++;

                entry->Index = (uint32)descriptorIndex;
                entry->Attributes = descriptor->Attributes;

                entry->Getter = nullptr;
                if(descriptor->Getter != nullptr)
                {
                    entry->Getter = descriptor->Getter;
                }

                entry->Setter = nullptr;
                if(descriptor->Setter != nullptr)
                {
                    entry->Setter = descriptor->Setter;
                }
            }

        } while(true);

        if(es5ArrayInfo->GetterSetterCount != 0)
        {
            alloc.SlabCommitArraySpace<TTD::NSSnapObjects::SnapES5ArrayGetterSetterEntry>(es5ArrayInfo->GetterSetterCount, length + 1);
        }
        else
        {
            alloc.SlabAbortArraySpace<TTD::NSSnapObjects::SnapES5ArrayGetterSetterEntry>(length + 1);
            es5ArrayInfo->GetterSetterEntries = nullptr;
        }

        es5ArrayInfo->BasicArrayData = sai;

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapES5ArrayInfo*, TTD::NSSnapObjects::SnapObjectType::SnapES5ArrayObject>(objData, es5ArrayInfo);
    }
#endif
}

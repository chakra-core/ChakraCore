//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptStringObject::JavascriptStringObject(DynamicType * type)
        : DynamicObject(type), value(nullptr)
    {
        Assert(type->GetTypeId() == TypeIds_StringObject);

        this->GetTypeHandler()->ClearHasOnlyWritableDataProperties(); // length is non-writable
        if(GetTypeHandler()->GetFlags() & DynamicTypeHandler::IsPrototypeFlag)
        {

            // No need to invalidate store field caches for non-writable properties here. Since this type is just being created, it cannot represent
            // an object that is already a prototype. If it becomes a prototype and then we attempt to add a property to an object derived from this
            // object, then we will check if this property is writable, and only if it is will we do the fast path for add property.
            GetLibrary()->NoPrototypeChainsAreEnsuredToHaveOnlyWritableDataProperties();
        }
    }

    JavascriptStringObject::JavascriptStringObject(JavascriptString* value, DynamicType * type)
        : DynamicObject(type), value(value)
    {
        Assert(type->GetTypeId() == TypeIds_StringObject);

        this->GetTypeHandler()->ClearHasOnlyWritableDataProperties(); // length is non-writable
        if(GetTypeHandler()->GetFlags() & DynamicTypeHandler::IsPrototypeFlag)
        {
            // No need to invalidate store field caches for non-writable properties here. Since this type is just being created, it cannot represent
            // an object that is already a prototype. If it becomes a prototype and then we attempt to add a property to an object derived from this
            // object, then we will check if this property is writable, and only if it is will we do the fast path for add property.
            GetLibrary()->NoPrototypeChainsAreEnsuredToHaveOnlyWritableDataProperties();
        }
    }

    bool JavascriptStringObject::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_StringObject;
    }

    JavascriptStringObject* JavascriptStringObject::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptString'");

        return static_cast<JavascriptStringObject *>(RecyclableObject::FromVar(aValue));
    }

    void JavascriptStringObject::Initialize(JavascriptString* value)
    {
        Assert(this->value == nullptr);

        this->value = value;
    }

    JavascriptString* JavascriptStringObject::InternalUnwrap()
    {
        if (value == nullptr)
        {
            ScriptContext* scriptContext = GetScriptContext();
            value = scriptContext->GetLibrary()->GetEmptyString();
        }

        return value;
    }

    /*static*/
    PropertyId const JavascriptStringObject::specialPropertyIds[] =
    {
        PropertyIds::length
    };

    bool JavascriptStringObject::IsValidIndex(PropertyId propertyId, bool conditionMetBehavior)
    {
        ScriptContext*scriptContext = GetScriptContext();
        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            if (index < (uint32)this->InternalUnwrap()->GetLength())
            {
                return conditionMetBehavior;
            }
        }
        return !conditionMetBehavior;
    }

    PropertyQueryFlags JavascriptStringObject::HasPropertyQuery(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return PropertyQueryFlags::Property_Found;
        }

        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasPropertyQuery(propertyId)))
        {
            return PropertyQueryFlags::Property_Found;
        }

        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptStringObject::IsValidIndex(propertyId, true));
    }

    DescriptorFlags JavascriptStringObject::GetSetter(PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags flags;
        if (GetSetterBuiltIns(propertyId, info, &flags))
        {
            return flags;
        }

        uint32 index;
        if (requestContext->IsNumericPropertyId(propertyId, &index))
        {
            return JavascriptStringObject::GetItemSetter(index, setterValue, requestContext);
        }

        return DynamicObject::GetSetter(propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags JavascriptStringObject::GetSetter(JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags flags;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr)
        {
            PropertyId propertyId = propertyRecord->GetPropertyId();
            if (GetSetterBuiltIns(propertyId, info, &flags))
            {
                return flags;
            }

            uint32 index;
            if (requestContext->IsNumericPropertyId(propertyId, &index))
            {
                return JavascriptStringObject::GetItemSetter(index, setterValue, requestContext);
            }
        }

        return DynamicObject::GetSetter(propertyNameString, setterValue, info, requestContext);
    }

    bool JavascriptStringObject::GetSetterBuiltIns(PropertyId propertyId, PropertyValueInfo* info, DescriptorFlags* descriptorFlags)
    {
        if (propertyId == PropertyIds::length)
        {
            PropertyValueInfo::SetNoCache(info, this);
            *descriptorFlags = Data;
            return true;
        }

        return false;
    }

    BOOL JavascriptStringObject::IsConfigurable(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }

        // From DynamicObject::IsConfigurable we can't tell if the result is from a property or just default
        // value. Call HasProperty to find out.
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasPropertyQuery(propertyId)))
        {
            return DynamicObject::IsConfigurable(propertyId);
        }

        return JavascriptStringObject::IsValidIndex(propertyId, false);
    }

    BOOL JavascriptStringObject::IsEnumerable(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }

        // Index properties of String objects are always enumerable, same as default value. No need to test.
        return DynamicObject::IsEnumerable(propertyId);
    }

    BOOL JavascriptStringObject::IsWritable(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }

        // From DynamicObject::IsWritable we can't tell if the result is from a property or just default
        // value. Call HasProperty to find out.
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasPropertyQuery(propertyId)))
        {
            return DynamicObject::IsWritable(propertyId);
        }

        return JavascriptStringObject::IsValidIndex(propertyId, false);
    }

    BOOL JavascriptStringObject::GetSpecialPropertyName(uint32 index, JavascriptString ** propertyName, ScriptContext * requestContext)
    {
        if (index == 0)
        {
            *propertyName = requestContext->GetPropertyString(PropertyIds::length);
            return true;
        }
        return false;
    }

    // Returns the number of special non-enumerable properties this type has.
    uint JavascriptStringObject::GetSpecialPropertyCount() const
    {
        return _countof(specialPropertyIds);
    }

    // Returns the list of special non-enumerable properties for the type.
    PropertyId const * JavascriptStringObject::GetSpecialPropertyIds() const
    {
        return specialPropertyIds;
    }

    PropertyQueryFlags JavascriptStringObject::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptStringObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }


    PropertyQueryFlags JavascriptStringObject::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        if (GetPropertyBuiltIns(propertyId, value, requestContext, &result))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(result);
        }

        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext)))
        {
            return PropertyQueryFlags::Property_Found;
        }

        // For NumericPropertyIds check that index is less than JavascriptString length
        ScriptContext*scriptContext = GetScriptContext();
        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            JavascriptString* str = JavascriptString::FromVar(CrossSite::MarshalVar(requestContext, this->InternalUnwrap()));
            return JavascriptConversion::BooleanToPropertyQueryFlags(str->GetItemAt(index, value));
        }

        *value = requestContext->GetMissingPropertyResult();
        return PropertyQueryFlags::Property_NotFound;
    }

    PropertyQueryFlags JavascriptStringObject::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord*");

        BOOL result;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, requestContext, &result))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(result);
        }

        return DynamicObject::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext);
    }

    bool JavascriptStringObject::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext, BOOL* result)
    {
        if (propertyId == PropertyIds::length)
        {
            *value = JavascriptNumber::ToVar(this->InternalUnwrap()->GetLength(), requestContext);
            *result = true;
            return true;
        }

        return false;
    }

    BOOL JavascriptStringObject::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        bool result;
        if (SetPropertyBuiltIns(propertyId, flags, &result))
        {
            return result;
        }

        return DynamicObject::SetProperty(propertyId, value, flags, info);
    }

    BOOL JavascriptStringObject::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        bool result;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && SetPropertyBuiltIns(propertyRecord->GetPropertyId(), flags, &result))
        {
            return result;
        }
        return DynamicObject::SetProperty(propertyNameString, value, flags, info);
    }

    bool JavascriptStringObject::SetPropertyBuiltIns(PropertyId propertyId, PropertyOperationFlags flags, bool* result)
    {
        if (propertyId == PropertyIds::length)
        {
            JavascriptError::ThrowCantAssignIfStrictMode(flags, this->GetScriptContext());

            *result = false;
            return true;
        }

        return false;
    }

    BOOL JavascriptStringObject::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        if (propertyId == PropertyIds::length)
        {
            JavascriptError::ThrowCantDeleteIfStrictMode(flags, this->GetScriptContext(), this->GetScriptContext()->GetPropertyName(propertyId)->GetBuffer());

            return FALSE;
        }
        return DynamicObject::DeleteProperty(propertyId, flags);
    }

    BOOL JavascriptStringObject::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags propertyOperationFlags)
    {
        JsUtil::CharacterBuffer<WCHAR> propertyName(propertyNameString->GetString(), propertyNameString->GetLength());
        if (BuiltInPropertyRecords::length.Equals(propertyName))
        {
            JavascriptError::ThrowCantDeleteIfStrictMode(propertyOperationFlags, this->GetScriptContext(), propertyNameString->GetString());

            return FALSE;
        }
        return DynamicObject::DeleteProperty(propertyNameString, propertyOperationFlags);
    }

    PropertyQueryFlags JavascriptStringObject::HasItemQuery(uint32 index)
    {
        if (this->InternalUnwrap()->HasItem(index))
        {
            return PropertyQueryFlags::Property_Found;
        }
        return DynamicObject::HasItemQuery(index);
    }

    PropertyQueryFlags JavascriptStringObject::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        JavascriptString* str = JavascriptString::FromVar(CrossSite::MarshalVar(requestContext, this->InternalUnwrap()));
        if (str->GetItemAt(index, value))
        {
            return PropertyQueryFlags::Property_Found;
        }
        return DynamicObject::GetItemQuery(originalInstance, index, value, requestContext);
    }

    PropertyQueryFlags JavascriptStringObject::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return this->GetItemQuery(originalInstance, index, value, requestContext);
    }

    DescriptorFlags JavascriptStringObject::GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext)
    {
        if (index < (uint32)this->InternalUnwrap()->GetLength())
        {
            return DescriptorFlags::Data;
        }
        return DynamicObject::GetItemSetter(index, setterValue, requestContext);
    }

    BOOL JavascriptStringObject::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        if (index < (uint32)this->InternalUnwrap()->GetLength())
        {
            return false;
        }
        return DynamicObject::SetItem(index, value, flags);
    }

    BOOL JavascriptStringObject::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache)
    {
        return GetEnumeratorWithPrefix(
            RecyclerNew(GetScriptContext()->GetRecycler(), JavascriptStringEnumerator, this->Unwrap(), requestContext),
            enumerator, flags, requestContext, forInCache);
    }

    BOOL JavascriptStringObject::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->Append(_u('"'));
        stringBuilder->Append(this->InternalUnwrap()->GetString(), this->InternalUnwrap()->GetLength());
        stringBuilder->Append(_u('"'));
        return TRUE;
    }

    BOOL JavascriptStringObject::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("String"));
        return TRUE;
    }

#if ENABLE_TTD
    void JavascriptStringObject::SetValue_TTD(Js::Var val)
    {
        AssertMsg(val == nullptr || Js::JavascriptString::Is(val), "Only legal values!");

        this->value = static_cast<Js::JavascriptString*>(val);
    }

    void JavascriptStringObject::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        if(this->value != nullptr)
        {
            extractor->MarkVisitVar(this->value);
        }
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptStringObject::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapBoxedValueObject;
    }

    void JavascriptStringObject::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::TTDVar, TTD::NSSnapObjects::SnapObjectType::SnapBoxedValueObject>(objData, this->value);
    }
#endif
} // namespace Js

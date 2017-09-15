//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    ModuleRoot::ModuleRoot(DynamicType * type):
        RootObjectBase(type)
    {
    }

    void ModuleRoot::SetHostObject(ModuleID moduleID, HostObjectBase * hostObject)
    {
        this->moduleID = moduleID;
        __super::SetHostObject(hostObject);
    }

    PropertyQueryFlags ModuleRoot::HasPropertyQuery(PropertyId propertyId)
    {
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasPropertyQuery(propertyId)))
        {
            return PropertyQueryFlags::Property_Found;
        }
        else if (this->hostObject && JavascriptOperators::HasProperty(this->hostObject, propertyId))
        {
            return PropertyQueryFlags::Property_Found;
        }
        return this->GetLibrary()->GetGlobalObject()->GlobalObject::HasPropertyQuery(propertyId);
    }

    BOOL ModuleRoot::EnsureProperty(PropertyId propertyId)
    {
        if (!RootObjectBase::HasOwnPropertyCheckNoRedecl(propertyId))
        {
            // Cannot pass the extra PropertyOperation_PreInit flag, because module root uses SetSlot directly from
            // SetRootProperty. If the property is not yet initialized SetSlot will (correctly) assert.
            this->InitProperty(propertyId, this->GetLibrary()->GetUndefined(), (PropertyOperationFlags)(PropertyOperation_SpecialValue | PropertyOperation_NonFixedValue));
        }
        return true;
    }

    BOOL ModuleRoot::HasRootProperty(PropertyId propertyId)
    {
        if (__super::HasRootProperty(propertyId))
        {
            return TRUE;
        }
        else if (this->hostObject && JavascriptOperators::HasProperty(this->hostObject, propertyId))
        {
            return TRUE;
        }
        return this->GetLibrary()->GetGlobalObject()->GlobalObject::HasRootProperty(propertyId);
    }

    BOOL ModuleRoot::HasOwnProperty(PropertyId propertyId)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasPropertyQuery(propertyId));
    }

    PropertyQueryFlags ModuleRoot::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyIndex index = GetPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            *value = this->GetSlot(index);
            if (info) // Avoid testing IsWritable if info not being queried
            {
                PropertyValueInfo::Set(info, this, index, IsWritable(propertyId) ? PropertyWritable : PropertyNone);
#if ENABLE_FIXED_FIELDS
                if (this->IsFixedProperty(propertyId))
                {
                    PropertyValueInfo::DisableStoreFieldCache(info);
                }
#endif
            }
            return PropertyQueryFlags::Property_Found;
        }
        if (this->hostObject && JavascriptOperators::GetProperty(this->hostObject, propertyId, value, requestContext))
        {
            return PropertyQueryFlags::Property_Found;
        }

        //
        // Try checking the global object
        // if the module root doesn't have the property and the host object also doesn't have it
        //

        GlobalObject* globalObj = this->GetLibrary()->GetGlobalObject();
        return globalObj->GlobalObject::GetPropertyQuery(originalInstance, propertyId, value, NULL, requestContext);
    }

    BOOL ModuleRoot::GetRootProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyIndex index = GetRootPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            *value = this->GetSlot(index);
            if (info) // Avoid testing IsWritable if info not being queried
            {
                PropertyValueInfo::Set(info, this, index, IsWritable(propertyId) ? PropertyWritable : PropertyNone);
#if ENABLE_FIXED_FIELDS
                if (this->IsFixedProperty(propertyId))
                {
                    PropertyValueInfo::DisableStoreFieldCache(info);
                }
#endif
            }
            return TRUE;
        }
        if (this->hostObject && JavascriptOperators::GetProperty(this->hostObject, propertyId, value, requestContext))
        {
            return TRUE;
        }

        //
        // Try checking the global object
        // if the module root doesn't have the property and the host object also doesn't have it
        //

        GlobalObject* globalObj = this->GetLibrary()->GetGlobalObject();
        return globalObj->GlobalObject::GetRootProperty(originalInstance, propertyId, value, NULL, requestContext);
    }

    PropertyQueryFlags ModuleRoot::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const * propertyRecord;
        this->GetScriptContext()->GetOrAddPropertyRecord(propertyNameString->GetString(), propertyNameString->GetLength(), &propertyRecord);
        return ModuleRoot::GetPropertyQuery(originalInstance, propertyRecord->GetPropertyId(), value, info, requestContext);
    }

    BOOL ModuleRoot::GetAccessors(PropertyId propertyId, Var* getter, Var* setter, ScriptContext * requestContext)
    {
        if (DynamicObject::GetAccessors(propertyId, getter, setter, requestContext))
        {
            return TRUE;
        }
        if (this->hostObject)
        {
            return this->hostObject->GetAccessors(propertyId, getter, setter, requestContext);
        }

        // Try checking the global object
        GlobalObject* globalObj = this->GetLibrary()->GetGlobalObject();
        return globalObj->GlobalObject::GetAccessors(propertyId, getter, setter, requestContext);
    }

    PropertyQueryFlags ModuleRoot::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info,
        ScriptContext* requestContext)
    {
        PropertyIndex index = GetPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            *value = this->GetSlot(index);
            if (info) // Avoid testing IsWritable if info not being queried
            {
                PropertyValueInfo::Set(info, this, index, IsWritable(propertyId) ? PropertyWritable : PropertyNone);
#if ENABLE_FIXED_FIELDS
                if (this->IsFixedProperty(propertyId))
                {
                    PropertyValueInfo::DisableStoreFieldCache(info);
                }
#endif
            }
            return PropertyQueryFlags::Property_Found;
        }
        if (this->hostObject && JavascriptOperators::GetPropertyReference(this->hostObject, propertyId, value, requestContext))
        {
            return PropertyQueryFlags::Property_NotFound;
        }

        //
        // Try checking the global object
        // if the module root doesn't have the property and the host object also doesn't have it
        //

        GlobalObject* globalObj = this->GetLibrary()->GetGlobalObject();
        return globalObj->GlobalObject::GetPropertyReferenceQuery(originalInstance, propertyId, value, NULL, requestContext);
    }

    BOOL ModuleRoot::GetRootPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info,
        ScriptContext* requestContext)
    {
        PropertyIndex index = GetRootPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            *value = this->GetSlot(index);
            if (info) // Avoid testing IsWritable if info not being queried
            {
                PropertyValueInfo::Set(info, this, index, IsWritable(propertyId) ? PropertyWritable : PropertyNone);
#if ENABLE_FIXED_FIELDS
                if (this->IsFixedProperty(propertyId))
                {
                    PropertyValueInfo::DisableStoreFieldCache(info);
                }
#endif
            }
            return TRUE;
        }
        if (this->hostObject && JavascriptOperators::GetPropertyReference(this->hostObject, propertyId, value, requestContext))
        {
            return TRUE;
        }

        //
        // Try checking the global object
        // if the module root doesn't have the property and the host object also doesn't have it
        //

        GlobalObject* globalObj = this->GetLibrary()->GetGlobalObject();
        return globalObj->GlobalObject::GetRootPropertyReference(originalInstance, propertyId, value, NULL, requestContext);
    }

    BOOL ModuleRoot::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PropertyIndex index = GetPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            if (this->IsWritable(propertyId) == FALSE)
            {
                JavascriptError::ThrowCantAssignIfStrictMode(flags, this->GetScriptContext());

#if ENABLE_FIXED_FIELDS
                if (this->IsFixedProperty(propertyId))
                {
                    PropertyValueInfo::SetNoCache(info, this);
                }
                else
#endif
                {
                    PropertyValueInfo::Set(info, this, index, PropertyNone); // Try to cache property info even if not writable
                }
                return FALSE;
            }
            this->SetSlot(SetSlotArguments(propertyId, index, value));
#if ENABLE_FIXED_FIELDS
            if (this->IsFixedProperty(propertyId))
            {
                PropertyValueInfo::SetNoCache(info, this);
            }
            else
#endif
            {
                PropertyValueInfo::Set(info, this, index);
            }
            return TRUE;
        }
        else if (this->hostObject && this->hostObject->HasProperty(propertyId))
        {
            return this->hostObject->SetProperty(propertyId, value, flags, NULL);
        }

        //
        // Try checking the global object
        // if the module root doesn't have the property and the host object also doesn't have it
        //
        GlobalObject* globalObj = this->GetLibrary()->GetGlobalObject();
        BOOL setAttempted = TRUE;
        if (globalObj->SetExistingProperty(propertyId, value, NULL, &setAttempted))
        {
            return TRUE;
        }

        //
        // Set was attempted. But the set operation returned false.
        // This happens, when the property is read only.
        // In those scenarios, we should be setting the property with default attributes
        //
        if (setAttempted)
        {
            return FALSE;
        }

        return DynamicObject::SetProperty(propertyId, value, flags, info);
    }

    BOOL ModuleRoot::SetRootProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PropertyIndex index = GetRootPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            if (this->IsWritable(propertyId) == FALSE)
            {
                JavascriptError::ThrowCantAssignIfStrictMode(flags, this->GetScriptContext());

#if ENABLE_FIXED_FIELDS
                if (this->IsFixedProperty(propertyId))
                {
                    PropertyValueInfo::SetNoCache(info, this);
                }
                else
#endif
                {
                    PropertyValueInfo::Set(info, this, index, PropertyNone); // Try to cache property info even if not writable
                }
                return FALSE;
            }
            this->SetSlot(SetSlotArgumentsRoot(propertyId, true, index, value));
#if ENABLE_FIXED_FIELDS
            if (this->IsFixedProperty(propertyId))
            {
                PropertyValueInfo::SetNoCache(info, this);
            }
            else
#endif
            {
                PropertyValueInfo::Set(info, this, index);
            }            
            return TRUE;
        }
        else if (this->hostObject && this->hostObject->HasProperty(propertyId))
        {
            return this->hostObject->SetProperty(propertyId, value, flags, NULL);
        }

        //
        // Try checking the global object
        // if the module root doesn't have the property and the host object also doesn't have it
        //
        GlobalObject* globalObj = this->GetLibrary()->GetGlobalObject();
        BOOL setAttempted = TRUE;
        if (globalObj->SetExistingRootProperty(propertyId, value, NULL, &setAttempted))
        {
            return TRUE;
        }

        //
        // Set was attempted. But the set operation returned false.
        // This happens, when the property is read only.
        // In those scenarios, we should be setting the property with default attributes
        //
        if (setAttempted)
        {
            return FALSE;
        }

        return __super::SetRootProperty(propertyId, value, (PropertyOperationFlags)(flags | PropertyOperation_NonFixedValue), info);
    }

    BOOL ModuleRoot::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PropertyRecord const * propertyRecord;
        this->GetScriptContext()->GetOrAddPropertyRecord(propertyNameString->GetString(), propertyNameString->GetLength(), &propertyRecord);
        return ModuleRoot::SetProperty(propertyRecord->GetPropertyId(), value, (PropertyOperationFlags)(flags | PropertyOperation_NonFixedValue), info);
    }

    BOOL ModuleRoot::InitPropertyScoped(PropertyId propertyId, Var value)
    {
        return DynamicObject::InitProperty(propertyId, value, PropertyOperation_NonFixedValue);
    }

    BOOL ModuleRoot::InitFuncScoped(PropertyId propertyId, Var value)
    {
        // Var binding of functions declared in eval are elided when conflicting
        // with global scope let/const variables, so do not actually set the
        // property if it exists and is a let/const variable.
        bool noRedecl = false;
        if (!GetTypeHandler()->HasRootProperty(this, propertyId, &noRedecl) || !noRedecl)
        {
            DynamicObject::InitProperty(propertyId, value, PropertyOperation_NonFixedValue);
        }
        return true;
    }

    BOOL ModuleRoot::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        if (DynamicObject::SetAccessors(propertyId, getter, setter, flags))
        {
            return TRUE;
        }
        if (this->hostObject)
        {
            return this->hostObject->SetAccessors(propertyId, getter, setter, flags);
        }

        //
        // Try checking the global object
        // if the module root doesn't have the property and the host object also doesn't have it
        //
        GlobalObject* globalObj = GetScriptContext()->GetGlobalObject();
        return globalObj->GlobalObject::SetAccessors(propertyId, getter, setter, flags);
    }

    BOOL ModuleRoot::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        int index = GetPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            return FALSE;
        }
        else if (this->hostObject && this->hostObject->HasProperty(propertyId))
        {
            return this->hostObject->DeleteProperty(propertyId, flags);
        }
        return this->GetLibrary()->GetGlobalObject()->GlobalObject::DeleteProperty(propertyId, flags);
    }

    BOOL ModuleRoot::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags)
    {
        PropertyRecord const *propertyRecord = nullptr;
        if (JavascriptOperators::ShouldTryDeleteProperty(this, propertyNameString, &propertyRecord))
        {
            Assert(propertyRecord);
            return DeleteProperty(propertyRecord->GetPropertyId(), flags);
        }

        return TRUE;
    }

    BOOL ModuleRoot::DeleteRootProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        int index = GetRootPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            return FALSE;
        }
        else if (this->hostObject && this->hostObject->HasProperty(propertyId))
        {
            return this->hostObject->DeleteProperty(propertyId, flags);
        }
        return this->GetLibrary()->GetGlobalObject()->GlobalObject::DeleteRootProperty(propertyId, flags);
    }

    PropertyQueryFlags ModuleRoot::HasItemQuery(uint32 index)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasItemQuery(index))
            || (this->hostObject && JavascriptOperators::HasItem(this->hostObject, index)));
    }

    BOOL ModuleRoot::HasOwnItem(uint32 index)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasItemQuery(index));
    }

    PropertyQueryFlags ModuleRoot::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetItemReferenceQuery(originalInstance, index, value, requestContext)))
        {
            return PropertyQueryFlags::Property_Found;
        }
        if (this->hostObject && JavascriptConversion::PropertyQueryFlagsToBoolean(this->hostObject->GetItemReferenceQuery(originalInstance, index, value, requestContext)))
        {
            return PropertyQueryFlags::Property_Found;
        }
        *value = requestContext->GetMissingItemResult();
        return PropertyQueryFlags::Property_NotFound;
    }

    BOOL ModuleRoot::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        if (DynamicObject::SetItem(index, value, flags))
        {
            return TRUE;
        }

        if (this->hostObject)
        {
            return this->hostObject->SetItem(index, value, flags);
        }
        return FALSE;
    }

    PropertyQueryFlags ModuleRoot::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetItemQuery(originalInstance, index, value, requestContext)))
        {
            return PropertyQueryFlags::Property_Found;
        }
        if (this->hostObject && this->hostObject->GetItem(originalInstance, index, value, requestContext))
        {
            return PropertyQueryFlags::Property_Found;
        }
        *value = requestContext->GetMissingItemResult();
        return PropertyQueryFlags::Property_NotFound;
    }

    BOOL ModuleRoot::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("{Named Item}"));
        return TRUE;
    }

    BOOL ModuleRoot::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object, (Named Item)"));
        return TRUE;
    }
}

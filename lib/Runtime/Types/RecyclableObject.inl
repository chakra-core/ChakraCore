//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // These function needs to be in INL file for static lib

    inline TypeId RecyclableObject::GetTypeId() const
    {
        return this->GetType()->GetTypeId();
    }

    inline JavascriptLibrary* RecyclableObject::GetLibrary() const
    {
        return this->GetType()->GetLibrary();
    }

    inline ScriptContext* RecyclableObject::GetScriptContext() const
    {
        return this->GetLibrary()->GetScriptContext();
    }

    inline BOOL RecyclableObject::IsExternal() const
    {
        Assert(this->IsExternalVirtual() == this->GetType()->IsExternal());
        return this->GetType()->IsExternal();
    }

    inline BOOL RecyclableObject::HasItem(uint32 index)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(HasItemQuery(index));
    }

    inline BOOL RecyclableObject::HasProperty(PropertyId propertyId)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(HasPropertyQuery(propertyId, nullptr /*info*/));
    }

    inline BOOL RecyclableObject::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(GetPropertyQuery(originalInstance, propertyId, value, info, requestContext));
    }

    inline BOOL RecyclableObject::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext));
    }

    inline BOOL RecyclableObject::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(GetPropertyReferenceQuery(originalInstance, propertyId, value, info, requestContext));
    }

    inline BOOL RecyclableObject::GetItem(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(GetItemQuery(originalInstance, index, value, requestContext));
    }

    inline BOOL RecyclableObject::GetItemReference(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(GetItemReferenceQuery(originalInstance, index, value, requestContext));
    }
};

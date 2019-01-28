//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    PropertyQueryFlags UnscopablesWrapperObject::HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptOperators::HasPropertyUnscopables(wrappedObject, propertyId));
    }

    BOOL UnscopablesWrapperObject::HasOwnProperty(PropertyId propertyId)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return HasProperty(propertyId);
    }

    BOOL UnscopablesWrapperObject::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        return JavascriptOperators::SetPropertyUnscopable(wrappedObject, wrappedObject, propertyId, value, info, wrappedObject->GetScriptContext());
    }

    PropertyQueryFlags UnscopablesWrapperObject::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptOperators::GetPropertyUnscopable(wrappedObject, wrappedObject, propertyId, value, requestContext, info));
    }


    BOOL UnscopablesWrapperObject::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        return JavascriptOperators::DeletePropertyUnscopables(wrappedObject, propertyId, flags);
    }

    DescriptorFlags UnscopablesWrapperObject::GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptOperators::GetterSetterUnscopable(wrappedObject, propertyId, setterValue, info, requestContext);
    }

    PropertyQueryFlags UnscopablesWrapperObject::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        RecyclableObject* copyState = wrappedObject;
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptOperators::PropertyReferenceWalkUnscopable(wrappedObject, &copyState, propertyId, value, info, requestContext));
    }

} // namespace Js

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    bool WithScopeObject::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_WithScopeObject;
    }
    WithScopeObject* WithScopeObject::FromVar(Var aValue)
    {
        Assert(WithScopeObject::Is(aValue));
        return static_cast<WithScopeObject*>(aValue);
    }

    PropertyQueryFlags WithScopeObject::HasPropertyQuery(PropertyId propertyId)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptOperators::HasPropertyUnscopables(wrappedObject, propertyId));
    }

    BOOL WithScopeObject::HasOwnProperty(PropertyId propertyId)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return HasProperty(propertyId);
    }

    BOOL WithScopeObject::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        return JavascriptOperators::SetPropertyUnscopable(wrappedObject, wrappedObject, propertyId, value, info, wrappedObject->GetScriptContext());
    }

    PropertyQueryFlags WithScopeObject::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptOperators::GetPropertyUnscopable(wrappedObject, wrappedObject, propertyId, value, requestContext, info));
    }


    BOOL WithScopeObject::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        return JavascriptOperators::DeletePropertyUnscopables(wrappedObject, propertyId, flags);
    }

    DescriptorFlags WithScopeObject::GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptOperators::GetterSetterUnscopable(wrappedObject, propertyId, setterValue, info, requestContext);
    }

    PropertyQueryFlags WithScopeObject::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        RecyclableObject* copyState = wrappedObject;
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptOperators::PropertyReferenceWalkUnscopable(wrappedObject, &copyState, propertyId, value, info, requestContext));
    }

} // namespace Js

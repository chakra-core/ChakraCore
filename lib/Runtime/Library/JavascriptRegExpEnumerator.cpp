//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
namespace Js
{
    JavascriptRegExpEnumerator::JavascriptRegExpEnumerator(JavascriptRegExpConstructor* regExpObject, EnumeratorFlags flags, ScriptContext * requestContext) :
        JavascriptEnumerator(requestContext),
        flags(flags),
        regExpObject(regExpObject)        
    {
        index = (uint)-1;
    }

    void JavascriptRegExpEnumerator::Reset()
    {
        index = (uint)-1;
    }

    JavascriptString * JavascriptRegExpEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        propertyId = Constants::NoProperty;
        ScriptContext* scriptContext = this->GetScriptContext();

        JavascriptString * item = nullptr;
        if (regExpObject->GetSpecialEnumerablePropertyName(++index, &item, scriptContext))
        {
            if (attributes != nullptr)
            {
                *attributes = PropertyEnumerable;
            }
            return item;
        }

        index = regExpObject->GetSpecialEnumerablePropertyCount();
        return nullptr;
    }
}

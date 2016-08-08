//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
namespace Js
{
    JavascriptRegExpEnumerator::JavascriptRegExpEnumerator(JavascriptRegExpConstructor* regExpObject, ScriptContext * requestContext, BOOL enumNonEnumerable) :
        JavascriptEnumerator(requestContext),
        regExpObject(regExpObject),
        enumNonEnumerable(enumNonEnumerable)
    {
        index = (uint)-1;
    }

    void JavascriptRegExpEnumerator::Reset()
    {
        index = (uint)-1;
    }

    Var JavascriptRegExpEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        propertyId = Constants::NoProperty;
        ScriptContext* scriptContext = regExpObject->GetScriptContext();
        if (++index < regExpObject->GetSpecialEnumerablePropertyCount())
        {
            if (attributes != nullptr)
            {
                *attributes = PropertyEnumerable;
            }

            Var item;
            if (regExpObject->GetSpecialEnumerablePropertyName(index, &item, scriptContext))
            {
                return item;
            }
            return regExpObject->GetScriptContext()->GetIntegerString(index);
        }
        else
        {
            index = regExpObject->GetSpecialEnumerablePropertyCount();
            return nullptr;
        }
    }

    JavascriptRegExpObjectEnumerator::JavascriptRegExpObjectEnumerator(JavascriptRegExpConstructor* regExpObject,
        ScriptContext* scriptContext, BOOL enumNonEnumerable, bool enumSymbols) :
        JavascriptEnumerator(scriptContext),
        regExpObject(regExpObject),
        enumNonEnumerable(enumNonEnumerable),
        enumSymbols(enumSymbols)
    {
        Reset();
    }

    Var JavascriptRegExpObjectEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        Var currentIndex;
        if (regExpEnumerator != nullptr)
        {
            currentIndex = regExpEnumerator->MoveAndGetNext(propertyId, attributes);
            if (currentIndex != nullptr)
            {
                return currentIndex;
            }
            regExpEnumerator = nullptr;
        }
        if (objectEnumerator != nullptr)
        {
            currentIndex = objectEnumerator->MoveAndGetNext(propertyId, attributes);
            if (currentIndex != nullptr)
            {
                return currentIndex;
            }
            objectEnumerator = nullptr;
        }
        return nullptr;
    }

    void JavascriptRegExpObjectEnumerator::Reset()
    {
        ScriptContext* scriptContext = GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();
        regExpEnumerator = RecyclerNew(recycler, JavascriptRegExpEnumerator, regExpObject, scriptContext, enumNonEnumerable);
        Var enumerator;
        regExpObject->DynamicObject::GetEnumerator(enumNonEnumerable, &enumerator, scriptContext, false, enumSymbols);
        objectEnumerator = (JavascriptEnumerator*)enumerator;
    }
}

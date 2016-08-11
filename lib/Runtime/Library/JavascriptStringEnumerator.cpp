//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptStringEnumerator::JavascriptStringEnumerator(JavascriptString* stringObject, ScriptContext * requestContext) :
        JavascriptEnumerator(requestContext),
        stringObject(stringObject),
        index(-1)
    {
    }

    void JavascriptStringEnumerator::Reset()
    {
        index = -1;
    }


    Var JavascriptStringEnumerator::MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        propertyId = Constants::NoProperty;
        if (++index < stringObject->GetLengthAsSignedInt())
        {
            if (attributes != nullptr)
            {
                *attributes = PropertyEnumerable;
            }

            return stringObject->GetScriptContext()->GetIntegerString(index);
        }
        else
        {
            index = stringObject->GetLength();
            return nullptr;
        }
    }

    JavascriptStringObjectEnumerator::JavascriptStringObjectEnumerator(JavascriptStringObject* stringObject,
        ScriptContext* scriptContext,
        BOOL enumNonEnumerable,
        bool enumSymbols) :
        JavascriptEnumerator(scriptContext),
        stringObject(stringObject),
        enumNonEnumerable(enumNonEnumerable),
        enumSymbols(enumSymbols)
    {
        Reset();
    }

    Var JavascriptStringObjectEnumerator::MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        Var currentIndex;
        propertyId = Constants::NoProperty;
        if (stringEnumerator != nullptr)
        {
            currentIndex = stringEnumerator->MoveAndGetNext(propertyId, attributes);
            if (currentIndex != nullptr)
            {
                return currentIndex;
            }
            stringEnumerator = nullptr;
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


    void JavascriptStringObjectEnumerator::Reset()
    {
        ScriptContext* scriptContext = GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();
        stringEnumerator = RecyclerNew(recycler, JavascriptStringEnumerator, JavascriptString::FromVar(CrossSite::MarshalVar(scriptContext, stringObject->Unwrap())), scriptContext);
        Var enumerator;
        stringObject->DynamicObject::GetEnumerator(enumNonEnumerable, &enumerator, scriptContext, true, enumSymbols);
        objectEnumerator = (JavascriptEnumerator*)enumerator;
    }
}

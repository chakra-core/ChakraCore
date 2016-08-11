//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    ArgumentsObjectEnumerator::ArgumentsObjectEnumerator(ArgumentsObject* argumentsObject, ScriptContext* requestContext, BOOL enumNonEnumerable, bool enumSymbols)
        : JavascriptEnumerator(requestContext),
        argumentsObject(argumentsObject),
        enumNonEnumerable(enumNonEnumerable),
        enumSymbols(enumSymbols)
    {
        Reset();
    }

    Var ArgumentsObjectEnumerator::MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        if (!doneFormalArgs)
        {
            formalArgIndex = argumentsObject->GetNextFormalArgIndex(formalArgIndex, this->enumNonEnumerable, attributes);
            if (formalArgIndex != JavascriptArray::InvalidIndex
                && formalArgIndex < argumentsObject->GetNumberOfArguments())
            {
                propertyId = Constants::NoProperty;
                return argumentsObject->GetScriptContext()->GetIntegerString(formalArgIndex);
            }

            doneFormalArgs = true;
        }
        return objectEnumerator->MoveAndGetNext(propertyId, attributes);
    }   

    void ArgumentsObjectEnumerator::Reset()
    {
        formalArgIndex = JavascriptArray::InvalidIndex;
        doneFormalArgs = false;

        Var enumerator;
        argumentsObject->DynamicObject::GetEnumerator(enumNonEnumerable, &enumerator, GetScriptContext(), true, enumSymbols);
        objectEnumerator = (Js::JavascriptEnumerator*)enumerator;
    }

    //---------------------- ES5ArgumentsObjectEnumerator -------------------------------

    ES5ArgumentsObjectEnumerator::ES5ArgumentsObjectEnumerator(ArgumentsObject* argumentsObject, ScriptContext* requestcontext, BOOL enumNonEnumerable, bool enumSymbols)
        : ArgumentsObjectEnumerator(argumentsObject, requestcontext, enumNonEnumerable, enumSymbols),
        enumeratedFormalsInObjectArrayCount(0)
    {
        this->Reset();
    }

    Var ES5ArgumentsObjectEnumerator::MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        // Formals:
        // - deleted => not in objectArray && not connected -- do not enum, do not advance
        // - connected,     in objectArray -- if (enumerable) enum it, advance objectEnumerator
        // - disconnected =>in objectArray -- if (enumerable) enum it, advance objectEnumerator

        if (!doneFormalArgs)
        {
            ES5HeapArgumentsObject* es5HAO = static_cast<ES5HeapArgumentsObject*>(argumentsObject);
            formalArgIndex = es5HAO->GetNextFormalArgIndexHelper(formalArgIndex, this->enumNonEnumerable, attributes);
            if (formalArgIndex != JavascriptArray::InvalidIndex
                && formalArgIndex < argumentsObject->GetNumberOfArguments())
            {                
                if (argumentsObject->HasObjectArrayItem(formalArgIndex))
                {
                    PropertyId tempPropertyId;
                    Var tempIndex = objectEnumerator->MoveAndGetNext(tempPropertyId, attributes);
                    AssertMsg(tempIndex, "We advanced objectEnumerator->MoveNext() too many times.");
                }

                propertyId = Constants::NoProperty;
                return argumentsObject->GetScriptContext()->GetIntegerString(formalArgIndex);
            }

            doneFormalArgs = true;
        }

        return objectEnumerator->MoveAndGetNext(propertyId, attributes);
    }

    void ES5ArgumentsObjectEnumerator::Reset()
    {
        __super::Reset();
        this->enumeratedFormalsInObjectArrayCount = 0;
    }
}

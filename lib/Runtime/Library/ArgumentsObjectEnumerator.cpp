//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#include "Library/ArgumentsObjectEnumerator.h"

namespace Js
{
    ArgumentsObjectPrefixEnumerator::ArgumentsObjectPrefixEnumerator(ArgumentsObject* argumentsObject, EnumeratorFlags flags, ScriptContext* requestContext)
        : JavascriptEnumerator(requestContext),
        argumentsObject(argumentsObject),
        flags(flags)
    {
        Reset();
    }

    JavascriptString * ArgumentsObjectPrefixEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        if (!doneFormalArgs)
        {
            formalArgIndex = argumentsObject->GetNextFormalArgIndex(formalArgIndex, !!(flags & EnumeratorFlags::EnumNonEnumerable), attributes);
            if (formalArgIndex != JavascriptArray::InvalidIndex
                && formalArgIndex < argumentsObject->GetNumberOfArguments())
            {
                propertyId = Constants::NoProperty;
                return this->GetScriptContext()->GetIntegerString(formalArgIndex);
            }

            doneFormalArgs = true;
        }
        return nullptr;
    }

    void ArgumentsObjectPrefixEnumerator::Reset()
    {
        formalArgIndex = JavascriptArray::InvalidIndex;
        doneFormalArgs = false;
    }

    //---------------------- ES5ArgumentsObjectEnumerator -------------------------------
    ES5ArgumentsObjectEnumerator * ES5ArgumentsObjectEnumerator::New(ArgumentsObject* argumentsObject, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache)
    {
        ES5ArgumentsObjectEnumerator * enumerator = RecyclerNew(requestContext->GetRecycler(), ES5ArgumentsObjectEnumerator, argumentsObject, flags, requestContext);
        if (!enumerator->Init(forInCache))
        {
            return nullptr;
        }
        return enumerator;
    }

    ES5ArgumentsObjectEnumerator::ES5ArgumentsObjectEnumerator(ArgumentsObject* argumentsObject, EnumeratorFlags flags, ScriptContext* requestcontext)
        : ArgumentsObjectPrefixEnumerator(argumentsObject, flags, requestcontext),
        enumeratedFormalsInObjectArrayCount(0)
    {
    }

    BOOL ES5ArgumentsObjectEnumerator::Init(ForInCache * forInCache)
    {
        __super::Reset();
        this->enumeratedFormalsInObjectArrayCount = 0;
        return argumentsObject->DynamicObject::GetEnumerator(&objectEnumerator, flags, GetScriptContext(), forInCache);
    }

    JavascriptString * ES5ArgumentsObjectEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        // Formals:
        // - deleted => not in objectArray && not connected -- do not enum, do not advance
        // - connected,     in objectArray -- if (enumerable) enum it, advance objectEnumerator
        // - disconnected =>in objectArray -- if (enumerable) enum it, advance objectEnumerator

        if (!doneFormalArgs)
        {
            ES5HeapArgumentsObject* es5HAO = static_cast<ES5HeapArgumentsObject*>(
                static_cast<ArgumentsObject*>(argumentsObject));
            formalArgIndex = es5HAO->GetNextFormalArgIndexHelper(formalArgIndex, !!(flags & EnumeratorFlags::EnumNonEnumerable), attributes);
            if (formalArgIndex != JavascriptArray::InvalidIndex
                && formalArgIndex < argumentsObject->GetNumberOfArguments())
            {
                if (argumentsObject->HasObjectArrayItem(formalArgIndex))
                {
                    PropertyId tempPropertyId;
                    JavascriptString * tempIndex = objectEnumerator.MoveAndGetNext(tempPropertyId, attributes);
                    AssertMsg(tempIndex, "We advanced objectEnumerator->MoveNext() too many times.");
                }

                propertyId = Constants::NoProperty;
                return this->GetScriptContext()->GetIntegerString(formalArgIndex);
            }

            doneFormalArgs = true;
        }

        return objectEnumerator.MoveAndGetNext(propertyId, attributes);
    }

    void ES5ArgumentsObjectEnumerator::Reset()
    {
        Init(nullptr);
    }
}

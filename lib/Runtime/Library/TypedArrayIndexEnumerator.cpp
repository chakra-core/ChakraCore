//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    TypedArrayIndexEnumerator::TypedArrayIndexEnumerator(TypedArrayBase* typedArrayBase, EnumeratorFlags flags, ScriptContext* scriptContext) :
        JavascriptEnumerator(scriptContext),
        typedArrayObject(typedArrayBase),
        flags(flags)
    {
        Reset();
    }

    JavascriptString * TypedArrayIndexEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        // TypedArrayIndexEnumerator follows the same logic in JavascriptArrayEnumerator,
        // but the implementation is slightly different as we don't have sparse array
        // in typed array, and typed array is a DynamicObject instead of JavascriptArray.
        propertyId = Constants::NoProperty;
        ScriptContext *scriptContext = this->GetScriptContext();

        if (!doneArray)
        {
            while (true)
            {
                uint32 lastIndex = index;
                index++;
                if ((uint32)index >= typedArrayObject->GetLength()) // End of array
                {
                    index = lastIndex;
                    doneArray = true;
                    break;
                }

                if (attributes != nullptr)
                {
                    *attributes = PropertyEnumerable;
                }

                return scriptContext->GetIntegerString(index);
            }
        }
        return nullptr;
    }

    void TypedArrayIndexEnumerator::Reset()
    {
        index = JavascriptArray::InvalidIndex;
        doneArray = false;
    }
}

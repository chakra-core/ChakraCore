//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptArrayIndexEnumerator::JavascriptArrayIndexEnumerator(
        JavascriptArray* arrayObject, EnumeratorFlags flags, ScriptContext* scriptContext) :
        JavascriptArrayIndexEnumeratorBase(arrayObject, flags, scriptContext)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arrayObject);
#endif
        Reset();
    }

    JavascriptString * JavascriptArrayIndexEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        // TypedArrayIndexEnumerator follow the same logic but implementation is slightly
        // different as we don't have sparse array in typed array, and typed array
        // is DynamicObject instead of JavascriptArray.
        propertyId = Constants::NoProperty;

        if (!doneArray)
        {
            while (true)
            {
                uint32 lastIndex = index;
                index = arrayObject->GetNextIndex(index);
                if (index == JavascriptArray::InvalidIndex) // End of array
                {
                    index = lastIndex;
                    doneArray = true;
                    break;
                }

                if (attributes != nullptr)
                {
                    *attributes = PropertyEnumerable;
                }

                return this->GetScriptContext()->GetIntegerString(index);
            }
        }
        return nullptr;
    }

    void JavascriptArrayIndexEnumerator::Reset()
    {
        index = JavascriptArray::InvalidIndex;
        doneArray = false;
    }
}

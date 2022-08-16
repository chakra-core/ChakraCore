//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptArrayIndexSnapshotEnumerator::JavascriptArrayIndexSnapshotEnumerator(
        JavascriptArray* arrayObject, EnumeratorFlags flags, ScriptContext* scriptContext) :
        JavascriptArrayIndexEnumeratorBase(arrayObject, flags, scriptContext),
        initialLength(arrayObject->GetLength())
    {
        Reset();
    }

    JavascriptString * JavascriptArrayIndexSnapshotEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        propertyId = Constants::NoProperty;

        if (!doneArray)
        {
            uint32 lastIndex = index;
            index = arrayObject->GetNextIndex(index);
            if (index >= initialLength) // End of array
            {
                index = lastIndex;
                doneArray = true;
            }
            else
            {
                if (attributes != nullptr)
                {
                    *attributes = PropertyEnumerable;
                }

                return this->GetScriptContext()->GetIntegerString(index);
            }
        }
        return nullptr;
    }

    void JavascriptArrayIndexSnapshotEnumerator::Reset()
    {
        index = JavascriptArray::InvalidIndex;
        doneArray = false;
        initialLength = arrayObject->GetLength();
    }
}

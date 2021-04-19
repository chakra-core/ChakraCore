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


    JavascriptString * JavascriptStringEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        propertyId = Constants::NoProperty;
        if (++index < stringObject->GetLengthAsSignedInt())
        {
            if (attributes != nullptr)
            {
                *attributes = PropertyEnumerable;
            }

            return this->GetScriptContext()->GetIntegerString(index);
        }
        else
        {
            index = stringObject->GetLength();
            return nullptr;
        }
    }
}

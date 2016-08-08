//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    ES5ArrayEnumerator::ES5ArrayEnumerator(Var originalInstance, ES5Array* arrayObject, ScriptContext* scriptContext, BOOL enumNonEnumerable, bool enumSymbols)
        : originalInstance(originalInstance), JavascriptArrayEnumeratorBase(arrayObject, scriptContext, enumNonEnumerable, enumSymbols)
    {
        if (originalInstance != arrayObject)
        {
            Assert(static_cast<DynamicObject*>(originalInstance)->GetObjectArray() == arrayObject);
        }

        Reset();
    }

    Var ES5ArrayEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        propertyId = Constants::NoProperty;

        if (!doneArray)
        {
            while (true)
            {
                if (index == dataIndex)
                {
                    dataIndex = arrayObject->GetNextIndex(dataIndex);
                }
                if (index == descriptorIndex || !GetArray()->IsValidDescriptorToken(descriptorValidationToken))
                {
                    descriptorIndex = GetArray()->GetNextDescriptor(index, &descriptor, &descriptorValidationToken);
                }

                index = min(dataIndex, descriptorIndex);
                if (index >= initialLength) // End of array
                {
                    doneArray = true;
                    break;
                }

                if (enumNonEnumerable
                    || index < descriptorIndex
                    || (descriptor->Attributes & PropertyEnumerable))
                {
                    if (attributes != nullptr)
                    {
                        if (index < descriptorIndex)
                        {
                            *attributes = PropertyEnumerable;
                        }
                        else
                        {
                            *attributes = descriptor->Attributes;
                        }
                    }

                    return arrayObject->GetScriptContext()->GetIntegerString(index);
                }
            }
        }
        if (!doneObject)
        {
            Var currentIndex = objectEnumerator->MoveAndGetNext(propertyId, attributes);
            if (currentIndex)
            {
                return currentIndex;
            }
            doneObject = true;
        }
        return NULL;
    }

    void ES5ArrayEnumerator::Reset()
    {
        initialLength = arrayObject->GetLength();
        dataIndex = JavascriptArray::InvalidIndex;
        descriptorIndex = JavascriptArray::InvalidIndex;
        descriptor = nullptr;
        descriptorValidationToken = nullptr;

        index = JavascriptArray::InvalidIndex;
        doneArray = false;
        doneObject = false;

        Var enumerator;
        arrayObject->DynamicObject::GetEnumerator(enumNonEnumerable, &enumerator, GetScriptContext(), true, enumSymbols);
        objectEnumerator = (JavascriptEnumerator*)enumerator;
    }
}

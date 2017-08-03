//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    ES5ArrayIndexEnumerator::ES5ArrayIndexEnumerator(ES5Array* arrayObject, EnumeratorFlags flags, ScriptContext* scriptContext)
        : JavascriptArrayIndexEnumeratorBase(arrayObject, flags, scriptContext)
    {
        Reset();
    }

    JavascriptString * ES5ArrayIndexEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
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
                    Js::IndexPropertyDescriptor * pResultDescriptor = nullptr;
                    void* tmpDescriptorValidationToken = nullptr;
                    descriptorIndex = GetArray()->GetNextDescriptor(
                        index, &pResultDescriptor, &tmpDescriptorValidationToken);
                    this->descriptor = pResultDescriptor;
                    this->descriptorValidationToken = tmpDescriptorValidationToken;
                }

                index = min(dataIndex, descriptorIndex);
                if (index >= initialLength) // End of array
                {
                    doneArray = true;
                    break;
                }

                if (!!(flags & EnumeratorFlags::EnumNonEnumerable)
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

                    return this->GetScriptContext()->GetIntegerString(index);
                }
            }
        }
        return nullptr;
    }

    void ES5ArrayIndexEnumerator::Reset()
    {
        initialLength = arrayObject->GetLength();
        dataIndex = JavascriptArray::InvalidIndex;
        descriptorIndex = JavascriptArray::InvalidIndex;
        descriptor = nullptr;
        descriptorValidationToken = nullptr;

        index = JavascriptArray::InvalidIndex;
        doneArray = false;
    }
}

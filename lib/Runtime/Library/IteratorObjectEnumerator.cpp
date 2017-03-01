//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    IteratorObjectEnumerator * IteratorObjectEnumerator::Create(ScriptContext* scriptContext, Var iterator)
    {
        return RecyclerNew(scriptContext->GetRecycler(), IteratorObjectEnumerator, scriptContext, iterator);
    }

    IteratorObjectEnumerator::IteratorObjectEnumerator(ScriptContext* scriptContext, Var iterator) :
        JavascriptEnumerator(scriptContext),
        done(false),
        value(nullptr)
    {
        Assert(JavascriptOperators::IsObject(iterator));
        iteratorObject = RecyclableObject::FromVar(iterator);
    }

    Var IteratorObjectEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        ScriptContext* scriptContext = GetScriptContext();
        Var resultValue = nullptr;
        if (JavascriptOperators::IteratorStepAndValue(iteratorObject, scriptContext, &resultValue))
        {
            this->value = resultValue;
            if (attributes != nullptr)
            {
                *attributes = PropertyEnumerable;
            }

            Var currentIndex = value;
            const PropertyRecord* propertyRecord = nullptr;
            if (!TaggedInt::Is(currentIndex) && JavascriptString::Is(currentIndex) &&
                VirtualTableInfo<Js::PropertyString>::HasVirtualTable(JavascriptString::FromVar(currentIndex)))
            {
                propertyRecord = ((PropertyString *)PropertyString::FromVar(currentIndex))->GetPropertyRecord();
            }
            else if (JavascriptSymbol::Is(currentIndex))
            {
                propertyRecord = JavascriptSymbol::FromVar(currentIndex)->GetValue();
            }
            else
            {
                JavascriptString* propertyName = JavascriptConversion::ToString(currentIndex, scriptContext);
                GetScriptContext()->GetOrAddPropertyRecord(propertyName->GetString(), propertyName->GetLength(), &propertyRecord);

                // Need to keep property records alive during enumeration to prevent collection
                // and eventual reuse during the same enumeration. For DynamicObjects, property
                // records are kept alive by type handlers.
                this->propertyRecords.Prepend(iteratorObject->GetRecycler(), propertyRecord);
            }

            propertyId = propertyRecord->GetPropertyId();
            return currentIndex;
        }
        return NULL;
    }

    void IteratorObjectEnumerator::Reset()
    {
        Assert(FALSE);
    }
};

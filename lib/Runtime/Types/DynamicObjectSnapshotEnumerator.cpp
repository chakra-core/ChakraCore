//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    template <bool enumNonEnumerable, bool enumSymbols>
    JavascriptEnumerator* DynamicObjectSnapshotEnumerator<enumNonEnumerable, enumSymbols>::New(ScriptContext* scriptContext, DynamicObject* object)
    {
        DynamicObjectSnapshotEnumerator* enumerator = RecyclerNew(scriptContext->GetRecycler(), DynamicObjectSnapshotEnumerator, scriptContext);
        enumerator->Initialize(object);
        return enumerator;
    }

    template <bool enumNonEnumerable, bool enumSymbols>
    Var DynamicObjectSnapshotEnumerator<enumNonEnumerable, enumSymbols>::MoveAndGetNextFromArray(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        if (this->arrayEnumerator)
        {
            Var currentIndex = this->arrayEnumerator->MoveAndGetNext(propertyId, attributes);
            if(currentIndex != nullptr)
            {
                return currentIndex;
            }
            this->arrayEnumerator = nullptr;
        }

        return nullptr;
    }

    template <bool enumNonEnumerable, bool enumSymbols>
    JavascriptString * DynamicObjectSnapshotEnumerator<enumNonEnumerable, enumSymbols>::MoveAndGetNextFromObject(BigPropertyIndex& index, PropertyId& propertyId, PropertyAttributes* attributes)
    {
        JavascriptString* propertyString = nullptr;
        auto newIndex = this->objectIndex;
        do
        {
            newIndex++;
            if (!this->object->FindNextProperty(newIndex, &propertyString, &propertyId, attributes, this->GetTypeToEnumerate(), !enumNonEnumerable, /*enumSymbols*/enumSymbols) || newIndex >= initialPropertyCount)
            {
                newIndex--;
                propertyString = nullptr;
                break;
            }
        }
        while (Js::IsInternalPropertyId(propertyId));

        index = newIndex;
        return propertyString;
    }

    template <bool enumNonEnumerable, bool enumSymbols>
    Var DynamicObjectSnapshotEnumerator<enumNonEnumerable, enumSymbols>::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        Var currentIndex = MoveAndGetNextFromArray(propertyId, attributes);
        return (currentIndex != nullptr)? currentIndex :
            this->MoveAndGetNextFromObject(this->objectIndex, propertyId, attributes);
    }

    template <bool enumNonEnumerable, bool enumSymbols>
    void DynamicObjectSnapshotEnumerator<enumNonEnumerable, enumSymbols>::Reset()
    {
        __super::Reset();
        initialPropertyCount = this->object->GetPropertyCount();
    }

    template <bool enumNonEnumerable, bool enumSymbols>
    void DynamicObjectSnapshotEnumerator<enumNonEnumerable, enumSymbols>::Initialize(DynamicObject* object)
    {
        __super::Initialize(object);
        initialPropertyCount = object->GetPropertyCount();
    }

    template class DynamicObjectSnapshotEnumerator<true, true>;
    template class DynamicObjectSnapshotEnumerator<true, false>;
    template class DynamicObjectSnapshotEnumerator<false, true>;
    template class DynamicObjectSnapshotEnumerator<false, false>;
}

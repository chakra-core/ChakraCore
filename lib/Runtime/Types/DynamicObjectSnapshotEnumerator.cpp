//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    JavascriptEnumerator* DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>::New(ScriptContext* scriptContext, DynamicObject* object)
    {
        DynamicObjectSnapshotEnumerator* enumerator = RecyclerNew(scriptContext->GetRecycler(), DynamicObjectSnapshotEnumerator, scriptContext);
        enumerator->Initialize(object);
        return enumerator;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    Var DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>::MoveAndGetNextFromArray(_Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        propertyId = Constants::NoProperty;
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

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    JavascriptString * DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>::MoveAndGetNextFromObject(T& index, PropertyId& propertyId, PropertyAttributes* attributes)
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

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    Var DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>::MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        Var currentIndex = MoveAndGetNextFromArray(propertyId, attributes);
        return (currentIndex != nullptr)? currentIndex :
            this->MoveAndGetNextFromObject(this->objectIndex, propertyId, attributes);
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    void DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>::Reset()
    {
        __super::Reset();
        initialPropertyCount = this->object->GetPropertyCount();
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    void DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>::Initialize(DynamicObject* object)
    {
        __super::Initialize(object);
        initialPropertyCount = object->GetPropertyCount();
    }

    template class DynamicObjectSnapshotEnumerator<PropertyIndex, true, true>;
    template class DynamicObjectSnapshotEnumerator<PropertyIndex, true, false>;
    template class DynamicObjectSnapshotEnumerator<PropertyIndex, false, true>;
    template class DynamicObjectSnapshotEnumerator<PropertyIndex, false, false>;
    template class DynamicObjectSnapshotEnumerator<BigPropertyIndex, true, true>;
    template class DynamicObjectSnapshotEnumerator<BigPropertyIndex, true, false>;
    template class DynamicObjectSnapshotEnumerator<BigPropertyIndex, false, true>;
    template class DynamicObjectSnapshotEnumerator<BigPropertyIndex, false, false>;
}

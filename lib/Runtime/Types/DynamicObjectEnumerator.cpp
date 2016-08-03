//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    JavascriptEnumerator* DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::New(ScriptContext* scriptContext, DynamicObject* object)
    {
        DynamicObjectEnumerator* enumerator = RecyclerNew(scriptContext->GetRecycler(), DynamicObjectEnumerator, scriptContext);
        enumerator->Initialize(object);
        return enumerator;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    DynamicType *DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::GetTypeToEnumerate() const
    {
        return
            snapShotSemantics &&
            initialType->GetIsLocked() &&
            CONFIG_FLAG(TypeSnapshotEnumeration)
                ? initialType
                : object->GetDynamicType();
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    Var DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::GetCurrentIndex()
    {
        if (arrayEnumerator)
        {
            return arrayEnumerator->GetCurrentIndex();
        }

        JavascriptString* propertyString = nullptr;
        PropertyId propertyId = Constants::NoProperty;
        if (!object->FindNextProperty(objectIndex, &propertyString, &propertyId, nullptr, GetTypeToEnumerate(), !enumNonEnumerable, enumSymbols))
        {
            return this->GetLibrary()->GetUndefined();
        }

        Assert(propertyId == Constants::NoProperty || !Js::IsInternalPropertyId(propertyId));

        return propertyString;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    BOOL DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::MoveNext(PropertyAttributes* attributes)
    {
        PropertyId propId;
        return GetCurrentAndMoveNext(propId, attributes) != NULL;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    bool DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::GetCurrentPropertyId(PropertyId *pPropertyId)
    {
        if (arrayEnumerator)
        {
            return arrayEnumerator->GetCurrentPropertyId(pPropertyId);
        }
        Js::PropertyId propertyId = object->GetPropertyId((T) objectIndex);

        if ((enumNonEnumerable || (propertyId != Constants::NoProperty && object->IsEnumerable(propertyId))))
        {
            *pPropertyId = propertyId;
            return true;
        }
        else
        {
            return false;
        }
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    uint32 DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::GetCurrentItemIndex()
    {
        if (arrayEnumerator)
        {
            return arrayEnumerator->GetCurrentItemIndex();
        }
        else
        {
            return JavascriptArray::InvalidIndex;
        }
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    void DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::Reset()
    {
        ResetHelper();
    }

    // Initialize (or reuse) this enumerator for a given object.
    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    void DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::Initialize(DynamicObject* object)
    {
        this->object = object;
        ResetHelper();
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    Var DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::GetCurrentAndMoveNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        if (arrayEnumerator)
        {
            Var currentIndex = arrayEnumerator->GetCurrentAndMoveNext(propertyId, attributes);
            if(currentIndex != NULL)
            {
                return currentIndex;
            }
            arrayEnumerator = NULL;
        }

        JavascriptString* propertyString;

        do
        {
            objectIndex++;
            propertyString = nullptr;
            if (!object->FindNextProperty(objectIndex, &propertyString, &propertyId, attributes, GetTypeToEnumerate(), !enumNonEnumerable, enumSymbols))
            {
                // No more properties
                objectIndex--;
                break;
            }
        }
        while (Js::IsInternalPropertyId(propertyId));

        return propertyString;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    void DynamicObjectEnumerator<T, enumNonEnumerable, enumSymbols, snapShotSemantics>::ResetHelper()
    {
        if (object->HasObjectArray())
        {
            // Pass "object" as originalInstance to objectArray enumerator
            BOOL result = object->GetObjectArrayOrFlagsAsArray()->GetEnumerator(object, enumNonEnumerable, (Var*)&arrayEnumerator, GetScriptContext(), snapShotSemantics, enumSymbols);
            Assert(result);
        }
        else
        {
            arrayEnumerator = nullptr;
        }
        initialType = object->GetDynamicType();
        objectIndex = (T)-1; // This is Constants::NoSlot or Constants::NoBigSlot
    }

    template class DynamicObjectEnumerator<PropertyIndex, /*enumNonEnumerable*/true, /*enumSymbols*/true, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator<BigPropertyIndex, /*enumNonEnumerable*/true, /*enumSymbols*/true, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator<PropertyIndex, /*enumNonEnumerable*/false, /*enumSymbols*/true, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator<BigPropertyIndex, /*enumNonEnumerable*/false, /*enumSymbols*/true, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator<PropertyIndex, /*enumNonEnumerable*/false, /*enumSymbols*/true, /*snapShotSemantics*/true>;
    template class DynamicObjectEnumerator<BigPropertyIndex, /*enumNonEnumerable*/false, /*enumSymbols*/true, /*snapShotSemantics*/true>;
    template class DynamicObjectEnumerator<PropertyIndex, /*enumNonEnumerable*/true, /*enumSymbols*/false, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator<BigPropertyIndex, /*enumNonEnumerable*/true, /*enumSymbols*/false, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator<PropertyIndex, /*enumNonEnumerable*/false, /*enumSymbols*/false, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator<BigPropertyIndex, /*enumNonEnumerable*/false, /*enumSymbols*/false, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator<PropertyIndex, /*enumNonEnumerable*/false, /*enumSymbols*/false, /*snapShotSemantics*/true>;
    template class DynamicObjectEnumerator<BigPropertyIndex, /*enumNonEnumerable*/false, /*enumSymbols*/false, /*snapShotSemantics*/true>;
    template class DynamicObjectEnumerator<PropertyIndex, /*enumNonEnumerable*/true, /*enumSymbols*/false, /*snapShotSemantics*/true>;
    template class DynamicObjectEnumerator<BigPropertyIndex, /*enumNonEnumerable*/true, /*enumSymbols*/false, /*snapShotSemantics*/true>;
    template class DynamicObjectEnumerator<PropertyIndex, /*enumNonEnumerable*/true, /*enumSymbols*/true, /*snapShotSemantics*/true>;
    template class DynamicObjectEnumerator<BigPropertyIndex, /*enumNonEnumerable*/true, /*enumSymbols*/true, /*snapShotSemantics*/true>;
}

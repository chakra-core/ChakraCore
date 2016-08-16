//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    template <bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    JavascriptEnumerator* DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, snapShotSemantics>::New(ScriptContext* scriptContext, DynamicObject* object)
    {
        DynamicObjectEnumerator* enumerator = RecyclerNew(scriptContext->GetRecycler(), DynamicObjectEnumerator, scriptContext);
        enumerator->Initialize(object);
        return enumerator;
    }

    template <bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    DynamicType *DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, snapShotSemantics>::GetTypeToEnumerate() const
    {
        return
            snapShotSemantics &&
            initialType->GetIsLocked() &&
            CONFIG_FLAG(TypeSnapshotEnumeration)
                ? initialType
                : object->GetDynamicType();
    }

    template <bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    uint32 DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, snapShotSemantics>::GetCurrentItemIndex()
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

    template <bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    void DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, snapShotSemantics>::Reset()
    {
        ResetHelper();
    }

    // Initialize (or reuse) this enumerator for a given object.
    template <bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    void DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, snapShotSemantics>::Initialize(DynamicObject* object)
    {
        this->object = object;
        ResetHelper();
    }

    template <bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    Var DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, snapShotSemantics>::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        if (arrayEnumerator)
        {
            Var currentIndex = arrayEnumerator->MoveAndGetNext(propertyId, attributes);
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

    template <bool enumNonEnumerable, bool enumSymbols, bool snapShotSemantics>
    void DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, snapShotSemantics>::ResetHelper()
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
        objectIndex = Constants::NoBigSlot;
    }
    
    template class DynamicObjectEnumerator</*enumNonEnumerable*/true, /*enumSymbols*/true, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator</*enumNonEnumerable*/false, /*enumSymbols*/true, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator</*enumNonEnumerable*/false, /*enumSymbols*/true, /*snapShotSemantics*/true>;    
    template class DynamicObjectEnumerator</*enumNonEnumerable*/true, /*enumSymbols*/false, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator</*enumNonEnumerable*/false, /*enumSymbols*/false, /*snapShotSemantics*/false>;
    template class DynamicObjectEnumerator</*enumNonEnumerable*/false, /*enumSymbols*/false, /*snapShotSemantics*/true>;
    template class DynamicObjectEnumerator</*enumNonEnumerable*/true, /*enumSymbols*/false, /*snapShotSemantics*/true>;    
    template class DynamicObjectEnumerator</*enumNonEnumerable*/true, /*enumSymbols*/true, /*snapShotSemantics*/true>;
}

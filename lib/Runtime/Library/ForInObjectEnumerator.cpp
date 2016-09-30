//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#include "Library/ForInObjectEnumerator.h"

namespace Js
{

    ForInObjectEnumerator::ForInObjectEnumerator(RecyclableObject* object, ScriptContext * scriptContext, bool enumSymbols) :
        scriptContext(scriptContext),
        propertyIds(nullptr),
        enumSymbols(enumSymbols)
    {
        Initialize(object, scriptContext);
    }

    void ForInObjectEnumerator::Clear()
    {
        // Only clear stuff that are not useful for the next enumerator
        propertyIds = nullptr;
        newPropertyStrings.Reset();
    }

    void ForInObjectEnumerator::Initialize(RecyclableObject* currentObject, ScriptContext * scriptContext)
    {
        Assert(propertyIds == nullptr);
        Assert(newPropertyStrings.Empty());
        Assert(this->GetScriptContext() == scriptContext);

        this->currentIndex = nullptr;

        if (currentObject == nullptr)
        {
            enumerator.Clear();
            this->object = nullptr;
            this->baseObject = nullptr;
            this->baseObjectType = nullptr;
            this->firstPrototype = nullptr;
            return;
        }

        Assert(JavascriptOperators::GetTypeId(currentObject) != TypeIds_Null
            && JavascriptOperators::GetTypeId(currentObject) != TypeIds_Undefined);

        if (this->object == currentObject &&
            this->baseObjectType == currentObject->GetType())
        {
            // We can re-use the enumerator, only if the 'object' and type from the previous enumeration
            // remains the same. If the previous enumeration involved prototype enumeration
            // 'object' and 'currentEnumerator' would represent the prototype. Hence,
            // we cannot re-use it. Null objects are always equal, therefore, the enumerator cannot
            // be re-used.
            enumerator.Reset();
        }
        else
        {
            this->baseObjectType = currentObject->GetType();
            this->object = currentObject;

            InitializeCurrentEnumerator();
        }

        this->baseObject = currentObject;
        firstPrototype = GetFirstPrototypeWithEnumerableProperties(object);

        if (firstPrototype != nullptr)
        {
            Recycler *recycler = scriptContext->GetRecycler();
            propertyIds = RecyclerNew(recycler, BVSparse<Recycler>, recycler);
        }
    }

    RecyclableObject* ForInObjectEnumerator::GetFirstPrototypeWithEnumerableProperties(RecyclableObject* object)
    {
        RecyclableObject* firstPrototype = nullptr;
        if (JavascriptOperators::GetTypeId(object) != TypeIds_HostDispatch)
        {
            firstPrototype = object;
            while (true)
            {
                firstPrototype = firstPrototype->GetPrototype();

                if (JavascriptOperators::GetTypeId(firstPrototype) == TypeIds_Null)
                {
                    firstPrototype = nullptr;
                    break;
                }

                if (!DynamicType::Is(firstPrototype->GetTypeId())
                    || !DynamicObject::FromVar(firstPrototype)->GetHasNoEnumerableProperties())
                {
                    break;
                }
            }
        }

        return firstPrototype;
    }

    BOOL ForInObjectEnumerator::InitializeCurrentEnumerator()
    {
        Assert(object);
        ScriptContext* scriptContext = GetScriptContext();
        EnumeratorFlags flags = EnumeratorFlags::EnumNonEnumerable | EnumeratorFlags::SnapShotSemantics | (enumSymbols ? EnumeratorFlags::EnumSymbols : EnumeratorFlags::None);
        if (VirtualTableInfo<DynamicObject>::HasVirtualTable(object))
        {
            DynamicObject* dynamicObject = (DynamicObject*)object;
            return dynamicObject->DynamicObject::GetEnumerator(&enumerator, flags, scriptContext);
        }

        return object->GetEnumerator(&enumerator, flags, scriptContext);
    }

    Var ForInObjectEnumerator::GetCurrentIndex()
    {
        return currentIndex;
    }

    BOOL ForInObjectEnumerator::TestAndSetEnumerated(PropertyId propertyId)
    {
        Assert(propertyIds != nullptr);
        Assert(!Js::IsInternalPropertyId(propertyId));

        return !(propertyIds->TestAndSet(propertyId));
    }

    BOOL ForInObjectEnumerator::MoveNext()
    {
        PropertyId propertyId;
        currentIndex = MoveAndGetNext(propertyId);
        return currentIndex != NULL;
    }

    Var ForInObjectEnumerator::MoveAndGetNext(PropertyId& propertyId)
    {        
        PropertyRecord const * propRecord;
        PropertyAttributes attributes = PropertyNone;

        while (true)
        {
            propertyId = Constants::NoProperty;
            currentIndex = enumerator.MoveAndGetNext(propertyId, &attributes);            
            if (currentIndex)
            {
                if (firstPrototype == nullptr)
                {
                    // We are calculating correct shadowing for non-enumerable properties of the child object, we will receive
                    // both enumerable and non-enumerable properties from MoveAndGetNext so we need to check before we simply
                    // return here. If this property is non-enumerable we're going to skip it.
                    if (!(attributes & PropertyEnumerable))
                    {
                        continue;
                    }

                    // There are no prototype that has enumerable properties,
                    // don't need to keep track of the propertyIds we visited.
                    return currentIndex;
                }

                // Property Id does not exist.
                if (propertyId == Constants::NoProperty)
                {
                    if (!JavascriptString::Is(currentIndex)) //This can be undefined
                    {
                        continue;
                    }
                    JavascriptString *pString = JavascriptString::FromVar(currentIndex);
                    if (VirtualTableInfo<Js::PropertyString>::HasVirtualTable(pString))
                    {
                        // If we have a property string, it is assumed that the propertyId is being
                        // kept alive with the object
                        PropertyString * propertyString = (PropertyString *)pString;
                        propertyId = propertyString->GetPropertyRecord()->GetPropertyId();
                    }
                    else
                    {
                        ScriptContext* scriptContext = pString->GetScriptContext();
                        scriptContext->GetOrAddPropertyRecord(pString->GetString(), pString->GetLength(), &propRecord);
                        propertyId = propRecord->GetPropertyId();

                        // We keep the track of what is enumerated using a bit vector of propertyID.
                        // so the propertyId can't be collected until the end of the for in enumerator
                        // Keep a list of the property string.
                        newPropertyStrings.Prepend(GetScriptContext()->GetRecycler(), propRecord);
                    }
                }

                //check for shadowed property
                if (TestAndSetEnumerated(propertyId) //checks if the property is already enumerated or not
                    && (attributes & PropertyEnumerable))
                {
                    return currentIndex;
                }
            }
            else
            {
                if (object == baseObject)
                {
                    if (firstPrototype == nullptr)
                    {
                        return NULL;
                    }
                    object = firstPrototype;
                }
                else
                {
                    //walk the prototype chain
                    object = object->GetPrototype();
                    if ((object == NULL) || (JavascriptOperators::GetTypeId(object) == TypeIds_Null))
                    {
                        return NULL;
                    }
                }

                do
                {
                    if (!InitializeCurrentEnumerator())
                    {
                        return nullptr;
                    }

                    if (!enumerator.IsNullEnumerator())
                    {
                        break;
                    }

                     //walk the prototype chain
                    object = object->GetPrototype();
                    if ((object == NULL) || (JavascriptOperators::GetTypeId(object) == TypeIds_Null))
                    {
                        return NULL;
                    }
                }
                while (true);
            }
        }
    }

    void ForInObjectEnumerator::Reset()
    {
        object = baseObject;
        if (propertyIds)
        {
            propertyIds->ClearAll();
        }

        currentIndex = nullptr;        
        InitializeCurrentEnumerator();
    }

    BOOL ForInObjectEnumerator::CanBeReused()
    {
        return object == nullptr || (object->GetScriptContext() == GetScriptContext() && !JavascriptProxy::Is(object));
    }
}

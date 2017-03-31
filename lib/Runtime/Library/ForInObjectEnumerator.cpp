//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#include "Library/ForInObjectEnumerator.h"

namespace Js
{
    ForInObjectEnumerator::ShadowData::ShadowData(
        RecyclableObject * initObject,
        RecyclableObject * firstPrototype,
        Recycler * recycler)
        : currentObject(initObject),
          firstPrototype(firstPrototype),
          propertyIds(recycler)
    {
    }

    ForInObjectEnumerator::ForInObjectEnumerator(RecyclableObject* object, ScriptContext * scriptContext, bool enumSymbols)
    {
        Initialize(object, scriptContext, enumSymbols);
    }

    void ForInObjectEnumerator::Clear()
    {
        // Only clear stuff that are not useful for the next enumerator
        shadowData = nullptr;
    }

    void ForInObjectEnumerator::Initialize(RecyclableObject* initObject, ScriptContext * requestContext, bool enumSymbols, ForInCache * forInCache)
    {
        this->enumeratingPrototype = false;

        if (initObject == nullptr)
        {
            enumerator.Clear(EnumeratorFlags::None, requestContext);
            this->shadowData = nullptr;
            this->canUseJitFastPath = false;
            return;
        }

        Assert(JavascriptOperators::GetTypeId(initObject) != TypeIds_Null
            && JavascriptOperators::GetTypeId(initObject) != TypeIds_Undefined);

        EnumeratorFlags flags;
        RecyclableObject * firstPrototype = nullptr;
        RecyclableObject * firstPrototypeWithEnumerableProperties = GetFirstPrototypeWithEnumerableProperties(initObject, &firstPrototype);
        if (firstPrototypeWithEnumerableProperties != nullptr)
        {
            Recycler *recycler = requestContext->GetRecycler();
            this->shadowData = RecyclerNew(recycler, ShadowData, initObject, firstPrototype, recycler);
            flags = EnumeratorFlags::UseCache | EnumeratorFlags::SnapShotSemantics | EnumeratorFlags::EnumNonEnumerable | (enumSymbols ? EnumeratorFlags::EnumSymbols : EnumeratorFlags::None);
        }
        // no enumerable properties in the prototype chain, no need to search it
        else
        {
            this->shadowData = nullptr;
            flags = EnumeratorFlags::UseCache | EnumeratorFlags::SnapShotSemantics | (enumSymbols ? EnumeratorFlags::EnumSymbols : EnumeratorFlags::None);
        }

        if (InitializeCurrentEnumerator(initObject, flags, requestContext, forInCache))
        {
            canUseJitFastPath = this->enumerator.CanUseJITFastPath();
        }
        else
        {
            // Nothing to enumerate.
            // We keep the shadowData so that it may walk up the prototype chain (e.g. primitive type)
            enumerator.Clear(flags, requestContext);
            canUseJitFastPath = false;
        }
    }

    RecyclableObject* ForInObjectEnumerator::GetFirstPrototypeWithEnumerableProperties(RecyclableObject* object, RecyclableObject** pFirstPrototype)
    {
        RecyclableObject* firstPrototype = nullptr;
        RecyclableObject* firstPrototypeWithEnumerableProperties = nullptr;

        if (JavascriptOperators::GetTypeId(object) != TypeIds_HostDispatch)
        {
            firstPrototypeWithEnumerableProperties = object;
            while (true)
            {
                firstPrototypeWithEnumerableProperties = firstPrototypeWithEnumerableProperties->GetPrototype();

                if (firstPrototypeWithEnumerableProperties == nullptr)
                {
                    break;
                }

                if (JavascriptOperators::GetTypeId(firstPrototypeWithEnumerableProperties) == TypeIds_Null)
                {
                    firstPrototypeWithEnumerableProperties = nullptr;
                    break;
                }

                if (firstPrototype == nullptr)
                {
                    firstPrototype = firstPrototypeWithEnumerableProperties;
                }

                if (!DynamicType::Is(firstPrototypeWithEnumerableProperties->GetTypeId())
                    || !DynamicObject::FromVar(firstPrototypeWithEnumerableProperties)->GetHasNoEnumerableProperties())
                {
                    break;
                }
            }
        }

        if (pFirstPrototype != nullptr)
        {
            *pFirstPrototype = firstPrototype;
        }

        return firstPrototypeWithEnumerableProperties;
    }

    BOOL ForInObjectEnumerator::InitializeCurrentEnumerator(RecyclableObject * object, ForInCache * forInCache)
    {
        EnumeratorFlags flags = enumerator.GetFlags();
        RecyclableObject * prototype = object->GetPrototype();
        if (prototype == nullptr || prototype->GetTypeId() == TypeIds_Null)
        {
            // If this is the last object on the prototype chain, we don't need to get the non-enumerable properties any more to track shadowing
            flags &= ~EnumeratorFlags::EnumNonEnumerable;
        }
        return InitializeCurrentEnumerator(object, flags, GetScriptContext(), forInCache);
    }

    BOOL ForInObjectEnumerator::InitializeCurrentEnumerator(RecyclableObject * object, EnumeratorFlags flags,  ScriptContext * scriptContext, ForInCache * forInCache)
    {
        Assert(object);
        Assert(scriptContext);

        if (VirtualTableInfo<DynamicObject>::HasVirtualTable(object))
        {
            DynamicObject* dynamicObject = (DynamicObject*)object;
            return dynamicObject->DynamicObject::GetEnumerator(&enumerator, flags, scriptContext, forInCache);
        }

        return object->GetEnumerator(&enumerator, flags, scriptContext, forInCache);
    }

    BOOL ForInObjectEnumerator::TestAndSetEnumerated(PropertyId propertyId)
    {
        Assert(this->shadowData != nullptr);
        Assert(!Js::IsInternalPropertyId(propertyId));

        return !(this->shadowData->propertyIds.TestAndSet(propertyId));
    }

    JavascriptString * ForInObjectEnumerator::MoveAndGetNext(PropertyId& propertyId)
    {        
        PropertyRecord const * propRecord;
        PropertyAttributes attributes = PropertyNone;

        while (true)
        {
            propertyId = Constants::NoProperty;
            JavascriptString * currentIndex = enumerator.MoveAndGetNext(propertyId, &attributes);

            // The object type may have changed and we may not be able to use Jit fast path anymore.
            // canUseJitFastPath is determined in ForInObjectEnumerator::Initialize, once we decide we can't use
            // Jit fast path we will never go back to use fast path so && with current value  - if it's already
            // false we don't call CanUseJITFastPath()

            this->canUseJitFastPath = this->canUseJitFastPath && enumerator.CanUseJITFastPath();

            if (currentIndex)
            {
                if (this->shadowData == nullptr)
                {
                    // There are no prototype that has enumerable properties,
                    // don't need to keep track of the propertyIds we visited.

                    // We have asked for enumerable properties only, so don't need to check the attribute returned.
                    Assert(attributes & PropertyEnumerable);

                    return currentIndex;
                }

                // Property Id does not exist.
                if (propertyId == Constants::NoProperty)
                {
                    if (VirtualTableInfo<Js::PropertyString>::HasVirtualTable(currentIndex))
                    {
                        // If we have a property string, it is assumed that the propertyId is being
                        // kept alive with the object
                        PropertyString * propertyString = (PropertyString *)currentIndex;
                        propertyId = propertyString->GetPropertyRecord()->GetPropertyId();
                    }
                    else
                    {
                        ScriptContext* scriptContext = currentIndex->GetScriptContext();
                        scriptContext->GetOrAddPropertyRecord(currentIndex->GetString(), currentIndex->GetLength(), &propRecord);
                        propertyId = propRecord->GetPropertyId();

                        // We keep the track of what is enumerated using a bit vector of propertyID.
                        // so the propertyId can't be collected until the end of the for in enumerator
                        // Keep a list of the property string.
                        this->shadowData->newPropertyStrings.Prepend(GetScriptContext()->GetRecycler(), propRecord);
                    }
                }

                if (TestAndSetEnumerated(propertyId) //checks if the property is already enumerated or not
                    && (attributes & PropertyEnumerable))
                {
                    return currentIndex;
                }
            }
            else
            {
                if (this->shadowData == nullptr)
                {
                    Assert(!this->enumeratingPrototype);
                    return nullptr;
                }

                RecyclableObject * object;
                if (!this->enumeratingPrototype)
                {  
                    this->enumeratingPrototype = true;
                    object = this->shadowData->firstPrototype;
                    this->shadowData->currentObject = object;
                }
                else
                {
                    //walk the prototype chain
                    object = this->shadowData->currentObject->GetPrototype();
                    this->shadowData->currentObject = object;
                    if ((object == nullptr) || (JavascriptOperators::GetTypeId(object) == TypeIds_Null))
                    {
                        return nullptr;
                    }
                }

                do
                {
                    if (!InitializeCurrentEnumerator(object))
                    {
                        return nullptr;
                    }

                    if (!enumerator.IsNullEnumerator())
                    {
                        break;
                    }

                     //walk the prototype chain
                    object = object->GetPrototype();
                    this->shadowData->currentObject = object;
                    if ((object == nullptr) || (JavascriptOperators::GetTypeId(object) == TypeIds_Null))
                    {
                        return nullptr;
                    }
                }
                while (true);
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    JavascriptEnumerator* DynamicObjectSnapshotEnumeratorWPCache<T, enumNonEnumerable, enumSymbols>::New(ScriptContext* scriptContext, DynamicObject* object)
    {
        DynamicObjectSnapshotEnumeratorWPCache* enumerator = RecyclerNew(scriptContext->GetRecycler(), DynamicObjectSnapshotEnumeratorWPCache, scriptContext);
        enumerator->Initialize(object, false);
        return enumerator;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    void DynamicObjectSnapshotEnumeratorWPCache<T, enumNonEnumerable, enumSymbols>::Initialize(DynamicObject* object, bool allowUnlockedType/*= false*/)
    {
        __super::Initialize(object);

        enumeratedCount = 0;

        if (!allowUnlockedType)
        {
            Assert(this->initialType->GetIsLocked());
        }
        else if (this->initialType->GetIsLocked())
        {
            VirtualTableInfo<DynamicObjectSnapshotEnumeratorWPCache>::SetVirtualTable(this); // Fix vtable which could have been downgraded
        }
        else
        {
            VirtualTableInfo<DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>>::SetVirtualTable(this); // Downgrade to normal snapshot enumerator
            return;
        }

        ScriptContext* scriptContext = this->GetScriptContext();
        ThreadContext * threadContext = scriptContext->GetThreadContext();
        CachedData * data = (CachedData *)threadContext->GetDynamicObjectEnumeratorCache(this->initialType);

        if (data == nullptr || data->enumNonEnumerable != enumNonEnumerable || data->enumSymbols != enumSymbols)
        {
            data = RecyclerNewStructPlus(scriptContext->GetRecycler(),
                this->initialPropertyCount * sizeof(PropertyString *) + this->initialPropertyCount * sizeof(T) + this->initialPropertyCount * sizeof(PropertyAttributes), CachedData);
            data->cachedCount = 0;
            data->strings = (PropertyString **)(data + 1);
            data->indexes = (T *)(data->strings + this->initialPropertyCount);
            data->attributes = (PropertyAttributes*)(data->indexes + this->initialPropertyCount);
            data->completed = false;
            data->enumNonEnumerable = enumNonEnumerable;
            data->enumSymbols = enumSymbols;
            threadContext->AddDynamicObjectEnumeratorCache(this->initialType, data);
        }
        this->cachedData = data;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    JavascriptString *
        DynamicObjectSnapshotEnumeratorWPCache<T, enumNonEnumerable, enumSymbols>::MoveAndGetNextFromObjectWPCache(T& index, _Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        propertyId = Constants::NoProperty;
        if (this->initialType != this->object->GetDynamicType())
        {
            if (this->IsCrossSiteEnumerator())
            {
                // downgrade back to the normal snapshot enumerator
                VirtualTableInfo<CrossSiteEnumerator<DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>>>::SetVirtualTable(this);
            }
            else
            {
                // downgrade back to the normal snapshot enumerator
                VirtualTableInfo<DynamicObjectSnapshotEnumerator<T, enumNonEnumerable, enumSymbols>>::SetVirtualTable(this);
            }
            return this->MoveAndGetNextFromObject(this->objectIndex, propertyId, attributes);
        }
        Assert(enumeratedCount <= cachedData->cachedCount);
        JavascriptString* propertyStringName;
        PropertyAttributes propertyAttributes = PropertyNone;
        if (enumeratedCount < cachedData->cachedCount)
        {
            PropertyString * propertyString = cachedData->strings[enumeratedCount];
            propertyStringName = propertyString;
            propertyId = propertyString->GetPropertyRecord()->GetPropertyId();

#if ENABLE_TTD
            //
            //TODO: We have code in MoveAndGetNextFromObject to record replay the order in which properties are enumerated. 
            //      Since caching may happen differently at record/replay time we need to force this to ensure the log/order is consistent.
            //      Later we may want to optimize by lifting the TTD code from the call and explicitly calling it here (but not the rest of the enumeration work).
            //
            Js::ScriptContext* actionCtx = this->object->GetScriptContext();
            if(actionCtx->ShouldPerformRecordAction() | actionCtx->ShouldPerformDebugAction())
            {
                PropertyId tempPropertyId;
                /* JavascriptString * tempPropertyString = */ this->MoveAndGetNextFromObject(this->objectIndex, tempPropertyId, attributes);

                Assert(tempPropertyId == propertyId);
                Assert(this->objectIndex == cachedData->indexes[enumeratedCount]);
            }
#elif DBG
            PropertyId tempPropertyId;
            /* JavascriptString * tempPropertyString = */ this->MoveAndGetNextFromObject(this->objectIndex, tempPropertyId, attributes);

            Assert(tempPropertyId == propertyId);
            Assert(this->objectIndex == cachedData->indexes[enumeratedCount]);
#endif

            this->objectIndex = cachedData->indexes[enumeratedCount];
            propertyAttributes = cachedData->attributes[enumeratedCount];

            enumeratedCount++;
        }
        else if (!cachedData->completed)
        {
            propertyStringName = this->MoveAndGetNextFromObject(this->objectIndex, propertyId, &propertyAttributes);

            if (propertyStringName && VirtualTableInfo<PropertyString>::HasVirtualTable(propertyStringName))
            {
                Assert(enumeratedCount < this->initialPropertyCount);
                cachedData->strings[enumeratedCount] = (PropertyString*)propertyStringName;
                cachedData->indexes[enumeratedCount] = this->objectIndex;
                cachedData->attributes[enumeratedCount] = propertyAttributes;
                cachedData->cachedCount = ++enumeratedCount;
            }
            else
            {
                cachedData->completed = true;
            }
        }
        else
        {
#if ENABLE_TTD
            //
            //TODO: We have code in MoveAndGetNextFromObject to record replay the order in which properties are enumerated. 
            //      Since caching may happen differently at record/replay time we need to force this to ensure the log/order is consistent.
            //      Later we may want to optimize by lifting the TTD code from the call and explicitly calling it here (but not the rest of the enumeration work).
            //
            Js::ScriptContext* actionCtx = this->object->GetScriptContext();
            if(actionCtx->ShouldPerformRecordAction() | actionCtx->ShouldPerformDebugAction())
            {
                PropertyId tempPropertyId;
                /*JavascriptString* tempPropertyStringName =*/ this->MoveAndGetNextFromObject(this->objectIndex, tempPropertyId, attributes);
            }
#elif DBG
            PropertyId tempPropertyId;
            Assert(this->MoveAndGetNextFromObject(this->objectIndex, tempPropertyId, attributes) == nullptr);
#endif

            propertyStringName = nullptr;
        }

        if (attributes != nullptr)
        {
            *attributes = propertyAttributes;
        }

        return propertyStringName;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    Var DynamicObjectSnapshotEnumeratorWPCache<T, enumNonEnumerable, enumSymbols>::MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        Var currentIndex = this->MoveAndGetNextFromArray(propertyId, attributes);

        if (currentIndex == nullptr)
        {
            currentIndex = this->MoveAndGetNextFromObjectWPCache(this->objectIndex, propertyId, attributes);
        }

        return currentIndex;
    }

    template <typename T, bool enumNonEnumerable, bool enumSymbols>
    void DynamicObjectSnapshotEnumeratorWPCache<T, enumNonEnumerable, enumSymbols>::Reset()
    {
        // If we are reusing the enumerator the object type should be the same
        Assert(this->object->GetDynamicType() == this->initialType);
        Assert(this->initialPropertyCount == this->object->GetPropertyCount());

        __super::Reset();

        this->enumeratedCount = 0;
    }

    template class DynamicObjectSnapshotEnumeratorWPCache<PropertyIndex, true, true>;
    template class DynamicObjectSnapshotEnumeratorWPCache<PropertyIndex, true, false>;
    template class DynamicObjectSnapshotEnumeratorWPCache<PropertyIndex, false, true>;
    template class DynamicObjectSnapshotEnumeratorWPCache<PropertyIndex, false, false>;
    template class DynamicObjectSnapshotEnumeratorWPCache<BigPropertyIndex, true, true>;
    template class DynamicObjectSnapshotEnumeratorWPCache<BigPropertyIndex, true, false>;
    template class DynamicObjectSnapshotEnumeratorWPCache<BigPropertyIndex, false, true>;
    template class DynamicObjectSnapshotEnumeratorWPCache<BigPropertyIndex, false, false>;
};

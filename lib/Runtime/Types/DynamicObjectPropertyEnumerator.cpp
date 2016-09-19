//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    bool DynamicObjectPropertyEnumerator::GetEnumNonEnumerable() const 
    { 
        return !!(flags & EnumeratorFlags::EnumNonEnumerable); 
    }
    bool DynamicObjectPropertyEnumerator::GetEnumSymbols() const 
    { 
        return !!(flags & EnumeratorFlags::EnumSymbols); 
    }
    bool DynamicObjectPropertyEnumerator::GetSnapShotSemantics() const 
    { 
        return !!(flags & EnumeratorFlags::SnapShotSemantics); 
    }

    bool DynamicObjectPropertyEnumerator::GetUseCache() const
    {
        return ((flags & (EnumeratorFlags::SnapShotSemantics | EnumeratorFlags::UseCache)) == (EnumeratorFlags::SnapShotSemantics | EnumeratorFlags::UseCache));
    }

    void DynamicObjectPropertyEnumerator::Initialize(DynamicType * type, CachedData * data, Js::BigPropertyIndex initialPropertyCount)
    {
        this->initialType = type;
        this->cachedData = data;
        this->initialPropertyCount = initialPropertyCount;
    }

    bool DynamicObjectPropertyEnumerator::Initialize(DynamicObject * object, EnumeratorFlags flags, ScriptContext * requestContext, ForInCache * forInCache)
    {
        this->scriptContext = requestContext;
        this->object = object;
        this->flags = flags;

        if (!object)
        {
            this->cachedData = nullptr;
            return true;
        }

        this->objectIndex = Constants::NoBigSlot;
        this->enumeratedCount = 0;

        if (!GetUseCache())
        {
            if (!object->GetDynamicType()->GetTypeHandler()->EnsureObjectReady(object))
            {
                return false;
            }            
            Initialize(object->GetDynamicType(), nullptr, GetSnapShotSemantics() ? this->object->GetPropertyCount() : Constants::NoBigSlot);
            return true;
        }

        DynamicType * type = object->GetDynamicType();

        CachedData * data;
        if (forInCache && type == forInCache->type)
        {
            // We shouldn't have a for in cache when asking to enum symbols
            Assert(!GetEnumSymbols());            
            data = (CachedData *)forInCache->data;

            Assert(data != nullptr);
            Assert(data->scriptContext == this->scriptContext); // The cache data script context should be the same as request context
            Assert(!data->enumSymbols);

            if (data->enumNonEnumerable == GetEnumNonEnumerable())
            {
                Initialize(type, data, data->propertyCount);
                return true;
            }
        }
      
        data = (CachedData *)requestContext->GetThreadContext()->GetDynamicObjectEnumeratorCache(type);

        if (data != nullptr && data->scriptContext == this->scriptContext && data->enumNonEnumerable == GetEnumNonEnumerable() && data->enumSymbols == GetEnumSymbols())
        {
            Initialize(type, data, data->propertyCount);

            if (forInCache)
            {
                forInCache->type = type;
                forInCache->data = data;
            }
            return true;
        }

        if (!object->GetDynamicType()->GetTypeHandler()->EnsureObjectReady(object))
        {
            return false;
        }

        // Reload the type after EnsureObjecteReady
        type = object->GetDynamicType();
        if (!type->PrepareForTypeSnapshotEnumeration())
        {
            Initialize(type, nullptr, object->GetPropertyCount());
            return true;
        }

        uint propertyCount = this->object->GetPropertyCount();
        data = RecyclerNewStructPlus(requestContext->GetRecycler(),
            propertyCount * sizeof(PropertyString *) + propertyCount * sizeof(BigPropertyIndex) + propertyCount * sizeof(PropertyAttributes), CachedData);
        data->scriptContext = requestContext;
        data->cachedCount = 0;
        data->propertyCount = propertyCount;
        data->strings = (PropertyString **)(data + 1);
        data->indexes = (BigPropertyIndex *)(data->strings + propertyCount);
        data->attributes = (PropertyAttributes*)(data->indexes + propertyCount);
        data->completed = false;
        data->enumNonEnumerable = GetEnumNonEnumerable();
        data->enumSymbols = GetEnumSymbols();
        requestContext->GetThreadContext()->AddDynamicObjectEnumeratorCache(type, data);
        Initialize(type, data, propertyCount);

        if (forInCache)
        {
            forInCache->type = type;
            forInCache->data = data;
        }
        return true;
    }

    bool DynamicObjectPropertyEnumerator::IsNullEnumerator() const
    {
        return this->object == nullptr;
    }

    bool DynamicObjectPropertyEnumerator::CanUseJITFastPath() const
    {
        return !this->IsNullEnumerator() && !GetEnumNonEnumerable() && this->cachedData != nullptr;
    }

    void DynamicObjectPropertyEnumerator::Clear(EnumeratorFlags flags, ScriptContext * requestContext)
    {
        Initialize(nullptr, flags, requestContext, nullptr);
    }

    void DynamicObjectPropertyEnumerator::Reset()
    {
        Initialize(object, flags, scriptContext, nullptr);
    }

    DynamicType * DynamicObjectPropertyEnumerator::GetTypeToEnumerate() const
    {
        return
            GetSnapShotSemantics() &&
            initialType->GetIsLocked() &&
            CONFIG_FLAG(TypeSnapshotEnumeration)
            ? initialType
            : object->GetDynamicType();
    }

    JavascriptString * DynamicObjectPropertyEnumerator::MoveAndGetNextWithCache(PropertyId& propertyId, PropertyAttributes* attributes)
    {
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
            if (actionCtx->ShouldPerformRecordAction() | actionCtx->ShouldPerformDebugAction())
            {
                PropertyId tempPropertyId;
                /* JavascriptString * tempPropertyString = */ this->MoveAndGetNextNoCache(tempPropertyId, attributes);

                Assert(tempPropertyId == propertyId);
                Assert(this->objectIndex == cachedData->indexes[enumeratedCount]);
            }
#elif DBG
            PropertyId tempPropertyId;
            /* JavascriptString * tempPropertyString = */ this->MoveAndGetNextNoCache(tempPropertyId, attributes);

            Assert(tempPropertyId == propertyId);
            Assert(this->objectIndex == cachedData->indexes[enumeratedCount]);
#endif

            this->objectIndex = cachedData->indexes[enumeratedCount];
            propertyAttributes = cachedData->attributes[enumeratedCount];

            enumeratedCount++;
        }
        else if (!cachedData->completed)
        {
            propertyStringName = this->MoveAndGetNextNoCache(propertyId, &propertyAttributes);

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
            if (actionCtx->ShouldPerformRecordAction() | actionCtx->ShouldPerformDebugAction())
            {
                PropertyId tempPropertyId;
                /*JavascriptString* tempPropertyStringName =*/ this->MoveAndGetNextNoCache(tempPropertyId, attributes);
            }
#elif DBG
            PropertyId tempPropertyId;
            Assert(this->MoveAndGetNextNoCache(tempPropertyId, attributes) == nullptr);
#endif

            propertyStringName = nullptr;
        }

        if (attributes != nullptr)
        {
            *attributes = propertyAttributes;
        }

        return propertyStringName;
    }
    JavascriptString * DynamicObjectPropertyEnumerator::MoveAndGetNextNoCache(PropertyId& propertyId, PropertyAttributes * attributes)
    {
        JavascriptString* propertyString = nullptr;

        BigPropertyIndex newIndex = this->objectIndex;
        do
        {
            newIndex++;
            if (!object->FindNextProperty(newIndex, &propertyString, &propertyId, attributes,
                GetTypeToEnumerate(), flags, this->scriptContext)
                || (GetSnapShotSemantics() && newIndex >= initialPropertyCount))
            {
                // No more properties
                newIndex--;
                propertyString = nullptr;
                break;
            }
        } while (Js::IsInternalPropertyId(propertyId));

        this->objectIndex = newIndex;
        return propertyString;
    }

    Var DynamicObjectPropertyEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes * attributes)
    {
        if (this->cachedData && this->initialType == this->object->GetDynamicType())
        {
            return MoveAndGetNextWithCache(propertyId, attributes);
        }
        if (this->object)
        {
            return MoveAndGetNextNoCache(propertyId, attributes);
        }
        return nullptr;
    }
}
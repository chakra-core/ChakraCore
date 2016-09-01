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

    bool DynamicObjectPropertyEnumerator::Initialize(DynamicObject * object, EnumeratorFlags flags, ScriptContext * requestContext)
    {
        if (object && !object->GetDynamicType()->GetTypeHandler()->EnsureObjectReady(object))
        {
            return false;
        }

        this->requestContext = requestContext;
        this->object = object;
        this->flags = flags;
        Reset();
        return true;
    }

    bool DynamicObjectPropertyEnumerator::IsNullEnumerator() const
    {
        return this->object == nullptr;
    }

    void DynamicObjectPropertyEnumerator::Clear()
    {
        this->object = nullptr;
    }

    void DynamicObjectPropertyEnumerator::Reset()
    {
        if (this->object)
        {
            enumeratedCount = 0;
            initialType = object->GetDynamicType();
            objectIndex = Constants::NoBigSlot;
            initialPropertyCount = GetSnapShotSemantics() ? this->object->GetPropertyCount() : Constants::NoBigSlot;
            // Create the appropriate enumerator object.
            if (GetSnapShotSemantics() && this->initialType->PrepareForTypeSnapshotEnumeration())
            {
                ScriptContext* scriptContext = this->object->GetScriptContext();
                ThreadContext * threadContext = scriptContext->GetThreadContext();
                CachedData * data = (CachedData *)threadContext->GetDynamicObjectEnumeratorCache(this->initialType);

                if (data == nullptr || data->scriptContext != this->requestContext || data->enumNonEnumerable != GetEnumNonEnumerable() || data->enumSymbols != GetEnumSymbols())
                {
                    data = RecyclerNewStructPlus(scriptContext->GetRecycler(),
                        this->initialPropertyCount * sizeof(PropertyString *) + this->initialPropertyCount * sizeof(BigPropertyIndex) + this->initialPropertyCount * sizeof(PropertyAttributes), CachedData);
                    data->scriptContext = requestContext;
                    data->cachedCount = 0;
                    data->strings = (PropertyString **)(data + 1);
                    data->indexes = (BigPropertyIndex *)(data->strings + this->initialPropertyCount);
                    data->attributes = (PropertyAttributes*)(data->indexes + this->initialPropertyCount);
                    data->completed = false;
                    data->enumNonEnumerable = GetEnumNonEnumerable();
                    data->enumSymbols = GetEnumSymbols();
                    threadContext->AddDynamicObjectEnumeratorCache(this->initialType, data);
                }
                this->cachedData = data;
                this->cachedDataType = this->initialType;
            }
            else
            {
                this->cachedData = nullptr;
                this->cachedDataType = nullptr;
            }
        }
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
                GetTypeToEnumerate(), flags, this->requestContext)
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
        if (this->object)
        {
            if (this->cachedDataType == this->object->GetDynamicType())
            {
                return MoveAndGetNextWithCache(propertyId, attributes);
            }
            return MoveAndGetNextNoCache(propertyId, attributes);
        }
        return nullptr;
    }
}
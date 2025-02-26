//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

#include "Types/NullTypeHandler.h"
#include "Types/SimpleTypeHandler.h"

namespace Js
{
    template<size_t size>
    SimpleTypeHandler<size>::SimpleTypeHandler(SimpleTypeHandler<size> * typeHandler, bool unused)
        : DynamicTypeHandler(sizeof(descriptors) / sizeof(SimplePropertyDescriptor),
            typeHandler->GetInlineSlotCapacity(), typeHandler->GetOffsetOfInlineSlots()), propertyCount(typeHandler->propertyCount)
    {
        Assert(typeHandler->GetIsInlineSlotCapacityLocked());
        SetIsInlineSlotCapacityLocked();
        for (int i = 0; i < propertyCount; i++)
        {
            descriptors[i] = typeHandler->descriptors[i];
        }
    }

    template<size_t size>
    SimpleTypeHandler<size>::SimpleTypeHandler(SimpleTypeHandler<size> * typeHandler) :
        DynamicTypeHandler(typeHandler)
    {
        Assert(typeHandler->GetIsInlineSlotCapacityLocked());
        SetIsInlineSlotCapacityLocked();
        for (int i = 0; i < propertyCount; i++)
        {
            descriptors[i] = typeHandler->descriptors[i];
        }
    }

    template<size_t size>
    SimpleTypeHandler<size>::SimpleTypeHandler(Recycler*) :
         DynamicTypeHandler(sizeof(descriptors) / sizeof(SimplePropertyDescriptor)),
         propertyCount(0)
    {
        SetIsInlineSlotCapacityLocked();
    }

    template<size_t size>
    SimpleTypeHandler<size>::SimpleTypeHandler(NO_WRITE_BARRIER_TAG_TYPE(const PropertyRecord* id), PropertyAttributes attributes, PropertyTypes propertyTypes, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots) :
        DynamicTypeHandler(sizeof(descriptors) / sizeof(SimplePropertyDescriptor),
        inlineSlotCapacity, offsetOfInlineSlots, DefaultFlags | IsLockedFlag | MayBecomeSharedFlag | IsSharedFlag), propertyCount(1)
    {
        Assert((attributes & PropertyDeleted) == 0);
        NoWriteBarrierSet(descriptors[0].Id, id); // Used to init from global static BuiltInPropertyId
        descriptors[0].Attributes = attributes;

        Assert((propertyTypes & (PropertyTypesAll & ~(PropertyTypesWritableDataOnly | PropertyTypesHasSpecialProperties))) == 0);
        SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesHasSpecialProperties, propertyTypes);
        SetIsInlineSlotCapacityLocked();
    }

    template<size_t size>
    SimpleTypeHandler<size>::SimpleTypeHandler(NO_WRITE_BARRIER_TAG_TYPE(SimplePropertyDescriptor const (&SharedFunctionPropertyDescriptors)[size]), PropertyTypes propertyTypes, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots) :
         DynamicTypeHandler(sizeof(descriptors) / sizeof(SimplePropertyDescriptor),
         inlineSlotCapacity, offsetOfInlineSlots, DefaultFlags | IsLockedFlag | MayBecomeSharedFlag | IsSharedFlag), propertyCount(size)
    {
        for (size_t i = 0; i < size; i++)
        {
            Assert((SharedFunctionPropertyDescriptors[i].Attributes & PropertyDeleted) == 0);
             // Used to init from global static BuiltInPropertyId
            NoWriteBarrierSet(descriptors[i].Id, SharedFunctionPropertyDescriptors[i].Id);
            descriptors[i].Attributes = SharedFunctionPropertyDescriptors[i].Attributes;
        }
        Assert((propertyTypes & (PropertyTypesAll & ~(PropertyTypesWritableDataOnly | PropertyTypesHasSpecialProperties))) == 0);
        SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesHasSpecialProperties, propertyTypes);
        SetIsInlineSlotCapacityLocked();
    }

    template<size_t size>
    DynamicTypeHandler * SimpleTypeHandler<size>::Clone(Recycler * recycler)
    {
        return RecyclerNew(recycler, SimpleTypeHandler<size>, this);
    }

    template<size_t size>
    bool SimpleTypeHandler<size>::DoConvertToPathType(DynamicType* type)
    {
        if ((PHASE_ON1(ShareCrossSiteFuncTypesPhase) && CrossSite::IsThunk(type->GetEntryPoint())) || type->GetTypeHandler()->GetIsPrototype())
        {
            return false;
        }

        if (PHASE_OFF1(ShareFuncTypesPhase))
        {
            if (type->GetTypeId() == TypeIds_Function)
            {
                return false;
            }
        }

        if (HasDeletedProperties())
        {
            return false;
        }

        return true;
    }

    template<size_t size>
    SimpleTypeHandler<size> * SimpleTypeHandler<size>::ConvertToNonSharedSimpleType(DynamicObject* instance)
    {
        ScriptContext* scriptContext = instance->GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();


        CompileAssert(_countof(descriptors) == size);

        SimpleTypeHandler * newTypeHandler = RecyclerNew(recycler, SimpleTypeHandler, this, true /*unused*/);

        // Consider: Add support for fixed fields to SimpleTypeHandler when
        // non-shared.  Here we could set the instance as the singleton instance on the newly
        // created handler.

        newTypeHandler->SetFlags(IsPrototypeFlag | HasKnownSlot0Flag, this->GetFlags());
        Assert(newTypeHandler->GetIsInlineSlotCapacityLocked());
        newTypeHandler->SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, this->GetPropertyTypes());
        newTypeHandler->SetInstanceTypeHandler(instance);

        return newTypeHandler;
    }

    template<size_t size>
    template <typename T>
    T* SimpleTypeHandler<size>::ConvertToTypeHandler(DynamicObject* instance)
    {
        ScriptContext* scriptContext = instance->GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();

#if ENABLE_FIXED_FIELDS && DBG
        DynamicType* oldType = instance->GetDynamicType();
#endif

        T* newTypeHandler = RecyclerNew(recycler, T, recycler, SimpleTypeHandler<size>::GetSlotCapacity(), GetInlineSlotCapacity(), GetOffsetOfInlineSlots());
#if ENABLE_FIXED_FIELDS
        Assert(HasSingletonInstanceOnlyIfNeeded());

        bool const hasSingletonInstance = newTypeHandler->SetSingletonInstanceIfNeeded(instance);
        // If instance has a shared type, the type handler change below will induce a type transition, which
        // guarantees that any existing fast path field stores (which could quietly overwrite a fixed field
        // on this instance) will be invalidated.  It is safe to mark all fields as fixed.
        bool const allowFixedFields = hasSingletonInstance && instance->HasLockedType();
#endif

        for (int i = 0; i < propertyCount; i++)
        {
            Var value = instance->GetSlot(i);
            Assert(value != nullptr || IsInternalPropertyId(descriptors[i].Id->GetPropertyId()));
#if ENABLE_FIXED_FIELDS
            bool markAsFixed = allowFixedFields && !IsInternalPropertyId(descriptors[i].Id->GetPropertyId()) &&
                (VarIs<JavascriptFunction>(value) ? ShouldFixMethodProperties() : false);
#else
            bool markAsFixed = false;
#endif
            newTypeHandler->Add(PointerValue(descriptors[i].Id), descriptors[i].Attributes, true, markAsFixed, false, scriptContext);
        }

        newTypeHandler->SetFlags(IsPrototypeFlag | HasKnownSlot0Flag, this->GetFlags());
        // We don't expect to convert to a PathTypeHandler.  If we change this later we need to review if we want the new type handler to have locked
        // inline slot capacity, or if we want to allow shrinking of the SimpleTypeHandler's inline slot capacity.
        Assert(!newTypeHandler->IsPathTypeHandler());
        Assert(newTypeHandler->GetIsInlineSlotCapacityLocked());
        newTypeHandler->SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, this->GetPropertyTypes());
        newTypeHandler->SetInstanceTypeHandler(instance);

#if ENABLE_FIXED_FIELDS && DBG
        // If we marked fields as fixed we had better forced a type transition.
        Assert(!allowFixedFields || instance->GetDynamicType() != oldType);
#endif

        return newTypeHandler;
    }

    template<size_t size>
    bool SimpleTypeHandler<size>::HasDeletedProperties()
    {
        for (PropertyIndex i = 0; i < propertyCount; i++)
        {
            if (descriptors[i].Attributes & PropertyDeleted)
            {
                return true;
            }
        }

        return false;
    }


    template<size_t size>
    PathTypeHandlerBase* SimpleTypeHandler<size>::ConvertToPathType(DynamicObject* instance)
    {
        ScriptContext *scriptContext = instance->GetScriptContext();
        PathTypeHandlerBase* newTypeHandler =
            PathTypeHandlerNoAttr::New(
                scriptContext,
                scriptContext->GetLibrary()->GetRootPath(),
                0,
                static_cast<PropertyIndex>(this->GetSlotCapacity()),
                this->GetInlineSlotCapacity(),
                this->GetOffsetOfInlineSlots(),
                true,
                false);
        newTypeHandler->SetMayBecomeShared();

#if ENABLE_FIXED_FIELDS
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        Assert(HasSingletonInstanceOnlyIfNeeded());

        // If instance has a shared type, the type handler change below will induce a type transition, which
        // guarantees that any existing fast path field stores (which could quietly overwrite a fixed field
        // on this instance) will be invalidated.  It is safe to mark all fields as fixed.
        bool const allowFixedFields = DynamicTypeHandler::AreSingletonInstancesNeeded() && instance->HasLockedType();
#endif
#endif

        DynamicType *existingType = instance->GetDynamicType();
        DynamicType *currentType = DynamicType::New(scriptContext, existingType->GetTypeId(), existingType->GetPrototype(), nullptr, newTypeHandler, false, false);
        PropertyId propertyId = Constants::NoProperty;
        ObjectSlotAttributes attr = ObjectSlotAttr_None;
        for (PropertyIndex i = 0; i < propertyCount; i++)
        {
            propertyId = descriptors[i].Id->GetPropertyId();
            attr = PathTypeHandlerBase::PropertyAttributesToObjectSlotAttributes(descriptors[i].Attributes);
            PropertyIndex index;
            currentType = newTypeHandler->PromoteType<false>(currentType, PathTypeSuccessorKey(propertyId, attr), false, scriptContext, instance, &index);
            newTypeHandler = PathTypeHandlerBase::FromTypeHandler(currentType->GetTypeHandler());
#if ENABLE_FIXED_FIELDS
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            Var value = instance->GetSlot(i);
            bool markAsFixed = allowFixedFields && !IsInternalPropertyId(propertyId) && value != nullptr &&
                (VarIs<JavascriptFunction>(value) ? ShouldFixMethodProperties() : false);
            newTypeHandler->InitializePath(instance, i, newTypeHandler->GetPathLength(), scriptContext, [=]() { return markAsFixed; });
#endif
#endif
        }

        if (existingType->GetIsLocked())
        {
            newTypeHandler->LockTypeHandler();
        }
        if (existingType->GetIsShared())
        {
            newTypeHandler->ShareTypeHandler(scriptContext);
        }
        newTypeHandler->SetFlags(IsPrototypeFlag | HasKnownSlot0Flag, this->GetFlags());
        newTypeHandler->SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, this->GetPropertyTypes());
        newTypeHandler->SetInstanceTypeHandler(instance, false);
        if (newTypeHandler->GetPredecessorType())
        {
            PathTypeHandlerBase *predTypeHandler = PathTypeHandlerBase::FromTypeHandler(newTypeHandler->GetPredecessorType()->GetTypeHandler());
            predTypeHandler->ReplaceSuccessor(newTypeHandler->GetPredecessorType(), PathTypeSuccessorKey(propertyId, attr), scriptContext->GetRecycler()->CreateWeakReferenceHandle<DynamicType>(existingType));
        }

        return newTypeHandler;
    }

    template<size_t size>
    DictionaryTypeHandler* SimpleTypeHandler<size>::ConvertToDictionaryType(DynamicObject* instance)
    {
        DictionaryTypeHandler* newTypeHandler = ConvertToTypeHandler<DictionaryTypeHandler>(instance);

#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertSimpleToDictionaryCount++;
#endif
        return newTypeHandler;
    }

    template<size_t size>
    SimpleDictionaryTypeHandler* SimpleTypeHandler<size>::ConvertToSimpleDictionaryType(DynamicObject* instance)
    {
        SimpleDictionaryTypeHandler* newTypeHandler = ConvertToTypeHandler<SimpleDictionaryTypeHandler >(instance);

#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertSimpleToSimpleDictionaryCount++;
#endif
        return newTypeHandler;
    }

    template<size_t size>
    ES5ArrayTypeHandler* SimpleTypeHandler<size>::ConvertToES5ArrayType(DynamicObject* instance)
    {
        ES5ArrayTypeHandler* newTypeHandler = ConvertToTypeHandler<ES5ArrayTypeHandler>(instance);

#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertSimpleToDictionaryCount++;
#endif
        return newTypeHandler;
    }

    template<size_t size>
    int SimpleTypeHandler<size>::GetPropertyCount()
    {
        return propertyCount;
    }

    template<size_t size>
    PropertyId SimpleTypeHandler<size>::GetPropertyId(ScriptContext* scriptContext, PropertyIndex index)
    {
        if (index < propertyCount && !(descriptors[index].Attributes & PropertyDeleted))
        {
            return descriptors[index].Id->GetPropertyId();
        }
        else
        {
            return Constants::NoProperty;
        }
    }

    template<size_t size>
    PropertyId SimpleTypeHandler<size>::GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index)
    {
        if (index < propertyCount && !(descriptors[index].Attributes & PropertyDeleted))
        {
            return descriptors[index].Id->GetPropertyId();
        }
        else
        {
            return Constants::NoProperty;
        }
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index, JavascriptString** propertyStringName,
        PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info)
    {
        Assert(propertyStringName);
        Assert(propertyId);
        Assert(type);

        for( ; index < propertyCount; ++index )
        {
            PropertyAttributes attribs = descriptors[index].Attributes;
            if( !(attribs & PropertyDeleted) && (!!(flags & EnumeratorFlags::EnumNonEnumerable) || (attribs & PropertyEnumerable)))
            {
                const PropertyRecord* propertyRecord = descriptors[index].Id;

                // Skip this property if it is a symbol and we are not including symbol properties
                if (!(flags & EnumeratorFlags::EnumSymbols) && propertyRecord->IsSymbol())
                {
                    continue;
                }

                if (attributes != nullptr)
                {
                    *attributes = attribs;
                }

                *propertyId = propertyRecord->GetPropertyId();
                PropertyString * propStr = scriptContext->GetPropertyString(*propertyId);
                *propertyStringName = propStr;

                PropertyValueInfo::SetCacheInfo(info, propStr, propStr->GetLdElemInlineCache(), false);
                if ((attribs & PropertyWritable) == PropertyWritable && type == typeToEnumerate)
                {
                    PropertyValueInfo::Set(info, instance, index, attribs);
                }
                else
                {
                    PropertyValueInfo::SetNoCache(info, instance);
                }
                return TRUE;
            }
        }
        PropertyValueInfo::SetNoCache(info, instance);

        return FALSE;
    }

    template<size_t size>
    PropertyIndex SimpleTypeHandler<size>::GetPropertyIndex(PropertyRecord const* propertyRecord)
    {
        PropertyIndex index;
        if (GetDescriptor(propertyRecord->GetPropertyId(), &index) && !(descriptors[index].Attributes & PropertyDeleted))
        {
            return (PropertyIndex)index;
        }
        return Constants::NoSlot;
    }

#if ENABLE_NATIVE_CODEGEN
    template<size_t size>
    bool SimpleTypeHandler<size>::GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info)
    {
        PropertyIndex index;
        if (GetDescriptor(propertyRecord->GetPropertyId(), &index) && !(descriptors[index].Attributes & PropertyDeleted))
        {
            info.slotIndex = AdjustSlotIndexForInlineSlots((PropertyIndex)index);
            info.isWritable = !!(descriptors[index].Attributes & PropertyWritable);
            return true;
        }
        else
        {
            info.slotIndex = Constants::NoSlot;
            info.isWritable = true;
            return false;
        }
    }

    template<size_t size>
    bool SimpleTypeHandler<size>::IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex)
    {
        Js::EquivalentPropertyEntry* properties = record.properties;
        for (uint pi = 0; pi < record.propertyCount; pi++)
        {
            const EquivalentPropertyEntry* refInfo = &properties[pi];
            if (!this->SimpleTypeHandler<size>::IsObjTypeSpecEquivalent(type, refInfo))
            {
                failedPropertyIndex = pi;
                return false;
            }
        }

        return true;
    }

    template<size_t size>
    bool SimpleTypeHandler<size>::IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry *entry)
    {
        if (this->propertyCount > 0)
        {
            for (int i = 0; i < this->propertyCount; i++)
            {
                SimplePropertyDescriptor* descriptor = &this->descriptors[i];
                Js::PropertyId propertyId = descriptor->Id->GetPropertyId();

                if (entry->propertyId == propertyId && !(descriptor->Attributes & PropertyDeleted))
                {
                    Js::PropertyIndex relSlotIndex = AdjustValidSlotIndexForInlineSlots(static_cast<PropertyIndex>(0));
                    if (relSlotIndex != entry->slotIndex ||
                        entry->isAuxSlot != (GetInlineSlotCapacity() == 0) ||
                        (entry->mustBeWritable && !(descriptor->Attributes & PropertyWritable)))
                    {
                        return false;
                    }
                }
                else
                {
                    if (entry->slotIndex != Constants::NoSlot || entry->mustBeWritable)
                    {
                        return false;
                    }
                }
            }
        }
        else
        {
            if (entry->slotIndex != Constants::NoSlot || entry->mustBeWritable)
            {
                return false;
            }
        }

        return true;
    }
#endif

    template<size_t size>
    BOOL SimpleTypeHandler<size>::HasProperty(DynamicObject* instance, PropertyId propertyId, __out_opt bool *noRedecl, _Inout_opt_ PropertyValueInfo* info)
    {
        if (noRedecl != nullptr)
        {
            *noRedecl = false;
        }

        for (int i = 0; i < propertyCount; i++)
        {
            if (descriptors[i].Id->GetPropertyId() == propertyId)
            {
                if (descriptors[i].Attributes & PropertyDeleted)
                {
                    return false;
                }
                if (noRedecl && descriptors[i].Attributes & PropertyNoRedecl)
                {
                    *noRedecl = true;
                }

                if (info)
                {
                    PropertyValueInfo::Set(info, instance, static_cast<PropertyIndex>(i), descriptors[i].Attributes);
                }
                return true;
            }
        }

        // Check numeric propertyId only if objectArray available
        uint32 indexVal;
        ScriptContext* scriptContext = instance->GetScriptContext();
        if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return SimpleTypeHandler<size>::HasItem(instance, indexVal);
        }

        return false;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::HasProperty(DynamicObject* instance, JavascriptString* propertyNameString)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord* before calling GetSetter");

        JsUtil::CharacterBuffer<WCHAR> propertyName(propertyNameString->GetString(), propertyNameString->GetLength());
        for (int i = 0; i < propertyCount; i++)
        {
            if (descriptors[i].Id->Equals(propertyName))
            {
                return !(descriptors[i].Attributes & PropertyDeleted);
            }
        }

        return false;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::GetProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        for (int i = 0; i < propertyCount; i++)
        {
            if (descriptors[i].Id->GetPropertyId() == propertyId)
            {
                if (descriptors[i].Attributes & PropertyDeleted)
                {
                    *value = requestContext->GetMissingPropertyResult();
                    return false;
                }
                *value = instance->GetSlot(i);
                PropertyValueInfo::Set(info, instance, static_cast<PropertyIndex>(i), descriptors[i].Attributes);
                return true;
            }
        }

        // Check numeric propertyId only if objectArray available
        uint32 indexVal;
        ScriptContext* scriptContext = instance->GetScriptContext();
        if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return SimpleTypeHandler<size>::GetItem(instance, originalInstance, indexVal, value, scriptContext);
        }

        *value = requestContext->GetMissingPropertyResult();
        return false;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::GetProperty(DynamicObject* instance, Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord* before calling GetSetter");

        JsUtil::CharacterBuffer<WCHAR> propertyName(propertyNameString->GetString(), propertyNameString->GetLength());
        for (int i = 0; i < propertyCount; i++)
        {
            if (descriptors[i].Id->Equals(propertyName))
            {
                if (descriptors[i].Attributes & PropertyDeleted)
                {
                    *value = requestContext->GetMissingPropertyResult();
                    return false;
                }
                *value = instance->GetSlot(i);
                PropertyValueInfo::Set(info, instance, static_cast<PropertyIndex>(i), descriptors[i].Attributes);
                return true;
            }
        }

        *value = requestContext->GetMissingPropertyResult();
        return false;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        ScriptContext* scriptContext = instance->GetScriptContext();
        PropertyIndex index;

        JavascriptLibrary::CheckAndInvalidateIsConcatSpreadableCache(propertyId, scriptContext);

        if (GetDescriptor(propertyId, &index))
        {
            if (descriptors[index].Attributes & PropertyDeleted)
            {
                // A locked type should not have deleted properties
                Assert(!GetIsLocked());
                descriptors[index].Attributes = PropertyDynamicTypeDefaults;
                instance->SetHasNoEnumerableProperties(false);
            }
            else if (!(descriptors[index].Attributes & PropertyWritable))
            {
                JavascriptError::ThrowCantAssignIfStrictMode(flags, scriptContext);

                PropertyValueInfo::Set(info, instance, static_cast<PropertyIndex>(index), descriptors[index].Attributes); // Try to cache property info even if not writable
                return false;
            }

            SetSlotUnchecked(instance, index, value);
            PropertyValueInfo::Set(info, instance, static_cast<PropertyIndex>(index), descriptors[index].Attributes);
            SetPropertyUpdateSideEffect(instance, propertyId, value, SideEffects_Any);
            return true;
        }

        // Always check numeric propertyId. This may create objectArray.
        uint32 indexVal;
        if (scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return SimpleTypeHandler<size>::SetItem(instance, indexVal, value, flags);
        }

        return this->AddProperty(instance, propertyId, value, PropertyDynamicTypeDefaults, info, flags, SideEffects_Any);
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        // Either the property exists in the dictionary, in which case a PropertyRecord for it exists,
        // or we have to add it to the dictionary, in which case we need to get or create a PropertyRecord.
        // Thus, just get or create one and call the PropertyId overload of SetProperty.
        PropertyRecord const* propertyRecord;
        instance->GetScriptContext()->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        return SimpleTypeHandler<size>::SetProperty(instance, propertyRecord->GetPropertyId(), value, flags, info);
    }

    template<size_t size>
    DescriptorFlags SimpleTypeHandler<size>::GetSetter(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyIndex index;
        PropertyValueInfo::SetNoCache(info, instance);
        if (GetDescriptor(propertyId, &index))
        {
            if (descriptors[index].Attributes & PropertyDeleted)
            {
                return None;
            }
            return (descriptors[index].Attributes & PropertyWritable) ? WritableData : Data;
        }

        uint32 indexVal;
        if (instance->GetScriptContext()->IsNumericPropertyId(propertyId, &indexVal))
        {
            return SimpleTypeHandler<size>::GetItemSetter(instance, indexVal, setterValue, requestContext);
        }

        return None;
    }

    template<size_t size>
    DescriptorFlags SimpleTypeHandler<size>::GetSetter(DynamicObject* instance, JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord* before calling GetSetter");

        JsUtil::CharacterBuffer<WCHAR> propertyName(propertyNameString->GetString(), propertyNameString->GetLength());
        for (int i = 0; i < propertyCount; i++)
        {
            if (descriptors[i].Id->Equals(propertyName))
            {
                if (descriptors[i].Attributes & PropertyDeleted)
                {
                    return None;
                }
                return (descriptors[i].Attributes & PropertyWritable) ? WritableData : Data;
            }
        }

        return None;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::DeleteProperty(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags propertyOperationFlags)
    {
        ScriptContext* scriptContext = instance->GetScriptContext();
        PropertyIndex index;
        if (GetDescriptor(propertyId, &index))
        {
            if (descriptors[index].Attributes & PropertyDeleted)
            {
                // A locked type should not have deleted properties
                Assert(!GetIsLocked());
                return true;
            }
            if (!(descriptors[index].Attributes & PropertyConfigurable))
            {
                JavascriptError::ThrowCantDeleteIfStrictModeOrNonconfigurable(
                    propertyOperationFlags, scriptContext, scriptContext->GetPropertyName(propertyId)->GetBuffer());

                return false;
            }

            if ((this->GetFlags() & IsPrototypeFlag)
                || JavascriptOperators::HasProxyOrPrototypeInlineCacheProperty(instance, propertyId))
            {
                // We don't evolve dictionary types when deleting a field, so we need to invalidate prototype caches.
                // We only have to do this though if the current type is used as a prototype, or the current property
                // is found on the prototype chain.)
                scriptContext->InvalidateProtoCaches(propertyId);
            }

            instance->ChangeType();


            CompileAssert(_countof(descriptors) == size);
            if (size > 1)
            {
                if (GetIsLocked())
                {
                    // Prevent conversion to path type and then dictionary. Remove this when path types support deleted properties.
                    this->ConvertToNonSharedSimpleType(instance)->SetAttribute(instance, index, PropertyDeleted);
                }
                else
                {
                    SetAttribute(instance, index, PropertyDeleted);
                }
            }
            else
            {
                SetSlotUnchecked(instance, index, nullptr);

                NullTypeHandlerBase* nullTypeHandler = ((this->GetFlags() & IsPrototypeFlag) != 0) ?
                    (NullTypeHandlerBase*)NullTypeHandler<true>::GetDefaultInstance() : (NullTypeHandlerBase*)NullTypeHandler<false>::GetDefaultInstance();
                if (instance->HasReadOnlyPropertiesInvisibleToTypeHandler())
                {
                    nullTypeHandler->ClearHasOnlyWritableDataProperties();
                }
                SetInstanceTypeHandler(instance, nullTypeHandler, false);
            }

            return true;
        }

        // Check numeric propertyId only if objectArray available
        uint32 indexVal;
        if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return SimpleTypeHandler<size>::DeleteItem(instance, indexVal, propertyOperationFlags);
        }

        return true;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::IsEnumerable(DynamicObject* instance, PropertyId propertyId)
    {
        PropertyIndex index;
        if (!GetDescriptor(propertyId, &index))
        {
            return true;
        }
        return descriptors[index].Attributes & PropertyEnumerable;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::IsWritable(DynamicObject* instance, PropertyId propertyId)
    {
        PropertyIndex index;
        if (!GetDescriptor(propertyId, &index))
        {
            return true;
        }
        return descriptors[index].Attributes & PropertyWritable;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::IsConfigurable(DynamicObject* instance, PropertyId propertyId)
    {
        PropertyIndex index;
        if (!GetDescriptor(propertyId, &index))
        {
            return true;
        }
        return descriptors[index].Attributes & PropertyConfigurable;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        PropertyIndex index;
        if (!GetDescriptor(propertyId, &index))
        {
            // Upgrade type handler if set objectArray item attribute.
            // Only check numeric propertyId if objectArray available.
            ScriptContext* scriptContext = instance->GetScriptContext();
            uint32 indexVal;
            if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
            {
                return SimpleTypeHandler<size>::ConvertToTypeWithItemAttributes(instance)
                    ->SetEnumerable(instance, propertyId, value);
            }
            return true;
        }

        if (value)
        {
            if (SetAttribute(instance, index, PropertyEnumerable))
            {
                instance->SetHasNoEnumerableProperties(false);
            }
        }
        else
        {
            ClearAttribute(instance, index, PropertyEnumerable);
        }
        return true;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        PropertyIndex index;
        if (!GetDescriptor(propertyId, &index))
        {
            // Upgrade type handler if set objectArray item attribute.
            // Only check numeric propertyId if objectArray available.
            ScriptContext* scriptContext = instance->GetScriptContext();
            uint32 indexVal;
            if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
            {
                return SimpleTypeHandler<size>::ConvertToTypeWithItemAttributes(instance)
                    ->SetWritable(instance, propertyId, value);
            }
            return true;
        }

        const Type* oldType = instance->GetType();
        if (value)
        {
            if (SetAttribute(instance, index, PropertyWritable))
            {
                instance->ChangeTypeIf(oldType); // Ensure type change to invalidate caches
            }
        }
        else
        {
            if (ClearAttribute(instance, index, PropertyWritable))
            {
                instance->ChangeTypeIf(oldType); // Ensure type change to invalidate caches

                // Clearing the attribute may have changed the type handler, so make sure
                // we access the current one.
                DynamicTypeHandler* const typeHandler = GetCurrentTypeHandler(instance);
                JavascriptLibrary * library = instance->GetLibrary();
                library->GetTypesWithOnlyWritablePropertyProtoChainCache()->ProcessProperty(typeHandler, PropertyNone, propertyId, library->GetScriptContext());
            }
        }
        return true;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        PropertyIndex index;
        if (!GetDescriptor(propertyId, &index))
        {
            // Upgrade type handler if set objectArray item attribute.
            // Only check numeric propertyId if objectArray available.
            ScriptContext* scriptContext = instance->GetScriptContext();
            uint32 indexVal;
            if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
            {
                return SimpleTypeHandler<size>::ConvertToTypeWithItemAttributes(instance)
                    ->SetConfigurable(instance, propertyId, value);
            }
            return true;
        }

        if (value)
        {
            SetAttribute(instance, index, PropertyConfigurable);
        }
        else
        {
            ClearAttribute(instance, index, PropertyConfigurable);
        }
        return true;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::GetDescriptor(PropertyId propertyId, PropertyIndex * index)
    {
        for (PropertyIndex i = 0; i < propertyCount; i++)
        {
            if (descriptors[i].Id->GetPropertyId() == propertyId)
            {
                *index = i;
                return true;
            }
        }
        return false;
    }

    //
    // Set an attribute bit. Return true if change is made.
    //
    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetAttribute(DynamicObject* instance, PropertyIndex index, PropertyAttributes attribute)
    {
        if (descriptors[index].Attributes & PropertyDeleted)
        {
            // A locked type should not have deleted properties
            Assert(!GetIsLocked());
            return false;
        }

        PropertyAttributes attributes = descriptors[index].Attributes;
        attributes |= attribute;
        if (attributes == descriptors[index].Attributes)
        {
            return false;
        }

        // If the type is locked we must force type transition to invalidate any potential property string
        // caches used in snapshot enumeration.
        if (GetIsLocked())
        {
            DynamicType* oldType = instance->GetDynamicType();

            // This changes TypeHandler, but not necessarily Type.
            if (DoConvertToPathType(oldType))
            {
                this->ConvertToPathType(instance)->SetAttributesAtIndex(instance, descriptors[index].Id->GetPropertyId(), index, attributes);
            }
            else
            {
                // Don't attempt to share cross-site function types
                this->ConvertToNonSharedSimpleType(instance)->descriptors[index].Attributes = attributes;
            }
            Assert(!oldType->GetIsLocked() || instance->GetDynamicType() != oldType);
        }
        else
        {
            descriptors[index].Attributes = attributes;
        }
        return true;
    }

    //
    // Clear an attribute bit. Return true if change is made.
    //
    template<size_t size>
    BOOL SimpleTypeHandler<size>::ClearAttribute(DynamicObject* instance, PropertyIndex index, PropertyAttributes attribute)
    {
        if (descriptors[index].Attributes & PropertyDeleted)
        {
            // A locked type should not have deleted properties
            Assert(!GetIsLocked());
            return false;
        }

        PropertyAttributes attributes = descriptors[index].Attributes;
        attributes &= ~attribute;
        if (attributes == descriptors[index].Attributes)
        {
            return false;
        }

        // If the type is locked we must force type transition to invalidate any potential property string
        // caches used in snapshot enumeration.
        if (GetIsLocked())
        {
            DynamicType* oldType = instance->GetDynamicType();

            // This changes TypeHandler, but not necessarily Type.
            if (DoConvertToPathType(oldType))
            {
                this->ConvertToPathType(instance)->SetAttributesAtIndex(instance, descriptors[index].Id->GetPropertyId(), index, attributes);
            }
            else
            {
                // Don't attempt to share cross-site function types
                this->ConvertToNonSharedSimpleType(instance)->descriptors[index].Attributes = attributes;
            }
            Assert(!oldType->GetIsLocked() || instance->GetDynamicType() != oldType);
        }
        else
        {
            descriptors[index].Attributes = attributes;
        }
        return true;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        if (DoConvertToPathType(instance->GetDynamicType()))
        {
            return ConvertToPathType(instance)->SetAccessors(instance, propertyId, getter, setter, flags);
        }
        else
        {
            // Don't attempt to share cross-site function types
            return ConvertToDictionaryType(instance)->SetAccessors(instance, propertyId, getter, setter, flags);
        }
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::PreventExtensions(DynamicObject* instance)
    {
        return ConvertToDictionaryType(instance)->PreventExtensions(instance);
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::Seal(DynamicObject* instance)
    {
        return ConvertToDictionaryType(instance)->Seal(instance);
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::FreezeImpl(DynamicObject* instance, bool isConvertedType)
    {
        return ConvertToDictionaryType(instance)->Freeze(instance, true);
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetPropertyWithAttributes(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        PropertyIndex index;
        if (GetDescriptor(propertyId, &index))
        {
            if (descriptors[index].Attributes != attributes)
            {
                SimpleTypeHandler * typeHandler = this;
                if (GetIsLocked())
                {
                    DynamicType* oldType = instance->GetDynamicType();
                    if (DoConvertToPathType(oldType))
                    {
                        return this->ConvertToPathType(instance)->SetPropertyWithAttributes(instance, propertyId, value, attributes, info, flags, possibleSideEffects);
                    }
                    else
                    {
                        // Don't attempt to share cross-site function types
                        typeHandler = this->ConvertToNonSharedSimpleType(instance);
                    }
                    Assert(!oldType->GetIsLocked() || instance->GetDynamicType() != oldType);
                }
                if (descriptors[index].Attributes & PropertyDeleted)
                {
                    instance->GetScriptContext()->InvalidateProtoCaches(propertyId);
                }
                typeHandler->descriptors[index].Attributes = attributes;
                if (attributes & PropertyEnumerable)
                {
                    instance->SetHasNoEnumerableProperties(false);
                }
                JavascriptLibrary * library = instance->GetLibrary();
                library->GetTypesWithOnlyWritablePropertyProtoChainCache()->ProcessProperty(typeHandler, attributes, propertyId, library->GetScriptContext());
            }
            SetSlotUnchecked(instance, index, value);
            PropertyValueInfo::Set(info, instance, static_cast<PropertyIndex>(index), descriptors[index].Attributes);
            SetPropertyUpdateSideEffect(instance, propertyId, value, possibleSideEffects);
            return true;
        }

        // Always check numeric propertyId. May create objectArray.
        ScriptContext* scriptContext = instance->GetScriptContext();
        uint32 indexVal;
        if (scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return SimpleTypeHandler<size>::SetItemWithAttributes(instance, indexVal, value, attributes);
        }

        return this->AddProperty(instance, propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetAttributes(DynamicObject* instance, PropertyId propertyId, PropertyAttributes attributes)
    {
        for (int i = 0; i < propertyCount; i++)
        {
            if (descriptors[i].Id->GetPropertyId() == propertyId)
            {
                if (descriptors[i].Attributes & PropertyDeleted)
                {
                    return true;
                }

                descriptors[i].Attributes = (descriptors[i].Attributes & ~PropertyDynamicTypeDefaults) | (attributes & PropertyDynamicTypeDefaults);
                if (descriptors[i].Attributes & PropertyEnumerable)
                {
                    instance->SetHasNoEnumerableProperties(false);
                }
                JavascriptLibrary * library = instance->GetLibrary();
                library->GetTypesWithOnlyWritablePropertyProtoChainCache()->ProcessProperty(this, descriptors[i].Attributes, propertyId, library->GetScriptContext());
                return true;
            }
        }

        // Check numeric propertyId only if objectArray available
        ScriptContext* scriptContext = instance->GetScriptContext();
        uint32 indexVal;
        if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return SimpleTypeHandler<size>::SetItemAttributes(instance, indexVal, attributes);
        }

        return true;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes)
    {
        if (index >= propertyCount) { return false; }
        Assert(descriptors[index].Id->GetPropertyId() == propertyId);
        if (descriptors[index].Attributes & PropertyDeleted)
        {
            return false;
        }
        *attributes = descriptors[index].Attributes & PropertyDynamicTypeDefaults;
        return true;
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::AddProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        ScriptContext* scriptContext = instance->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
#if DBG
        PropertyIndex index;
        uint32 indexVal;
        Assert(!GetDescriptor(propertyId, &index));
        Assert(!scriptContext->IsNumericPropertyId(propertyId, &indexVal));
#endif

        if (propertyCount >= sizeof(descriptors)/sizeof(SimplePropertyDescriptor))
        {
            Assert(propertyId != Constants::NoProperty);
            if (DoConvertToPathType(instance->GetDynamicType()))
            {
                return ConvertToPathType(instance)->SetPropertyWithAttributes(instance, propertyId, value, attributes, info, flags);
            }
            else
            {
                PropertyRecord const* propertyRecord = scriptContext->GetPropertyName(propertyId);
                return ConvertToSimpleDictionaryType(instance)->AddProperty(instance, propertyRecord, value, attributes, info, flags, possibleSideEffects);
            }
        }

        descriptors[propertyCount].Id = scriptContext->GetPropertyName(propertyId);
        descriptors[propertyCount].Attributes = attributes;
        if (attributes & PropertyEnumerable)
        {
            instance->SetHasNoEnumerableProperties(false);
        }

        library->GetTypesWithOnlyWritablePropertyProtoChainCache()->ProcessProperty(this, attributes, propertyId, scriptContext);
        library->GetTypesWithNoSpecialPropertyProtoChainCache()->ProcessProperty(this, attributes, propertyId, scriptContext);

        SetSlotUnchecked(instance, propertyCount, value);
        PropertyValueInfo::Set(info, instance, static_cast<PropertyIndex>(propertyCount), attributes);
        propertyCount++;

        if ((this->GetFlags() && IsPrototypeFlag)
            || JavascriptOperators::HasProxyOrPrototypeInlineCacheProperty(instance, propertyId))
        {
            scriptContext->InvalidateProtoCaches(propertyId);
        }
        SetPropertyUpdateSideEffect(instance, propertyId, value, possibleSideEffects);
        return true;
    }

    template<size_t size>
    void SimpleTypeHandler<size>::SetAllPropertiesToUndefined(DynamicObject* instance, bool invalidateFixedFields)
    {
        // Note: This method is currently only called from ResetObject, which in turn only applies to external objects.
        // Before using for other purposes, make sure the assumptions made here make sense in the new context.  In particular,
        // the invalidateFixedFields == false is only correct if a) the object is known not to have any, or b) the type of the
        // object has changed and/or property guards have already been invalidated through some other means.

        // We can ignore invalidateFixedFields, because SimpleTypeHandler doesn't support fixed fields at this point.
        Js::RecyclableObject* undefined = instance->GetLibrary()->GetUndefined();
        for (int propertyIndex = 0; propertyIndex < this->propertyCount; propertyIndex++)
        {
            SetSlotUnchecked(instance, propertyIndex, undefined);
        }
    }

    template<size_t size>
    void SimpleTypeHandler<size>::MarshalAllPropertiesToScriptContext(DynamicObject* instance, ScriptContext* targetScriptContext, bool invalidateFixedFields)
    {
        // Note: This method is currently only called from ResetObject, which in turn only applies to external objects.
        // Before using for other purposes, make sure the assumptions made here make sense in the new context.  In particular,
        // the invalidateFixedFields == false is only correct if a) the object is known not to have any, or b) the type of the
        // object has changed and/or property guards have already been invalidated through some other means.

        // We can ignore invalidateFixedFields, because SimpleTypeHandler doesn't support fixed fields at this point.
        for (int propertyIndex = 0; propertyIndex < this->propertyCount; propertyIndex++)
        {
            SetSlotUnchecked(instance, propertyIndex, CrossSite::MarshalVar(targetScriptContext, GetSlot(instance, propertyIndex)));
        }
    }

    template<size_t size>
    DynamicTypeHandler* SimpleTypeHandler<size>::ConvertToTypeWithItemAttributes(DynamicObject* instance)
    {
        return JavascriptArray::IsNonES5Array(instance) ?
            ConvertToES5ArrayType(instance) : ConvertToDictionaryType(instance);
    }

    template<size_t size>
    void SimpleTypeHandler<size>::SetIsPrototype(DynamicObject* instance)
    {
        // Don't return if IsPrototypeFlag is set, because we may still need to do a type transition and
        // set fixed bits.  If this handler is shared, this instance may not even be a prototype yet.
        // In this case we may need to convert to a non-shared type handler.
        if (!ChangeTypeOnProto() && !(GetIsOrMayBecomeShared() && IsolatePrototypes()))
        {
            return;
        }

        ConvertToSimpleDictionaryType(instance)->SetIsPrototype(instance);
    }

    template<size_t size>
    BOOL SimpleTypeHandler<size>::SetInternalProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags)
    {
        return SetPropertyWithAttributes(instance, propertyId, value, PropertyInternalDefaults, nullptr, flags);
    }

#if DBG
    template<size_t size>
    bool SimpleTypeHandler<size>::CanStorePropertyValueDirectly(const DynamicObject* instance, PropertyId propertyId, bool allowLetConst)
    {
        Assert(!allowLetConst);
        PropertyIndex index;
        if (GetDescriptor(propertyId, &index))
        {
            return true;
        }
        else
        {
            AssertMsg(false, "Asking about a property this type handler doesn't know about?");
            return false;
        }
    }
#endif

#if ENABLE_TTD
    template<size_t size>
    void SimpleTypeHandler<size>::MarkObjectSlots_TTD(TTD::SnapshotExtractor* extractor, DynamicObject* obj) const
    {
        uint32 plength = this->propertyCount;

        for(uint32 index = 0; index < plength; ++index)
        {
            Js::PropertyId pid = this->descriptors[index].Id->GetPropertyId();

            if(DynamicTypeHandler::ShouldMarkPropertyId_TTD(pid) & !(this->descriptors[index].Attributes & PropertyDeleted))
            {
                Js::Var value = obj->GetSlot(index);
                extractor->MarkVisitVar(value);
            }
        }
    }

    template<size_t size>
    uint32 SimpleTypeHandler<size>::ExtractSlotInfo_TTD(TTD::NSSnapType::SnapHandlerPropertyEntry* entryInfo, ThreadContext* threadContext, TTD::SlabAllocator& alloc) const
    {
        uint32 plength = this->propertyCount;

        for(uint32 index = 0; index < plength; ++index)
        {
            TTD::NSSnapType::ExtractSnapPropertyEntryInfo(entryInfo + index, this->descriptors[index].Id->GetPropertyId(), this->descriptors[index].Attributes, TTD::NSSnapType::SnapEntryDataKindTag::Data);
        }

        return plength;
    }

    template<size_t size>
    Js::BigPropertyIndex SimpleTypeHandler<size>::GetPropertyIndex_EnumerateTTD(const Js::PropertyRecord* pRecord)
    {
        PropertyIndex index;
        if(this->GetDescriptor(pRecord->GetPropertyId(), &index))
        {
            TTDAssert(!(this->descriptors[index].Attributes & PropertyDeleted), "How is this deleted but we enumerated it anyway???");

            return (Js::BigPropertyIndex)index;
        }

        TTDAssert(false, "We found this during enum so what is going on here?");
        return Js::Constants::NoBigSlot;
    }

#endif

#if DBG_DUMP
    template<size_t size>
    void SimpleTypeHandler<size>::Dump(unsigned indent) const
    {
        Output::Print(_u("%*sSimpleTypeHandler<%u> (0x%p): Dump unimplemented\n"), indent, _u(""), size, this);
    }
#endif

    template class SimpleTypeHandler<1>;
    template class SimpleTypeHandler<2>;
    template class SimpleTypeHandler<3>;
    template class SimpleTypeHandler<6>;

}

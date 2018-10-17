//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    // These attributes should match up for ease of translation
    CompileAssert(ObjectSlotAttr_Enumerable == PropertyEnumerable);
    CompileAssert(ObjectSlotAttr_Configurable == PropertyConfigurable);
    CompileAssert(ObjectSlotAttr_Writable == PropertyWritable);
    CompileAssert(ObjectSlotAttr_Deleted == PropertyDeleted);

    PathTypeSuccessorKey::PathTypeSuccessorKey() : propertyId(Constants::NoProperty), attributes(ObjectSlotAttr_Default)
    {
    }

    PathTypeSuccessorKey::PathTypeSuccessorKey(
        const PropertyId propertyId,
        const ObjectSlotAttributes attributes)
        : propertyId(propertyId), attributes(attributes)
    {
    }

    bool PathTypeSuccessorKey::HasInfo() const
    {
        return propertyId != Constants::NoProperty;
    }

    void PathTypeSuccessorKey::Clear()
    {
        propertyId = Constants::NoProperty;
    }

    PropertyId PathTypeSuccessorKey::GetPropertyId() const
    {
        return propertyId;
    }

    ObjectSlotAttributes PathTypeSuccessorKey::GetAttributes() const
    {
        return attributes;
    }

    bool PathTypeSuccessorKey::operator ==(const PathTypeSuccessorKey &other) const
    {
        return propertyId == other.propertyId && attributes == other.attributes;
    }

    bool PathTypeSuccessorKey::operator !=(const PathTypeSuccessorKey &other) const
    {
        return !(*this == other);
    }

    hash_t PathTypeSuccessorKey::GetHashCode() const
    {
        return static_cast<hash_t>((propertyId << ObjectSlotAttr_BitSize) | static_cast<ObjectSlotAttr_TSize>(attributes));
    }

    bool PathTypeSuccessorInfo::GetSuccessor(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> ** typeWeakRef) const
    {
        if (IsSingleSuccessor())
        {
            return static_cast<const PathTypeSingleSuccessorInfo*>(this)->GetSuccessor(successorKey, typeWeakRef);
        }
        else
        {
            return static_cast<const PathTypeMultiSuccessorInfo*>(this)->GetSuccessor(successorKey, typeWeakRef);
        }
    }

    void PathTypeSuccessorInfo::SetSuccessor(DynamicType * type, const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext)
    {
        if (IsSingleSuccessor())
        {
            static_cast<PathTypeSingleSuccessorInfo*>(this)->SetSuccessor(type, successorKey, typeWeakRef, scriptContext);
        }
        else
        {
            static_cast<PathTypeMultiSuccessorInfo*>(this)->SetSuccessor(type, successorKey, typeWeakRef, scriptContext);
        }
    }

    void PathTypeSuccessorInfo::ReplaceSuccessor(DynamicType * type, PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef)
    {
        if (IsSingleSuccessor())
        {
            static_cast<PathTypeSingleSuccessorInfo*>(this)->ReplaceSuccessor(type, successorKey, typeWeakRef);
        }
        else
        {
            static_cast<PathTypeMultiSuccessorInfo*>(this)->ReplaceSuccessor(type, successorKey, typeWeakRef);
        }
    }

    template<class Fn>
    void PathTypeSuccessorInfo::MapSuccessors(Fn fn)
    {
        if (IsSingleSuccessor())
        {
            (static_cast<PathTypeSingleSuccessorInfo*>(this))->MapSingleSuccessor(fn);
        }
        else
        {
            (static_cast<PathTypeMultiSuccessorInfo*>(this))->MapMultiSuccessors(fn);
        }
    }

    template<class Fn>
    void PathTypeSuccessorInfo::MapSuccessorsUntil(Fn fn)
    {
        if (IsSingleSuccessor())
        {
            (static_cast<PathTypeSingleSuccessorInfo*>(this))->MapSingleSuccessor(fn);
        }
        else
        {
            (static_cast<PathTypeMultiSuccessorInfo*>(this))->MapMultiSuccessorsUntil(fn);
        }
    }

    PathTypeSingleSuccessorInfo::PathTypeSingleSuccessorInfo(const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> * typeWeakRef)
        : successorKey(key), successorTypeWeakRef(typeWeakRef)
    {
        Assert(key.HasInfo());
        Assert(typeWeakRef != nullptr);
        this->kind = PathTypeSuccessorKindSingle;
    }

    PathTypeSingleSuccessorInfo * PathTypeSingleSuccessorInfo::New(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext)
    {
        return RecyclerNew(scriptContext->GetRecycler(), PathTypeSingleSuccessorInfo, successorKey, typeWeakRef);
    }

    bool PathTypeSingleSuccessorInfo::GetSuccessor(const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> ** typeWeakRef) const
    {
        if (key != this->successorKey)
        {
            *typeWeakRef = nullptr;
            return false;
        }
        *typeWeakRef = this->successorTypeWeakRef;
        return true;
    }

    void PathTypeSingleSuccessorInfo::SetSuccessor(DynamicType * type, const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext)
    {
        if (!successorTypeWeakRef || !successorTypeWeakRef->Get())
        {
            successorKey = key;
            successorTypeWeakRef = typeWeakRef;
            return;
        }

        Assert(key != successorKey);

        PathTypeMultiSuccessorInfo * newInfo = PathTypeMultiSuccessorInfo::New(this->successorKey, this->successorTypeWeakRef, scriptContext);
        newInfo->SetSuccessor(type, key, typeWeakRef, scriptContext);

        Assert(PathTypeHandlerBase::FromTypeHandler(type->GetTypeHandler())->GetSuccessorInfo() == this);
        PathTypeHandlerBase::FromTypeHandler(type->GetTypeHandler())->SetSuccessorInfo(newInfo);

#ifdef PROFILE_TYPES
        scriptContext->convertSimplePathToPathCount++;
#endif
    }

    void PathTypeSingleSuccessorInfo::ReplaceSuccessor(DynamicType * type, const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> * typeWeakRef)
    {
        Assert(successorKey == key);
        successorTypeWeakRef = typeWeakRef;
    }

    template<class Fn>
    void PathTypeSingleSuccessorInfo::MapSingleSuccessor(Fn fn)
    {
        if (this->successorTypeWeakRef)
        {
            fn(this->successorKey, this->successorTypeWeakRef);
        }
    }

    PathTypeMultiSuccessorInfo::PathTypeMultiSuccessorInfo(Recycler * recycler, const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> * typeWeakRef)
    {
        this->propertySuccessors = RecyclerNew(recycler, PropertySuccessorsMap, recycler, 3);
        this->propertySuccessors->Item(key, typeWeakRef);
        this->kind = PathTypeSuccessorKindMulti;
    }

    PathTypeMultiSuccessorInfo * PathTypeMultiSuccessorInfo::New(const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext)
    {
        return RecyclerNew(scriptContext->GetRecycler(), PathTypeMultiSuccessorInfo, scriptContext->GetRecycler(), key, typeWeakRef);
    }

    bool PathTypeMultiSuccessorInfo::GetSuccessor(const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> ** typeWeakRef) const
    {
        Assert(this->propertySuccessors);
        if (!propertySuccessors->TryGetValue(key, typeWeakRef))
        {
            *typeWeakRef = nullptr;
            return false;
        }
        return true;
    }

    void PathTypeMultiSuccessorInfo::SetSuccessor(DynamicType * type, const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext)
    {
        Assert(this->propertySuccessors);
        propertySuccessors->Item(key, typeWeakRef);
    }

    void PathTypeMultiSuccessorInfo::ReplaceSuccessor(DynamicType * type, const PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> * typeWeakRef)
    {
        Assert(this->propertySuccessors);
        Assert(propertySuccessors->Item(key));
        propertySuccessors->Item(key, typeWeakRef);
    }

    template<class Fn>
    void PathTypeMultiSuccessorInfo::MapMultiSuccessors(Fn fn)
    {
        Assert(this->propertySuccessors);
        this->propertySuccessors->Map(fn);
    }

    template<class Fn>
    void PathTypeMultiSuccessorInfo::MapMultiSuccessorsUntil(Fn fn)
    {
        Assert(this->propertySuccessors);
        this->propertySuccessors->MapUntil(fn);
    }

    PathTypeHandlerBase::PathTypeHandlerBase(TypePath* typePath, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked, bool isShared, DynamicType* predecessorType) :
        DynamicTypeHandler(slotCapacity, inlineSlotCapacity, offsetOfInlineSlots, DefaultFlags | (isLocked ? IsLockedFlag : 0) | (isShared ? (MayBecomeSharedFlag | IsSharedFlag) : 0)),
        typePath(typePath),
        predecessorType(predecessorType),
        successorInfo(nullptr)
    {
        Assert(pathLength <= slotCapacity);
        Assert(inlineSlotCapacity <= slotCapacity);
        SetUnusedBytesValue(pathLength);
        isNotPathTypeHandlerOrHasUserDefinedCtor = predecessorType == nullptr ? false : predecessorType->GetTypeHandler()->GetIsNotPathTypeHandlerOrHasUserDefinedCtor();
    }

    int PathTypeHandlerBase::GetPropertyCount()
    {
        return GetPathLength();
    }

    PropertyId PathTypeHandlerBase::GetPropertyId(ScriptContext* scriptContext, PropertyIndex index)
    {
        if (index < GetPathLength())
        {
            return GetTypePath()->GetPropertyId(index)->GetPropertyId();
        }
        else
        {
            return Constants::NoProperty;
        }
    }

    PropertyId PathTypeHandlerBase::GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index)
    {
        if (index < GetPathLength())
        {
            return GetTypePath()->GetPropertyId(index)->GetPropertyId();
        }
        else
        {
            return Constants::NoProperty;
        }
    }

    bool PathTypeHandlerBase::GetSuccessor(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> ** typeWeakRef) const
    {
        if (this->successorInfo == nullptr)
        {
            *typeWeakRef = nullptr;
            return false;
        }
        return this->successorInfo->GetSuccessor(successorKey, typeWeakRef);
    }

    void PathTypeHandlerBase::SetSuccessor(DynamicType * type, const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext)
    {
#if DBG
        DynamicType * successorType = typeWeakRef->Get();
        AssertMsg(!successorType || !successorType->GetTypeHandler()->IsPathTypeHandler() ||
                  PathTypeHandlerBase::FromTypeHandler(successorType->GetTypeHandler())->GetPredecessorType() == type, "We're using a successor that has a different predecessor?");
#endif
        if (this->successorInfo == nullptr)
        {
            this->successorInfo = PathTypeSingleSuccessorInfo::New(successorKey, typeWeakRef, scriptContext);
            return;
        }
        this->successorInfo->SetSuccessor(type, successorKey, typeWeakRef, scriptContext);
    }

    template<class Fn>
    void PathTypeHandlerBase::MapSuccessors(Fn fn)
    {
        if (!this->successorInfo)
        {
            return;
        }

        this->successorInfo->MapSuccessors(fn);
    }

    template<class Fn>
    void PathTypeHandlerBase::MapSuccessorsUntil(Fn fn)
    {
        if (!this->successorInfo)
        {
            return;
        }

        this->successorInfo->MapSuccessorsUntil(fn);
    }

    BOOL PathTypeHandlerBase::FindNextPropertyHelper(ScriptContext* scriptContext, ObjectSlotAttributes * objectAttrs, PropertyIndex& index, JavascriptString** propertyStringName, PropertyId* propertyId,
        PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info)
    {
        Assert(propertyStringName);
        Assert(propertyId);
        Assert(type);

        if (type == typeToEnumerate)
        {
            for (; index < GetPathLength(); ++index)
            {
                ObjectSlotAttributes attr = objectAttrs ? objectAttrs[index] : ObjectSlotAttr_Default;
                if( !(attr & ObjectSlotAttr_Deleted) && attr != ObjectSlotAttr_Setter && (!!(flags & EnumeratorFlags::EnumNonEnumerable) || (attr & ObjectSlotAttr_Enumerable)))
                {
                    const PropertyRecord* propertyRecord = GetTypePath()->GetPropertyId(index);

                    // Skip this property if it is a symbol and we are not including symbol properties
                    if (!(flags & EnumeratorFlags::EnumSymbols) && propertyRecord->IsSymbol())
                    {
                        continue;
                    }

                    if (attributes)
                    {
                        *attributes = ObjectSlotAttributesToPropertyAttributes(attr);
                    }

                    *propertyId = propertyRecord->GetPropertyId();
                    PropertyString* propertyString = scriptContext->GetPropertyString(*propertyId);
                    *propertyStringName = propertyString;

                    if ((attr & (ObjectSlotAttr_Writable | ObjectSlotAttr_Accessor)) == ObjectSlotAttr_Writable)
                    {
                        PropertyValueInfo::SetCacheInfo(info, propertyString, propertyString->GetLdElemInlineCache(), false);
                        SetPropertyValueInfo(info, instance, index, attr);
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

        // Need to enumerate a different type than the current one. This is because type snapshot enumerate is enabled and the
        // object's type changed since enumeration began, so need to enumerate properties of the initial type.
        DynamicTypeHandler *const typeHandlerToEnumerate = typeToEnumerate->GetTypeHandler();

        if (!typeHandlerToEnumerate->IsPathTypeHandler())
        {
            return typeHandlerToEnumerate->FindNextProperty(scriptContext, index, propertyStringName, propertyId, attributes, type, typeToEnumerate, flags, instance, info);
        }

        PathTypeHandlerBase* pathTypeToEnumerate = (PathTypeHandlerBase*)typeHandlerToEnumerate;

        BOOL found = pathTypeToEnumerate->FindNextProperty(scriptContext, index, propertyStringName, propertyId, attributes, typeToEnumerate, typeToEnumerate, flags, instance, info);

        // We got a property from previous type, but this property may have been deleted
        if (found == TRUE && this->GetPropertyIndex(*propertyId) == Js::Constants::NoSlot)
        {
            PropertyValueInfo::SetNoCache(info, instance);
            return FALSE;
        }
        PropertyValueInfo::SetNoCache(info, instance);

        return found;
    }

    BOOL PathTypeHandlerBase::SetAttributesHelper(DynamicObject* instance, PropertyId propertyId, PropertyIndex propertyIndex, ObjectSlotAttributes * instanceAttributes, ObjectSlotAttributes propertyAttributes, bool setAllAttributes)
    {
        if (instanceAttributes)
        {
            if (!setAllAttributes)
            {
                // Preserve non-default bits like accessors
                propertyAttributes = (ObjectSlotAttributes)(propertyAttributes | (instanceAttributes[propertyIndex] & ~ObjectSlotAttr_PropertyAttributesMask));
            }
            if (propertyAttributes == instanceAttributes[propertyIndex])
            {
                return true;
            }
        }
        else if (propertyAttributes == ObjectSlotAttr_Default)
        {
            return true;
        }

        // Create a handler with attributes and use it to set the attribute.

        // Find the predecessor from which to branch.
        PathTypeHandlerBase *predTypeHandler = this;
        DynamicType *currentType = instance->GetDynamicType();
        while (predTypeHandler->GetPathLength() > propertyIndex)
        {
            currentType = predTypeHandler->GetPredecessorType();
            if (currentType == nullptr)
            {
#ifdef PROFILE_TYPES
                instance->GetScriptContext()->convertPathToDictionaryNoRootCount++;
#endif
                // This can happen if object header inlining is deoptimized, and we haven't built a full path from the root.
                // For now, just punt this case.

                if (setAllAttributes)
                {
                    // We could be trying to convert an accessor to a data property, or something similar, so do the type handler conversion here and let the caller handle setting the attributes.
                    TryConvertToSimpleDictionaryType(instance, GetPathLength());
                    return false;
                }
                else
                {
                    return TryConvertToSimpleDictionaryType(instance, GetPathLength())->SetAttributes(instance, propertyId, ObjectSlotAttributesToPropertyAttributes(propertyAttributes));
                }
            }
            predTypeHandler = PathTypeHandlerBase::FromTypeHandler(currentType->GetTypeHandler());
        }
        Assert(predTypeHandler);
        Assert(predTypeHandler->GetTypePath()->LookupInline(propertyId, predTypeHandler->GetPathLength()) == Constants::NoSlot);

        // Add this property with the new attributes and add the remaining properties with no attributes.
        PropertyIndex pathLength = GetPathLength();
        PropertyIndex currentSlotIndex = propertyIndex;
        ObjectSlotAttributes currentAttributes = propertyAttributes;
        PathTypeHandlerBase *currentTypeHandler = predTypeHandler;
        ScriptContext *scriptContext = instance->GetScriptContext();
        while (true)
        {
            const PropertyRecord *currentPropertyRecord = GetTypePath()->GetPropertyIdUnchecked(currentSlotIndex);
            PropertyIndex index;
            currentType = currentTypeHandler->PromoteType<false>(currentType, PathTypeSuccessorKey(currentPropertyRecord->GetPropertyId(), currentAttributes), false, scriptContext, instance, &index);
            currentTypeHandler = PathTypeHandlerBase::FromTypeHandler(currentType->GetTypeHandler());
#if ENABLE_FIXED_FIELDS
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            currentTypeHandler->InitializePath(
                instance, currentSlotIndex, currentTypeHandler->GetPathLength(), scriptContext, [=]() { return typePath->GetIsFixedFieldAt(currentSlotIndex, currentTypeHandler->GetPathLength()); });
#endif
#endif
            if (currentAttributes == ObjectSlotAttr_Setter)
            {
                PropertyIndex getterIndex = currentTypeHandler->GetPropertyIndex(currentPropertyRecord->GetPropertyId());
                Assert(getterIndex != Constants::NoSlot);
                if (currentTypeHandler->GetAttributes(getterIndex) & ObjectSlotAttr_Accessor)
                {
                    currentTypeHandler->SetSetterSlot(getterIndex, (PathTypeSetterSlotIndex)currentSlotIndex);
                }
            }
            currentSlotIndex = currentTypeHandler->GetPathLength();
            if (currentSlotIndex >= pathLength)
            {
                break;
            }
            currentAttributes = instanceAttributes ? instanceAttributes[currentSlotIndex] : ObjectSlotAttr_Default;
        }

        Assert(currentType != instance->GetType());
        instance->ReplaceType(currentType);
        if(!IsolatePrototypes() && GetFlags() & IsPrototypeFlag)
        {
            scriptContext->InvalidateProtoCaches(propertyId);
        }

        return true;
    }


#if ENABLE_FIXED_FIELDS
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
    void PathTypeHandlerBase::InitializeExistingPath(
        const PropertyIndex slotIndex,
        const PropertyIndex objectSlotCount,
        ScriptContext *const scriptContext)
    {
        Assert(scriptContext);

        TypePath *const typePath = GetTypePath();
        Assert(slotIndex < typePath->GetMaxInitializedLength());
        Assert(objectSlotCount <= typePath->GetMaxInitializedLength());

        if(typePath->GetIsUsedFixedFieldAt(slotIndex, objectSlotCount))
        {
            // We are adding a new value where some other instance already has an existing value.  If this is a fixed
            // field we must clear the bit. If the value was hard coded in the JIT-ed code, we must invalidate the guards.

            // Invalidate any JIT-ed code that hard coded this method. No need to invalidate store field
            // inline caches (which might quitely overwrite this fixed fields, because they have never been populated.
            scriptContext->GetThreadContext()->InvalidatePropertyGuards(typePath->GetPropertyIdUnchecked(slotIndex)->GetPropertyId());
        }

        // If we're overwriting an existing value of this property, we don't consider the new one fixed.
        // This also means that it's ok to populate the inline caches for this property from now on.
        typePath->ClearIsFixedFieldAt(slotIndex, objectSlotCount);

        Assert(HasOnlyInitializedNonFixedProperties(/*typePath, objectSlotCount*/));
        Assert(HasSingletonInstanceOnlyIfNeeded(/*typePath*/));
        if(objectSlotCount == typePath->GetMaxInitializedLength())
        {
            // We have now reached the most advanced instance along this path.  If this instance is not the singleton instance,
            // then the former singleton instance (if any) is no longer a singleton.  This instance could be the singleton
            // instance, if we just happen to set (overwrite) its last property.

            // This is perhaps the most fragile point of fixed fields on path types.  If we cleared the singleton instance
            // while some fields remained fixed, the instance would be collectible, and yet some code would expect to see
            // values and call methods on it.  Clearly, a recipe for disaster.  We rely on the fact that we always add
            // properties to (pre-initialized) type handlers in the order they appear on the type path.  By the time
            // we reach the singleton instance, all fixed fields will have been invalidated.  Otherwise, some fields
            // could remain fixed (or even uninitialized) and we would have to spin off a loop here to invalidate any
            // remaining fixed fields - a rather unfortunate overhead.
            typePath->ClearSingletonInstance();
        }
    }
#endif
#endif

    PropertyIndex PathTypeHandlerBase::GetPropertyIndex(const PropertyRecord* propertyRecord)
    {
        return GetTypePath()->LookupInline(propertyRecord->GetPropertyId(), GetPathLength());
    }

    PropertyIndex PathTypeHandlerBase::GetPropertyIndex(PropertyId propertyId)
    {
        return GetTypePath()->LookupInline(propertyId, GetPathLength());
    }

#if ENABLE_NATIVE_CODEGEN
    bool PathTypeHandlerBase::GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info)
    {
        Js::PropertyIndex absSlotIndex = GetTypePath()->LookupInline(propertyRecord->GetPropertyId(), GetPathLength());
        info.slotIndex = AdjustSlotIndexForInlineSlots(absSlotIndex);
        info.isAuxSlot = absSlotIndex >= this->inlineSlotCapacity;
        info.isWritable = info.slotIndex != Constants::NoSlot;
        return info.slotIndex != Constants::NoSlot;
    }

    bool PathTypeHandlerBase::IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex)
    {
        return IsObjTypeSpecEquivalentHelper(type, nullptr, record, failedPropertyIndex);
    }

    bool PathTypeHandlerBase::IsObjTypeSpecEquivalentHelper(const Type* type, const ObjectSlotAttributes * attributes, const TypeEquivalenceRecord& record, uint& failedPropertyIndex)
    {
        uint propertyCount = record.propertyCount;
        Js::EquivalentPropertyEntry* properties = record.properties;
        for (uint pi = 0; pi < propertyCount; pi++)
        {
            const EquivalentPropertyEntry* entry = &properties[pi];
            if (!this->PathTypeHandlerBase::IsObjTypeSpecEquivalentHelper(type, attributes, entry))
            {
                failedPropertyIndex = pi;
                return false;
            }
        }

        return true;
    }

    bool PathTypeHandlerBase::IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry *entry)
    {
        return IsObjTypeSpecEquivalentHelper(type, nullptr, entry);
    }

    bool PathTypeHandlerBase::IsObjTypeSpecEquivalentHelper(const Type* type, const ObjectSlotAttributes * attributes, const EquivalentPropertyEntry *entry)
    {
        Js::PropertyIndex absSlotIndex = GetTypePath()->LookupInline(entry->propertyId, GetPathLength());

        if (absSlotIndex != Constants::NoSlot)
        {
            ObjectSlotAttributes attr = attributes ? attributes[absSlotIndex] : ObjectSlotAttr_Default;

            if (attr & ObjectSlotAttr_Deleted)
            {
                return entry->slotIndex == Constants::NoSlot && !entry->mustBeWritable;
            }

            if (attr & ObjectSlotAttr_Accessor)
            {
                return false;
            }

            Js::PropertyIndex relSlotIndex = AdjustValidSlotIndexForInlineSlots(absSlotIndex);
            if (relSlotIndex != entry->slotIndex || ((absSlotIndex >= GetInlineSlotCapacity()) != entry->isAuxSlot))
            {
                return false;
            }

            if (entry->mustBeWritable)
            {
                if (!(attr & ObjectSlotAttr_Writable))
                {
                    return false;
                }

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
                int maxInitializedLength = this->GetTypePath()->GetMaxInitializedLength();
                if (FixPropsOnPathTypes() && (absSlotIndex >= maxInitializedLength || this->GetTypePath()->GetIsFixedFieldAt(absSlotIndex, this->GetPathLength())))
                {
                    return false;
                }
#endif
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

    void PathTypeHandlerBase::SetPropertyValueInfo(PropertyValueInfo* info, RecyclableObject* instance, PropertyIndex index, ObjectSlotAttributes attributes)
    {
        PropertyValueInfo::Set(info, instance, index, ObjectSlotAttributesToPropertyAttributes(attributes));
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        if (FixPropsOnPathTypes() && (index >= this->GetTypePath()->GetMaxInitializedLength() || this->GetTypePath()->GetIsFixedFieldAt(index, GetPathLength())))
        {
            PropertyValueInfo::DisableStoreFieldCache(info);
        }
#endif
    }

    BOOL PathTypeHandlerBase::HasProperty(DynamicObject* instance, PropertyId propertyId, __out_opt bool *noRedecl, _Inout_opt_ PropertyValueInfo* info)
    {
        uint32 indexVal;
        if (noRedecl != nullptr)
        {
            *noRedecl = false;
        }

        PropertyIndex propertyIndex = PathTypeHandlerBase::GetPropertyIndex(propertyId);
        if (propertyIndex != Constants::NoSlot)
        {
            if (info)
            {
                this->SetPropertyValueInfo(info, instance, propertyIndex);
            }
            return true;
        }

        // Check numeric propertyId only if objectArray is available
        ScriptContext* scriptContext = instance->GetScriptContext();
        if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return PathTypeHandlerBase::HasItem(instance, indexVal);
        }

        return false;
    }

    BOOL PathTypeHandlerBase::HasProperty(DynamicObject* instance, JavascriptString* propertyNameString)
    {
        // Consider: Implement actual string hash lookup
        PropertyRecord const* propertyRecord;
        instance->GetScriptContext()->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        return PathTypeHandlerBase::HasProperty(instance, propertyRecord->GetPropertyId());
    }

    BOOL PathTypeHandlerBase::GetProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyIndex index = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (index != Constants::NoSlot)
        {
            *value = instance->GetSlot(index);
            SetPropertyValueInfo(info, instance, index);
            return true;
        }

        // Check numeric propertyId only if objectArray available
        uint32 indexVal;
        if (instance->HasObjectArray() && requestContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return PathTypeHandlerBase::GetItem(instance, originalInstance, indexVal, value, requestContext);
        }

        *value = requestContext->GetMissingPropertyResult();
        return false;
    }

    BOOL PathTypeHandlerBase::GetProperty(DynamicObject* instance, Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        // Consider: Implement actual string hash lookup
        Assert(requestContext);
        PropertyRecord const* propertyRecord;

        if (instance->HasObjectArray())
        {
            requestContext->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        }
        else
        {
            requestContext->FindPropertyRecord(propertyNameString, &propertyRecord);
            if (propertyRecord == nullptr)
            {
                *value = requestContext->GetMissingPropertyResult();
                return false;
            }
        }
        return PathTypeHandlerBase::GetProperty(instance, originalInstance, propertyRecord->GetPropertyId(), value, info, requestContext);
    }

    BOOL PathTypeHandlerBase::InitProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PropertyIndex index = PathTypeHandlerBase::GetPropertyIndex(propertyId);
        return SetPropertyInternal<false>(instance, propertyId, index, value, ObjectSlotAttr_Default, info, flags, SideEffects_Any, true /* IsInit */);
    }

    BOOL PathTypeHandlerBase::SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PropertyIndex index = PathTypeHandlerBase::GetPropertyIndex(propertyId);
        return SetPropertyInternal<false>(instance, propertyId, index, value, ObjectSlotAttr_Default, info, flags, SideEffects_Any);
    }

    BOOL PathTypeHandlerBase::SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        // Consider: Implement actual string hash lookup
        PropertyRecord const* propertyRecord;
        instance->GetScriptContext()->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        return PathTypeHandlerBase::SetProperty(instance, propertyRecord->GetPropertyId(), value, flags, info);
    }

    void PathTypeHandlerBase::SetSlotAndCache(DynamicObject* instance, PropertyId propertyId, PropertyRecord const * propertyRecord, PropertyIndex index, Var value, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
#if ENABLE_FIXED_FIELDS
        // Don't populate inline cache if this handler isn't yet shared.  If we did, a new instance could
        // reach this handler without us noticing and we could fail to release the old singleton instance, which may later
        // become collectible (not referenced by anything other than this handler), thus we would leak the old singleton instance.
        bool populateInlineCache = GetIsShared() ||
            ProcessFixedFieldChange(instance, propertyId, index, value, (flags & PropertyOperation_NonFixedValue) != 0, propertyRecord);
#else
        bool populateInlineCache = true;
#endif

        SetSlotUnchecked(instance, index, value);

        if (populateInlineCache)
        {
#if ENABLE_FIXED_FIELDS
            Assert((instance->GetDynamicType()->GetIsShared()) || (FixPropsOnPathTypes() && instance->GetDynamicType()->GetTypeHandler()->GetIsOrMayBecomeShared()));
#endif
            // Can't assert the following.  With NewScObject we can jump to the type handler at the tip (where the singleton is),
            // even though we haven't yet initialized the properties all the way to the tip, and we don't want to kill
            // the singleton in that case yet.  It's basically a transient inconsistent state, but we have to live with it.
            // The user's code will never see the object in this state.
            //Assert(!instance->GetTypeHandler()->HasSingletonInstance());
            PropertyValueInfo::Set(info, instance, index);
        }
        else
        {
            PropertyValueInfo::SetNoCache(info, instance);
        }

        SetPropertyUpdateSideEffect(instance, propertyId, value, possibleSideEffects);
    }

    template <bool setAttributes>
    BOOL PathTypeHandlerBase::SetPropertyInternal(DynamicObject* instance, PropertyId propertyId, PropertyIndex index, Var value, ObjectSlotAttributes attr, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects, bool isInit)
    {
        PathTypeHandlerBase *newTypeHandler = nullptr;

        // Path type handler doesn't support pre-initialization (PropertyOperation_PreInit). Pre-initialized properties
        // will get marked as fixed when pre-initialized and then as non-fixed when their actual values are set.

        Assert(value != nullptr || IsInternalPropertyId(propertyId));

        JavascriptLibrary::CheckAndInvalidateIsConcatSpreadableCache(propertyId, instance->GetScriptContext());

        if (index != Constants::NoSlot)
        {
            // If type is shared then the handler must be shared as well.  This is a weaker invariant than in AddPropertyInternal,
            // because the type coming in here may be the result of DynamicObject::ChangeType(). In that case the handler may have
            // already been shared, but the newly created type isn't - and likely never will be - shared (is typically unreachable).
            // In CacheOperators::CachePropertyWrite we ensure that we never cache property adds for types that aren't shared.
            Assert(!instance->GetDynamicType()->GetIsShared() || GetIsShared());

            bool setAttrDone;
            if (setAttributes)
            {
                setAttrDone = this->SetAttributesHelper(instance, propertyId, index, GetAttributeArray(), attr, true);
                if (!setAttrDone)
                {
                    return instance->GetTypeHandler()->SetPropertyWithAttributes(instance, propertyId, value, attr, info, flags, possibleSideEffects);
                }
            }
            else if (isInit)
            {
                ObjectSlotAttributes * attributes = this->GetAttributeArray();
                if (attributes && (attributes[index] & ObjectSlotAttr_Accessor))
                {
                    setAttrDone = this->SetAttributesHelper(instance, propertyId, index, attributes, (ObjectSlotAttributes)(attributes[index] & ~ObjectSlotAttr_Accessor), true);
                    if (!setAttrDone)
                    {
                        return instance->GetTypeHandler()->InitProperty(instance, propertyId, value, flags, info);
                    }
                    // We're changing an accessor into a data property at object init time. Don't cache this transition from setter to non-setter,
                    // as it behaves differently from a normal set property.
                    PropertyValueInfo::SetNoCache(info, instance);
                    newTypeHandler = PathTypeHandlerBase::FromTypeHandler(instance->GetDynamicType()->GetTypeHandler());
                    newTypeHandler->SetSlotUnchecked(instance, index, value);
                    return true;
                }
            }
            newTypeHandler = PathTypeHandlerBase::FromTypeHandler(instance->GetDynamicType()->GetTypeHandler());
            newTypeHandler->SetSlotAndCache(instance, propertyId, nullptr, index, value, info, flags, possibleSideEffects);
            return true;
        }

        // Always check numeric propertyId. This may create an objectArray.
        ScriptContext* scriptContext = instance->GetScriptContext();
        uint32 indexVal;
        if (scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            if (setAttributes)
            {
                if (attr != ObjectSlotAttr_Default)
                {
                    return this->ConvertToTypeWithItemAttributes(instance)->SetItemWithAttributes(instance, indexVal, value, ObjectSlotAttributesToPropertyAttributes(attr));
                }
            }
            return PathTypeHandlerBase::SetItem(instance, indexVal, value, PropertyOperation_None);
        }

        return PathTypeHandlerBase::AddPropertyInternal(instance, propertyId, value, attr, info, flags, possibleSideEffects);
    }

    void PathTypeHandlerBase::MoveAuxSlotsToObjectHeader(DynamicObject *const object)
    {
        Assert(object);
        AssertMsg(!this->IsObjectHeaderInlinedTypeHandler(), "Already ObjectHeaderInlined?");
        AssertMsg(!object->HasObjectArray(), "Can't move auxSlots to inline when we have ObjectArray");

        // When transition from ObjectHeaderInlined to auxSlot happend 2 properties where moved from inlineSlot to auxSlot
        // as we have to have space for auxSlot and objectArray (see DynamicTypeHandler::AdjustSlots). And then the new
        // property was added at 3rd index in auxSlot
        AssertMsg(this->GetUnusedBytesValue() - this->GetInlineSlotCapacity() == 3, "Should have only 3 values in auxSlot");

        // Get the auxSlot[0] and auxSlot[1] value as we will over write it
        Var auxSlotZero = object->GetAuxSlot(0);
        Var auxSlotOne = object->GetAuxSlot(1);

#ifdef EXPLICIT_FREE_SLOTS
        Var auxSlots = object->auxSlots;
        const int auxSlotsCapacity = this->GetSlotCapacity() - this->GetInlineSlotCapacity();
#endif
        // Move all current inline slots up to object header inline offset
        Var *const oldInlineSlots = reinterpret_cast<Var *>(reinterpret_cast<uintptr_t>(object) + this->GetOffsetOfInlineSlots());
        Field(Var) *const newInlineSlots = reinterpret_cast<Field(Var) *>(reinterpret_cast<uintptr_t>(object) + this->GetOffsetOfObjectHeaderInlineSlots());

        PHASE_PRINT_TRACE1(ObjectHeaderInliningPhase, _u("ObjectHeaderInlining: Re-optimizing the object. Moving auxSlots properties to ObjectHeader.\n"));

        PropertyIndex propertyIndex = 0;
        while (propertyIndex < this->GetInlineSlotCapacity())
        {
            newInlineSlots[propertyIndex] = oldInlineSlots[propertyIndex];
            propertyIndex++;
        }

        // auxSlot should only have 2 entry, move that to inlineSlot
        newInlineSlots[propertyIndex++] = auxSlotZero;
        newInlineSlots[propertyIndex++] = auxSlotOne;

#ifdef EXPLICIT_FREE_SLOTS
        object->GetRecycler()->ExplicitFreeNonLeaf(auxSlots, auxSlotsCapacity * sizeof(Var));
#endif

        Assert(this->GetPredecessorType()->GetTypeHandler()->IsPathTypeHandler());
        Assert(propertyIndex == ((PathTypeHandlerBase*)this->GetPredecessorType()->GetTypeHandler())->GetInlineSlotCapacity());
    }

    BOOL PathTypeHandlerBase::DeleteLastProperty(DynamicObject *const object)
    {
        DynamicType* predecessorType = this->GetPredecessorType();
        Assert(predecessorType != nullptr);

        // -----------------------------------------------------------------------------------------
        //         Current Type     |      Predecessor Type      |       Action
        // -----------------------------------------------------------------------------------------
        //    ObjectHeaderInlined   |    ObjectHeaderInlined     | No movement needed
        // -----------------------------------------------------------------------------------------
        //    ObjectHeaderInlined   |  Not ObjectHeaderInlined   | Not possible (Should be a BUG)
        // -----------------------------------------------------------------------------------------
        //  Not ObjectHeaderInlined |  Not ObjectHeaderInlined   | No movement needed
        // -----------------------------------------------------------------------------------------
        //  Not ObjectHeaderInlined |    ObjectHeaderInlined     | Move from auxSlots to inline slots (ReOptimize)
        // -----------------------------------------------------------------------------------------

        bool isCurrentTypeOHI = this->IsObjectHeaderInlinedTypeHandlerUnchecked();

        Assert(predecessorType->GetTypeHandler()->IsPathTypeHandler());
        PathTypeHandlerBase* predecessorTypeHandler = (PathTypeHandlerBase*)predecessorType->GetTypeHandler();

        Assert(predecessorTypeHandler->GetUnusedBytesValue() == (this->GetUnusedBytesValue() - 1));

        bool isPredecessorTypeOHI = predecessorTypeHandler->IsObjectHeaderInlinedTypeHandlerUnchecked();

        AssertMsg(!isCurrentTypeOHI || isPredecessorTypeOHI, "Current type is ObjectHeaderInlined but previous type is not ObjectHeaderInlined?");

        AssertMsg((isCurrentTypeOHI ^ isPredecessorTypeOHI) ||
            (this->GetInlineSlotCapacity() == predecessorTypeHandler->GetInlineSlotCapacity()),
            "When both types are ObjectHeaderInlined (or not ObjectHeaderInlined), InlineSlotCapacity of types should match");

        if (!isCurrentTypeOHI && isPredecessorTypeOHI)
        {
            if (object->HasObjectArray())
            {
                // We can't move auxSlots
                return FALSE;
            }

            Assert(predecessorTypeHandler->GetInlineSlotCapacity() == (this->GetUnusedBytesValue() - 1));
            this->MoveAuxSlotsToObjectHeader(object);
        }

        Assert(predecessorTypeHandler->GetSlotCapacity() <= this->GetSlotCapacity());

        // Another type (this) reached the old (predecessorType) type so share it.
        // ShareType will take care of invalidating fixed fields and removing singleton object from predecessorType
        predecessorType->ShareType();

#if ENABLE_FIXED_FIELDS
        this->GetTypePath()->ClearSingletonInstanceIfSame(object);
#endif

        object->ReplaceTypeWithPredecessorType(predecessorType);

        return TRUE;
    }

    BOOL PathTypeHandlerBase::DeleteProperty(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags flags)
    {
        // Check numeric propertyId only if objectArray available
        ScriptContext* scriptContext = instance->GetScriptContext();
        uint32 indexVal;
        if (instance->HasObjectArray() && scriptContext->IsNumericPropertyId(propertyId, &indexVal))
        {
            return PathTypeHandlerBase::DeleteItem(instance, indexVal, flags);
        }

        PropertyIndex index = PathTypeHandlerBase::GetPropertyIndex(propertyId);

        // If property is not found exit early
        if (index == Constants::NoSlot)
        {
            return TRUE;
        }

        ObjectSlotAttributes attr = this->GetAttributes(index);
        if (attr & ObjectSlotAttr_Deleted)
        {
            return TRUE;
        }
        if (!(attr & ObjectSlotAttr_Configurable))
        {
            JavascriptError::ThrowCantDeleteIfStrictModeOrNonconfigurable(
                flags, scriptContext, scriptContext->GetPropertyName(propertyId)->GetBuffer());
            return FALSE;
        }

        uint16 pathLength = GetPathLength();

        if ((index + 1) == pathLength &&
            this->GetPredecessorType() != nullptr &&
            this->DeleteLastProperty(instance))
        {
            return TRUE;
        }

#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertPathToDictionaryDeletedCount++;
#endif
        BOOL deleteResult = TryConvertToSimpleDictionaryType(instance, pathLength)->DeleteProperty(instance, propertyId, flags);

        AssertMsg(deleteResult, "PathType delete property can return false, this should be handled in DeleteLastProperty as well.");

        return deleteResult;
    }

    BOOL PathTypeHandlerBase::IsEnumerable(DynamicObject* instance, PropertyId propertyId)
    {
        return true;
    }

    BOOL PathTypeHandlerBase::IsWritable(DynamicObject* instance, PropertyId propertyId)
    {
        return true;
    }

    BOOL PathTypeHandlerBase::IsConfigurable(DynamicObject* instance, PropertyId propertyId)
    {
        return true;
    }

    BOOL PathTypeHandlerBase::SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (value)
        {
            return true;
        }

        if (PHASE_OFF1(ShareTypesWithAttributesPhase))
        {
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryAttributesCount++;
#endif
            return TryConvertToSimpleDictionaryType(instance, GetPathLength())->SetConfigurable(instance, propertyId, value);
        }

        // Find the property
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            // Upgrade type handler if set objectArray item attribute.
            // Only check numeric propertyId if objectArray available.
            if (instance->HasObjectArray())
            {
                PropertyRecord const* propertyRecord = instance->GetScriptContext()->GetPropertyName(propertyId);
                if (propertyRecord->IsNumeric())
                {
                    return ConvertToTypeWithItemAttributes(instance)->SetConfigurable(instance, propertyId, value);
                }
            }
            return true;
        }

        return SetAttributesHelper(instance, propertyId, propertyIndex, nullptr, (ObjectSlotAttributes)(ObjectSlotAttr_Default & ~ObjectSlotAttr_Configurable));
    }

    BOOL PathTypeHandlerBase::SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (value)
        {
            return true;
        }

        if (PHASE_OFF1(ShareTypesWithAttributesPhase))
        {
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryAttributesCount++;
#endif
            return TryConvertToSimpleDictionaryType(instance, GetPathLength())->SetEnumerable(instance, propertyId, value);
        }

        // Find the property
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            // Upgrade type handler if set objectArray item attribute.
            // Only check numeric propertyId if objectArray available.
            if (instance->HasObjectArray())
            {
                PropertyRecord const* propertyRecord = instance->GetScriptContext()->GetPropertyName(propertyId);
                if (propertyRecord->IsNumeric())
                {
                    return ConvertToTypeWithItemAttributes(instance)->SetEnumerable(instance, propertyId, value);
                }
            }
            return true;
        }

        return SetAttributesHelper(instance, propertyId, propertyIndex, nullptr, (ObjectSlotAttributes)(ObjectSlotAttr_Default & ~ObjectSlotAttr_Enumerable));
    }

    BOOL PathTypeHandlerBase::SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (value)
        {
            return true;
        }

        if (PHASE_OFF1(ShareTypesWithAttributesPhase))
        {
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryAttributesCount++;
#endif
            return TryConvertToSimpleDictionaryType(instance, GetPathLength())->SetWritable(instance, propertyId, value);
        }

        // Find the property
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            // Upgrade type handler if set objectArray item attribute.
            // Only check numeric propertyId if objectArray available.
            if (instance->HasObjectArray())
            {
                PropertyRecord const* propertyRecord = instance->GetScriptContext()->GetPropertyName(propertyId);
                if (propertyRecord->IsNumeric())
                {
                    return ConvertToTypeWithItemAttributes(instance)->SetWritable(instance, propertyId, value);
                }
            }
            return true;
        }

        return SetAttributesHelper(instance, propertyId, propertyIndex, nullptr, (ObjectSlotAttributes)(ObjectSlotAttr_Default & ~ObjectSlotAttr_Writable));
    }

    BOOL PathTypeHandlerBase::SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        return SetAccessorsHelper(instance, propertyId, nullptr, nullptr, getter, setter, flags);
    }

    BOOL PathTypeHandlerBase::SetAccessorsHelper(DynamicObject* instance, PropertyId propertyId, ObjectSlotAttributes * attributes, PathTypeSetterSlotIndex * setters, Var getter, Var setter, PropertyOperationFlags flags)
    {
        if (instance->GetType()->IsExternal() || instance->GetScriptContext()->IsScriptContextInDebugMode() || PHASE_OFF1(ShareAccessorTypesPhase))
        {
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryAccessorsCount++;
#endif
            return ConvertToDictionaryType(instance)->SetAccessors(instance, propertyId, getter, setter, flags);
        }

        PathTypeHandlerBase * newTypeHandler = this;
        ScriptContext *scriptContext = instance->GetScriptContext();
        JavascriptLibrary *library = instance->GetLibrary();

        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            PropertyRecord const* propertyRecord = scriptContext->GetPropertyName(propertyId);
            if (propertyRecord->IsNumeric())
            {
#ifdef PROFILE_TYPES
                instance->GetScriptContext()->convertPathToDictionaryItemAccessorsCount++;
#endif
                return ConvertToDictionaryType(instance)->SetItemAccessors(instance, propertyRecord->GetNumericValue(), getter, setter);
            }

            // We'll add 2 properties to the type, so check the limit.
            if (GetPathLength() + 2 > TypePath::MaxPathTypeHandlerLength)
            {
#ifdef PROFILE_TYPES
                instance->GetScriptContext()->convertPathToDictionaryAccessorsCount++;
#endif
                return ConvertToDictionaryType(instance)->SetAccessors(instance, propertyId, getter, setter, flags);
            }

            getter = CanonicalizeAccessor(getter, library);
            setter = CanonicalizeAccessor(setter, library);

            propertyIndex = GetPathLength();
            AddPropertyInternal(instance, propertyId, getter, (ObjectSlotAttributes)(ObjectSlotAttr_Default | ObjectSlotAttr_Accessor), nullptr, flags, SideEffects_None);
            newTypeHandler = PathTypeHandlerBase::FromTypeHandler(instance->GetTypeHandler());
            Assert(propertyIndex == newTypeHandler->GetTypePath()->LookupInline(propertyId, newTypeHandler->GetPathLength()));
        }
        else
        {
            ObjectSlotAttributes attr = attributes ? attributes[propertyIndex] : ObjectSlotAttr_Default;
            if (!(attr & ObjectSlotAttr_Accessor))
            {
                getter = CanonicalizeAccessor(getter, library);
                setter = CanonicalizeAccessor(setter, library);

                if (!setters || setters[propertyIndex] == NoSetterSlot)
                {
                    // We'll add 1 property to the type, so check the limit.
                    if (GetPathLength() + 1 > TypePath::MaxPathTypeHandlerLength)
                    {
#ifdef PROFILE_TYPES
                        instance->GetScriptContext()->convertPathToDictionaryAccessorsCount++;
#endif
                        return ConvertToDictionaryType(instance)->SetAccessors(instance, propertyId, getter, setter, flags);
                    }
                }

                SetAttributesHelper(instance, propertyId, propertyIndex, attributes, (ObjectSlotAttributes)(attr | ObjectSlotAttr_Accessor));
                // SetAttributesHelper can convert to dictionary in corner cases, e.g., if we haven't got a full path from the root. Remove this check when that's fixed.
                if (!instance->GetTypeHandler()->IsPathTypeHandler())
                {
                    return instance->GetTypeHandler()->SetAccessors(instance, propertyId, getter, setter, flags);
                }
                newTypeHandler = PathTypeHandlerBase::FromTypeHandler(instance->GetTypeHandler());
            }
            if (getter != nullptr)
            {
                newTypeHandler->SetSlotUnchecked(instance, propertyIndex, CanonicalizeAccessor(getter, library));
            }
        }

        // The type has been changed, if necessary, and the getter value has been set. Now see if the setter index needs to be set up.
        PathTypeSetterSlotIndex setterSlot = NoSetterSlot;
        if (newTypeHandler == this)
        {
            Assert(setters != nullptr);
            _Analysis_assume_(setters != nullptr);
            setterSlot = setters[propertyIndex];
        }
        else
        {
            // We may already have a valid setter slot, if the property has gone from accessor to data and now back to accessor.
            // Re-use the setter slot if we can.
            setters = PathTypeHandlerWithAttr::FromPathTypeHandler(newTypeHandler)->PathTypeHandlerWithAttr::GetSetterSlots();
            if (setters != nullptr)
            {
                setterSlot = setters[propertyIndex];
                Assert(setterSlot == NoSetterSlot || setterSlot >= GetPathLength() || attributes[setterSlot] == ObjectSlotAttr_Setter);
            }
        }

        if (setterSlot == NoSetterSlot || setterSlot >= GetPathLength())
        {
            PropertyIndex index = Constants::NoSlot;

            setterSlot = (PathTypeSetterSlotIndex)newTypeHandler->GetPathLength();
            DynamicType *newType = newTypeHandler->PromoteType<false>(instance->GetDynamicType(), PathTypeSuccessorKey(propertyId, ObjectSlotAttr_Setter), false, scriptContext, instance, &index);

            if (newType != instance->GetType())
            {
                instance->EnsureSlots(newTypeHandler->GetSlotCapacity(), newType->GetTypeHandler()->GetSlotCapacity(), scriptContext, newType->GetTypeHandler());
                instance->ReplaceType(newType);
            }
            newTypeHandler = PathTypeHandlerBase::FromTypeHandler(newType->GetTypeHandler());
            setters = PathTypeHandlerWithAttr::FromPathTypeHandler(newTypeHandler)->PathTypeHandlerWithAttr::GetSetterSlots();
            Assert(setters != nullptr);
            _Analysis_assume_(setters != nullptr);
            setters[propertyIndex] = setterSlot;
#if ENABLE_FIXED_FIELDS
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            newTypeHandler->InitializePath(
                instance, setterSlot, newTypeHandler->GetPathLength(), scriptContext, [=]() { return newTypeHandler->GetTypePath()->GetIsFixedFieldAt(setterSlot, newTypeHandler->GetPathLength()); });
#endif
#endif
        }

        if (setter != nullptr)
        {
            Assert(setters);
            newTypeHandler->SetSlotUnchecked(instance, setters[propertyIndex], CanonicalizeAccessor(setter, library));
        }

        newTypeHandler->ClearHasOnlyWritableDataProperties();
        if (this->GetFlags() & IsPrototypeFlag)
        {
            scriptContext->InvalidateProtoCaches(propertyId);
        }
        newTypeHandler->SetPropertyUpdateSideEffect(instance, propertyId, nullptr, SideEffects_Any);

        return true;
    }

    BOOL PathTypeHandlerBase::PreventExtensions(DynamicObject* instance)
    {
#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertPathToDictionaryExtensionsCount++;
#endif
        if (!CanConvertToSimpleDictionaryType())
        {
            return ConvertToDictionaryType(instance)->PreventExtensions(instance);
        }

        BOOL tempResult = this->ConvertToSharedNonExtensibleTypeIfNeededAndCallOperation(instance, InternalPropertyRecords::NonExtensibleType,
            [&](SimpleDictionaryTypeHandlerWithNonExtensibleSupport* newTypeHandler)
            {
                return newTypeHandler->PreventExtensionsInternal(instance);
            });

        Assert(tempResult);
        if (tempResult)
        {
            // Call preventExtensions on the objectArray -- which will create different type for array type handler.
            ArrayObject * objectArray = instance->GetObjectArray();
            if (objectArray)
            {
                objectArray->PreventExtensions();
            }
        }

        return tempResult;
    }

    BOOL PathTypeHandlerBase::Seal(DynamicObject* instance)
    {
#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertPathToDictionaryExtensionsCount++;
#endif
        // For seal we need an array with non-default attributes, which is ES5Array,
        // and in current design ES5Array goes side-by-side with DictionaryTypeHandler.
        // Note that 2 instances can have same PathTypehandler but still different objectArray items, e.g. {x:0, 0:0} and {x:0, 1:0}.
        // Technically we could change SimpleDictionaryTypehandler to override *Item* methods,
        // similar to DictionaryTypeHandler, but objects with numeric properties are currently seen as low priority,
        // so just don't share the type.
        if (instance->HasObjectArray() || !this->CanConvertToSimpleDictionaryType())
        {
            return this->ConvertToDictionaryType(instance)->Seal(instance);
        }
        else
        {
            return this->ConvertToSharedNonExtensibleTypeIfNeededAndCallOperation(instance, InternalPropertyRecords::SealedType,
                [&](SimpleDictionaryTypeHandlerWithNonExtensibleSupport* newTypeHandler)
                {
                    return newTypeHandler->SealInternal(instance);
                });
        }
    }

    BOOL PathTypeHandlerBase::FreezeImpl(DynamicObject* instance, bool isConvertedType)
    {
#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertPathToDictionaryExtensionsCount++;
#endif
        // See the comment inside Seal WRT HasObjectArray branch.
        if (instance->HasObjectArray() || !this->CanConvertToSimpleDictionaryType())
        {
            return this->ConvertToDictionaryType(instance)->Freeze(instance, isConvertedType);
        }
        else
        {
            return this->ConvertToSharedNonExtensibleTypeIfNeededAndCallOperation(instance, InternalPropertyRecords::FrozenType,
                [&](SimpleDictionaryTypeHandlerWithNonExtensibleSupport* newTypeHandler)
                {
                    return newTypeHandler->FreezeInternal(instance, true);  // true: we don't want to change type in FreezeInternal.
                });
        }
    }

    // Checks whether conversion to shared type is needed and performs it, then calls actual operation on the shared type.
    // Template method used for PreventExtensions, Seal, Freeze.
    // Parameters:
    // - instance: object instance to operate on.
    // - operationInternalPropertyRecord: the internal property record for preventExtensions/seal/freeze.
    // - FType: functor/lambda to perform actual forced operation (such as PreventExtensionsInternal) on the shared type.
    template<typename FType>
    BOOL PathTypeHandlerBase::ConvertToSharedNonExtensibleTypeIfNeededAndCallOperation(DynamicObject* instance, const PropertyRecord* operationInternalPropertyRecord, FType operation)
    {
        AssertMsg(operationInternalPropertyRecord == InternalPropertyRecords::NonExtensibleType ||
            operationInternalPropertyRecord == InternalPropertyRecords::SealedType ||
            operationInternalPropertyRecord == InternalPropertyRecords::FrozenType,
            "Wrong/unsupported value of operationInternalPropertyRecord.");

        RecyclerWeakReference<DynamicType>* newTypeWeakRef = nullptr;
        DynamicType * oldType = instance->GetDynamicType();
        PathTypeSuccessorKey key(operationInternalPropertyRecord->GetPropertyId(), ObjectSlotAttr_Default);

        // See if we already have shared type for this type and convert to it, otherwise create a new one.
        if (!GetSuccessor(key, &newTypeWeakRef) || newTypeWeakRef->Get() == nullptr)
        {
            Assert(CanConvertToSimpleDictionaryType());

            // Convert to new shared type with shared simple dictionary type handler and call operation on it.
            SimpleDictionaryTypeHandlerWithNonExtensibleSupport * newTypeHandler =
                ConvertToSimpleDictionaryType<SimpleDictionaryTypeHandlerWithNonExtensibleSupport>(instance, this->GetPathLength(), true);

            Assert(newTypeHandler->GetMayBecomeShared() && !newTypeHandler->GetIsShared());
            DynamicType* newType = instance->GetDynamicType();
            newType->LockType();
            Assert(!newType->GetIsShared());

            ScriptContext * scriptContext = instance->GetScriptContext();
            Recycler * recycler = scriptContext->GetRecycler();
            SetSuccessor(oldType, key, recycler->CreateWeakReferenceHandle<DynamicType>(newType), scriptContext);
            return operation(newTypeHandler);
        }
        else
        {
            DynamicType* newType = newTypeWeakRef->Get();
            DynamicTypeHandler* newTypeHandler = newType->GetTypeHandler();

            // Consider: Consider doing something special for frozen objects, whose values cannot
            // change and so we could retain them as fixed, even when the type becomes shared.
            newType->ShareType();
            // Consider: If we isolate prototypes, we should never get here with the prototype flag set.
            // There should be nothing to transfer.
            // Assert(!IsolatePrototypes() || (this->GetFlags() & IsPrototypeFlag) == 0);
            newTypeHandler->SetFlags(IsPrototypeFlag, this->GetFlags());
#if ENABLE_FIXED_FIELDS
            Assert(!newTypeHandler->HasSingletonInstance());
#endif

            if(instance->IsObjectHeaderInlinedTypeHandler())
            {
                const PropertyIndex newInlineSlotCapacity = newTypeHandler->GetInlineSlotCapacity();
                AdjustSlots(instance, newInlineSlotCapacity, newTypeHandler->GetSlotCapacity() - newInlineSlotCapacity);
            }
            ReplaceInstanceType(instance, newType);
        }

        return TRUE;
    }

    DynamicType* PathTypeHandlerBase::PromoteType(DynamicObject* instance, const PathTypeSuccessorKey key, PropertyIndex* propertyIndex)
    {
        ScriptContext* scriptContext = instance->GetScriptContext();
        DynamicType* currentType = instance->GetDynamicType();

        DynamicType* nextType = this->PromoteType<false>(currentType, key, false, scriptContext, instance, propertyIndex);
        PathTypeHandlerBase* nextPath = (PathTypeHandlerBase*) nextType->GetTypeHandler();

        instance->EnsureSlots(this->GetSlotCapacity(), nextPath->GetSlotCapacity(), scriptContext, nextType->GetTypeHandler());

        ReplaceInstanceType(instance, nextType);
        return nextType;
    }

    template <typename T>
    T* PathTypeHandlerBase::ConvertToTypeHandler(DynamicObject* instance)
    {
        Assert(instance);
        ScriptContext* scriptContext = instance->GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();

        instance->PrepareForConversionToNonPathType();

        PathTypeHandlerBase * oldTypeHandler;

        // Ideally 'this' and oldTypeHandler->GetTypeHandler() should be same
        // But we can have calls from external DOM objects, which requests us to replace the type of the
        // object with a new type. And in such cases, this API gets called with oldTypeHandler and the
        // new type (obtained from the External DOM object)
        // We use the duplicated typeHandler, if we deOptimized the object successfully, else we retain the earlier
        // behavior of using 'this' pointer.

        if (instance->DeoptimizeObjectHeaderInlining())
        {
            oldTypeHandler = reinterpret_cast<PathTypeHandlerBase *>(instance->GetTypeHandler());
        }
        else
        {
            oldTypeHandler = this;
        }

        Assert(oldTypeHandler);

        T* newTypeHandler = RecyclerNew(recycler, T, recycler, oldTypeHandler->GetSlotCapacity(), oldTypeHandler->GetInlineSlotCapacity(), oldTypeHandler->GetOffsetOfInlineSlots());
        // We expect the new type handler to start off marked as having only writable data properties.
        Assert(newTypeHandler->GetHasOnlyWritableDataProperties());

#if ENABLE_FIXED_FIELDS
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        DynamicType* oldType = instance->GetDynamicType();
        RecyclerWeakReference<DynamicObject>* oldSingletonInstance = oldTypeHandler->GetSingletonInstance();
        oldTypeHandler->TraceFixedFieldsBeforeTypeHandlerChange(_u("converting"), _u("PathTypeHandler"), _u("DictionaryTypeHandler"), instance, oldTypeHandler, oldType, oldSingletonInstance);
#endif

        bool const canBeSingletonInstance = DynamicTypeHandler::CanBeSingletonInstance(instance);
        // If this type had been installed on a stack instance it shouldn't have a singleton Instance
        Assert(canBeSingletonInstance || !oldTypeHandler->HasSingletonInstance());

        // This instance may not be the singleton instance for this handler. There may be a singleton at the tip
        // and this instance may be getting initialized via an object literal and one of the properties may
        // be an accessor.  In this case we will convert to a DictionaryTypeHandler and it's correct to
        // transfer this instance, even tough different from the singleton. Ironically, this instance
        // may even appear to be at the tip along with the other singleton, because the set of properties (by
        // name, not value) may be identical.
        // Consider: Consider threading PropertyOperation_Init through InitProperty and SetAccessors,
        // to be sure that we don't assert only in this narrow case.
        // Assert(this->typePath->GetSingletonInstance() == instance);

        Assert(oldTypeHandler->HasSingletonInstanceOnlyIfNeeded());

        // Don't install stack instance as singleton instance
        if (canBeSingletonInstance)
        {
            if (DynamicTypeHandler::AreSingletonInstancesNeeded())
            {
                RecyclerWeakReference<DynamicObject>* curSingletonInstance = oldTypeHandler->GetTypePath()->GetSingletonInstance();
                if (curSingletonInstance != nullptr && curSingletonInstance->Get() == instance)
                {
                    newTypeHandler->SetSingletonInstance(curSingletonInstance);
                }
                else
                {
                    newTypeHandler->SetSingletonInstance(instance->CreateWeakReferenceToSelf());
                }
            }
        }

        bool transferFixed = canBeSingletonInstance;

        // If we are a prototype or may become a prototype we must transfer used as fixed bits.  See point 4 in ConvertToSimpleDictionaryType.
        Assert(!DynamicTypeHandler::IsolatePrototypes() || ((oldTypeHandler->GetFlags() & IsPrototypeFlag) == 0));
        bool transferUsedAsFixed = ((oldTypeHandler->GetFlags() & IsPrototypeFlag) != 0 || (oldTypeHandler->GetIsOrMayBecomeShared() && !DynamicTypeHandler::IsolatePrototypes()));
#endif

        ObjectSlotAttributes * attributes = this->GetAttributeArray();
        TypePath * typePath = oldTypeHandler->GetTypePath();
        for (PropertyIndex i = 0; i < oldTypeHandler->GetPathLength(); i++)
        {
            ObjectSlotAttributes attr = attributes ? attributes[i] : ObjectSlotAttr_Default;
            const PropertyRecord * propertyRecord = typePath->GetPropertyId(i);
            if (attr == ObjectSlotAttr_Setter)
            {
                // Adding a setter. Don't add another descriptor. Find the getter and convert its descriptor, which will cause
                // the setter to get the next free slot.
                DictionaryPropertyDescriptor<PropertyIndex> *descriptor;
                bool result = newTypeHandler->propertyMap->TryGetReference(propertyRecord, &descriptor);
                Assert(result);
                if (!(attributes[descriptor->GetDataPropertyIndex<false>()] & ObjectSlotAttr_Accessor))
                {
                    // Setter without a getter; this is a stale entry, so ignore it
                    // Just consume the slot so no descriptor refers to it.
                    Assert(i == newTypeHandler->nextPropertyIndex);
                    ::Math::PostInc(newTypeHandler->nextPropertyIndex);
                    continue;
                }
                Assert(oldTypeHandler->GetSetterSlotIndex(descriptor->GetDataPropertyIndex<false>()) == newTypeHandler->nextPropertyIndex);
                descriptor->ConvertToGetterSetter(newTypeHandler->nextPropertyIndex);
                newTypeHandler->ClearHasOnlyWritableDataProperties();
            }
            else
            {
#if ENABLE_FIXED_FIELDS
                // Consider: As noted in point 2 in ConvertToSimpleDictionaryType, when converting to non-shared handler we could be more
                // aggressive and mark every field as fixed, because we will always take a type transition. We have to remember to respect
                // the switches as to which kinds of properties we should fix, and for that we need the values from the instance. Even if
                // the type handler says the property is initialized, the current instance may not have a value for it. Check for value != null.
                if (PathTypeHandlerBase::FixPropsOnPathTypes())
                {
                    newTypeHandler->Add(propertyRecord, ObjectSlotAttributesToPropertyAttributes(attr),
                        i < typePath->GetMaxInitializedLength(),
                        transferFixed && typePath->GetIsFixedFieldAt(i, oldTypeHandler->GetPathLength()),
                        transferUsedAsFixed && typePath->GetIsUsedFixedFieldAt(i, oldTypeHandler->GetPathLength()),
                        scriptContext);
                }
                else
#endif
                {
                    newTypeHandler->Add(propertyRecord, ObjectSlotAttributesToPropertyAttributes(attr), true, false, false, scriptContext);
                }
            }
        }

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        if (PathTypeHandlerBase::FixPropsOnPathTypes())
        {
            Assert(oldTypeHandler->HasSingletonInstanceOnlyIfNeeded());
            oldTypeHandler->GetTypePath()->ClearSingletonInstanceIfSame(instance);
        }
#endif

        // PathTypeHandlers are always shared, so if we're isolating prototypes, a PathTypeHandler should
        // never have the prototype flag set.
        Assert(!DynamicTypeHandler::IsolatePrototypes() || ((oldTypeHandler->GetFlags() & IsPrototypeFlag) == 0));
        AssertMsg(!newTypeHandler->GetIsPrototype(), "Why did we create a brand new type handler with a prototype flag set?");
        newTypeHandler->SetFlags(IsPrototypeFlag, oldTypeHandler->GetFlags());

        // Any new type handler we expect to see here should have inline slot capacity locked.  If this were to change, we would need
        // to update our shrinking logic (see ShrinkSlotAndInlineSlotCapacity).
        Assert(newTypeHandler->GetIsInlineSlotCapacityLocked());
        newTypeHandler->SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, oldTypeHandler->GetPropertyTypes());
        newTypeHandler->SetInstanceTypeHandler(instance);

#if ENABLE_FIXED_FIELDS
        Assert(!newTypeHandler->HasSingletonInstance() || !instance->HasSharedType());

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        PathTypeHandlerBase::TraceFixedFieldsAfterTypeHandlerChange(instance, oldTypeHandler, newTypeHandler, oldType, instance->GetDynamicType(), oldSingletonInstance);
#endif
#endif

        return newTypeHandler;
    }

    DictionaryTypeHandler* PathTypeHandlerBase::ConvertToDictionaryType(DynamicObject* instance)
    {
        return ConvertToTypeHandler<DictionaryTypeHandler>(instance);
    }

    ES5ArrayTypeHandler* PathTypeHandlerBase::ConvertToES5ArrayType(DynamicObject* instance)
    {
        return ConvertToTypeHandler<ES5ArrayTypeHandler>(instance);
    }

    DynamicTypeHandler* PathTypeHandlerBase::ConvertToNonShareableTypeHandler(DynamicObject* instance)
    {
        return TryConvertToSimpleDictionaryType<SimpleDictionaryTypeHandler>(instance, GetPathLength(), false);
    }

    template <typename T>
    DynamicTypeHandler * PathTypeHandlerBase::TryConvertToSimpleDictionaryType(DynamicObject* instance, int propertyCapacity, bool mayBecomeShared)
    {
        if (CanConvertToSimpleDictionaryType())
        {
            return ConvertToSimpleDictionaryType<T>(instance, propertyCapacity, mayBecomeShared);
        }
        return ConvertToDictionaryType(instance);
    }

    template <typename T>
    T* PathTypeHandlerBase::ConvertToSimpleDictionaryType(DynamicObject* instance, int propertyCapacity, bool mayBecomeShared)
    {
        Assert(CanConvertToSimpleDictionaryType());
        Assert(instance);
        ScriptContext* scriptContext = instance->GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();

        instance->PrepareForConversionToNonPathType();

        // Ideally 'this' and oldTypeHandler->GetTypeHandler() should be same
        // But we can have calls from external DOM objects, which requests us to replace the type of the
        // object with a new type. And in such cases, this API gets called with oldTypeHandler and the
        // new type (obtained from the External DOM object)
        // We use the duplicated typeHandler, if we deOptimized the object successfully, else we retain the earlier
        // behavior of using 'this' pointer.

        PathTypeHandlerBase * oldTypeHandler = nullptr;

        if (instance->DeoptimizeObjectHeaderInlining())
        {
            Assert(instance->GetTypeHandler()->IsPathTypeHandler());
            oldTypeHandler = reinterpret_cast<PathTypeHandlerBase *>(instance->GetTypeHandler());
        }
        else
        {
            oldTypeHandler = this;
        }

        Assert(oldTypeHandler);

#if ENABLE_FIXED_FIELDS
        DynamicType* oldType = instance->GetDynamicType();
#endif
        T* newTypeHandler = RecyclerNew(recycler, T, recycler, oldTypeHandler->GetSlotCapacity(), propertyCapacity, oldTypeHandler->GetInlineSlotCapacity(), oldTypeHandler->GetOffsetOfInlineSlots());
        // We expect the new type handler to start off marked as having only writable data properties.
        Assert(newTypeHandler->GetHasOnlyWritableDataProperties());

        // Care must be taken to correctly set up fixed field bits whenever a type's handler is changed.  Exactly what needs to
        // be done depends on whether the current handler is shared, whether the new handler is shared, whether the current
        // handler has the prototype flag set, and even whether we take a type transition as part of the process.
        //
        // 1. Can we set fixed bits on new handler for the fields that are marked as fixed on current handler?
        //
        //    Yes, if the new type handler is not shared.  If the handler is not shared, we know that only this instance will
        //    ever use it.  Otherwise, a different instance could transition to the same type handler, but have different values
        //    for fields marked as fixed.
        //
        // 2. Can we set fixed bits on new handler even for the fields that are not marked as fixed on current handler?
        //
        //    Yes, if the new type handler is not shared and we take a type transition during conversion.  The first condition
        //    is required for the same reason as in point 1 above.  The type transition is needed to ensure that any store
        //    field fast paths for this instance get invalidated.  If they didn't, then the newly fixed field could get
        //    overwritten on the fast path without triggering necessary invalidation.
        //
        //    Note that it's desirable to mark additional fields as fixed (particularly when the instance becomes a prototype)
        //    to counteract the effect of false type sharing, which may unnecessarily turn off some fixed field bits.
        //
        // 3. Do we need to clear any fixed field bits on the old or new type handler?
        //
        //    Yes, we must clear fixed fields bits for properties that aren't also used as fixed, but only if both type handlers
        //    are shared and we don't isolate prototypes.  This is rather tricky and results from us pre-creating certain handlers
        //    even before any instances actually have values for all represented properties.  We must avoid the situation, in which
        //    one instance switched to a new type handler with some fixed field not yet used as fixed, and later the second
        //    instance follows the same handler evolution with the same field used as fixed.  Upon switching to the new handler
        //    the second instance would "forget" that the field was used as fixed and fail to invalidate when overwritten.
        //
        //    Example: Instance A with TH1 has a fixed method FOO, which has not been used as fixed yet.  Then instance B gets
        //    pre-created and lands on TH1 (and so far assumes FOO is fixed).  As B's pre-creation continues, it moves to TH2, but
        //    thus far FOO has not been used as fixed.  Now instance A becomes a prototype, and its method FOO is used in a hard-coded
        //    JIT sequence, thus marking it as used as fixed.  Instance A then transitions to TH2 and we lose track of FOO being used
        //    as fixed.  If FOO is then overwritten on A, the hard-coded JIT sequence does not get invalidated and continues to call
        //    the old method FOO.
        //
        // 4. Can we avoid setting used as fixed bits on new handler for fields marked as used as fixed on current handler?
        //
        //    Yes, if the current type handler doesn't have the prototype flag and current handler is not shared or new handler
        //    is not shared or we isolate prototypes, and we take a type transition as part of the conversion.
        //
        //    Type transition ensures that any field loads from the instance are invalidated (including
        //    any that may have hard-coded the fixed field's value).  Hence, if the fixed field on this instance were to be later
        //    overwritten it will not cause any functional issues.  On the other hand, field loads from prototype are not affected
        //    by the prototype object's type change.  Therefore, if this instance is a prototype we must carry the used as fixed
        //    bits forward to ensure that if we overwrite any fixed field we explicitly trigger invalidation.
        //
        //    Review: Actually, the comment below is overly conservative.  If the second instance that became a prototype
        //    followed the same type evolution path, it would have to have invalidated all fixed fields, so there should be no need
        //    to transfer used as fixed bits, unless the current instance is already a prototype.
        //    In addition, if current handler is shared and the new handler is shared, a different instance with the current handler
        //    may later become a prototype (if we don't isolate prototypes) and follow the same conversion to the new handler, even
        //    if the current instance is not a prototype.  Hence, the new type handler must retain the used as fixed bits, so that
        //    proper invalidation can be triggered later, if overwritten.
        //
        //    Note that this may lead to the new type handler with some fields not marked as fixed, but marked as used as fixed.
        //
        //    Note also that if we isolate prototypes, we guarantee that no prototype instance will share a type handler with any
        //    other instance.  Hence, the problem sequence above could not take place.
        //
        // 5. Do we need to invalidate JIT-ed code for any fields marked as used as fixed on current handler?
        //
        //    No.  With the rules above any necessary invalidation will be triggered when the value actually gets overwritten.
        //

#if ENABLE_FIXED_FIELDS
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        RecyclerWeakReference<DynamicObject>* oldSingletonInstance = oldTypeHandler->GetSingletonInstance();
        oldTypeHandler->TraceFixedFieldsBeforeTypeHandlerChange(_u("converting"), _u("PathTypeHandler"), _u("SimpleDictionaryTypeHandler"), instance, oldTypeHandler, oldType, oldSingletonInstance);
#endif

        bool const canBeSingletonInstance = DynamicTypeHandler::CanBeSingletonInstance(instance);
        // If this type had been installed on a stack instance it shouldn't have a singleton Instance
        Assert(canBeSingletonInstance || !oldTypeHandler->HasSingletonInstance());

        // Consider: It looks like we're delaying sharing of these type handlers until the second instance arrives, so we could
        // set the singleton here and zap it later.
        if (!mayBecomeShared && canBeSingletonInstance)
        {
            Assert(oldTypeHandler->HasSingletonInstanceOnlyIfNeeded());
            if (DynamicTypeHandler::AreSingletonInstancesNeeded())
            {
                RecyclerWeakReference<DynamicObject>* curSingletonInstance = oldTypeHandler->GetTypePath()->GetSingletonInstance();
                if (curSingletonInstance != nullptr && curSingletonInstance->Get() == instance)
                {
                    newTypeHandler->SetSingletonInstance(curSingletonInstance);
                }
                else
                {
                    newTypeHandler->SetSingletonInstance(instance->CreateWeakReferenceToSelf());
                }
            }
        }

        // It would be nice to transfer fixed fields if the new type handler may become fixed later (but isn't yet).  This would allow
        // singleton instances to retain fixed fields.  It would require that when we do actually share the target type (when the second
        // instance arrives), we clear (and invalidate, if necessary) any fixed fields.  This may be a reasonable trade-off.
        bool transferIsFixed = !mayBecomeShared && canBeSingletonInstance;

        // If we are a prototype or may become a prototype we must transfer used as fixed bits.  See point 4 above.
        Assert(!DynamicTypeHandler::IsolatePrototypes() || ((oldTypeHandler->GetFlags() & IsPrototypeFlag) == 0));
        // For the global object we don't emit a type check before a hard-coded use of a fixed field.  Therefore a type transition isn't sufficient to
        // invalidate any used fixed fields, and we must continue tracking them on the new type handler.  The global object should never have a path
        // type handler.
        Assert(instance->GetTypeId() != TypeIds_GlobalObject);
        // If the type isn't locked, we may not change the type of the instance, and we must also track the used fixed fields on the new handler.
        bool transferUsedAsFixed = !instance->GetDynamicType()->GetIsLocked() || ((oldTypeHandler->GetFlags() & IsPrototypeFlag) != 0 || (oldTypeHandler->GetIsOrMayBecomeShared() && !DynamicTypeHandler::IsolatePrototypes()));
#endif

        // Consider: As noted in point 2 above, when converting to non-shared SimpleDictionaryTypeHandler we could be more aggressive
        // and mark every field as fixed, because we will always take a type transition.  We have to remember to respect the switches as
        // to which kinds of properties we should fix, and for that we need the values from the instance.  Even if the type handler
        // says the property is initialized, the current instance may not have a value for it.  Check for value != null.

        ObjectSlotAttributes * attributes = this->GetAttributeArray();
        for (PropertyIndex i = 0; i < oldTypeHandler->GetPathLength(); i++)
        {
#if ENABLE_FIXED_FIELDS
            if (PathTypeHandlerBase::FixPropsOnPathTypes())
            {
                Js::TypePath * typePath = oldTypeHandler->GetTypePath();
                newTypeHandler->Add(typePath->GetPropertyId(i), attributes ? ObjectSlotAttributesToPropertyAttributes(attributes[i]) : PropertyDynamicTypeDefaults,
                    i < typePath->GetMaxInitializedLength(),
                    transferIsFixed && typePath->GetIsFixedFieldAt(i, GetPathLength()),
                    transferUsedAsFixed && typePath->GetIsUsedFixedFieldAt(i, GetPathLength()),
                    scriptContext);
            }
            else
#endif
            {
                newTypeHandler->Add(oldTypeHandler->GetTypePath()->GetPropertyId(i), attributes ? ObjectSlotAttributesToPropertyAttributes(attributes[i]) : PropertyDynamicTypeDefaults, true, false, false, scriptContext);
            }

            // No need to clear fixed fields not used as fixed, because we never convert during pre-creation of type handlers and we always
            // add properties in order they appear on the type path.  Hence, any existing fixed fields will be turned off by any other
            // instance following this type path.  See point 3 above.
        }

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        // Clear the singleton from this handler regardless of mayBecomeShared, because this instance no longer uses this handler.
        if (PathTypeHandlerBase::FixPropsOnPathTypes())
        {
            Assert(oldTypeHandler->HasSingletonInstanceOnlyIfNeeded());
            oldTypeHandler->GetTypePath()->ClearSingletonInstanceIfSame(instance);
        }
#endif

        if (mayBecomeShared)
        {
            newTypeHandler->SetFlags(IsLockedFlag | MayBecomeSharedFlag);
        }

        Assert(!DynamicTypeHandler::IsolatePrototypes() || !oldTypeHandler->GetIsOrMayBecomeShared() || ((oldTypeHandler->GetFlags() & IsPrototypeFlag) == 0));
        AssertMsg((newTypeHandler->GetFlags() & IsPrototypeFlag) == 0, "Why did we create a brand new type handler with a prototype flag set?");
        newTypeHandler->SetFlags(IsPrototypeFlag, oldTypeHandler->GetFlags());

        // Any new type handler we expect to see here should have inline slot capacity locked.  If this were to change, we would need
        // to update our shrinking logic (see ShrinkSlotAndInlineSlotCapacity).
        Assert(newTypeHandler->GetIsInlineSlotCapacityLocked());
        newTypeHandler->SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, oldTypeHandler->GetPropertyTypes());
        newTypeHandler->SetInstanceTypeHandler(instance);

#if ENABLE_FIXED_FIELDS
        Assert(!newTypeHandler->HasSingletonInstance() || !instance->HasSharedType());
        // We assumed that we don't need to transfer used as fixed bits unless we are a prototype, which is only valid if we also changed the type.
        Assert(transferUsedAsFixed || (instance->GetType() != oldType && oldType->GetTypeId() != TypeIds_GlobalObject));

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        PathTypeHandlerBase::TraceFixedFieldsAfterTypeHandlerChange(instance, oldTypeHandler, newTypeHandler, oldType, instance->GetDynamicType(), oldSingletonInstance);
#endif
#endif

#ifdef PROFILE_TYPES
        scriptContext->convertPathToSimpleDictionaryCount++;
#endif
        return newTypeHandler;
    }

    BOOL PathTypeHandlerBase::SetPropertyWithAttributes(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        if (!ObjectSlotAttributesContains(attributes) || PHASE_OFF1(ShareTypesWithAttributesPhase))
        {
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryAttributesCount++;
#endif

            return TryConvertToSimpleDictionaryType(instance, GetPathLength() + 1)->SetPropertyWithAttributes(instance, propertyId, value, attributes, info, flags, possibleSideEffects);
        }

        PropertyIndex index = PathTypeHandlerBase::GetPropertyIndex(propertyId);
        return PathTypeHandlerBase::SetPropertyInternal<true>(instance, propertyId, index, value, PropertyAttributesToObjectSlotAttributes(attributes), info, flags, possibleSideEffects);
    }

    BOOL PathTypeHandlerBase::SetAttributes(DynamicObject* instance, PropertyId propertyId, PropertyAttributes attributes)
    {
        if (!ObjectSlotAttributesContains(attributes) || PHASE_OFF1(ShareTypesWithAttributesPhase))
        {
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryAttributesCount++;
#endif

            return TryConvertToSimpleDictionaryType(instance, GetPathLength())->SetAttributes(instance, propertyId, attributes);
        }

        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            if (instance->HasObjectArray() && attributes != PropertyDynamicTypeDefaults)
            {
                const PropertyRecord * propertyRecord = instance->GetScriptContext()->GetPropertyName(propertyId);
                if (propertyRecord->IsNumeric())
                {
                    this->ConvertToTypeWithItemAttributes(instance)->SetItemAttributes(instance, propertyRecord->GetNumericValue(), attributes);
                }
            }
            return true;
        }

        return SetAttributesAtIndex(instance, propertyId, propertyIndex, attributes);
    }

    BOOL PathTypeHandlerBase::SetAttributesAtIndex(DynamicObject* instance, PropertyId propertyId, PropertyIndex index, PropertyAttributes attributes)
    {
        return SetAttributesHelper(instance, propertyId, index, GetAttributeArray(), PropertyAttributesToObjectSlotAttributes(attributes));
    }

    BOOL PathTypeHandlerBase::GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes)
    {
        if (index < this->GetPathLength())
        {
            Assert(this->GetPropertyId(instance->GetScriptContext(), index) == propertyId);
            *attributes = PropertyDynamicTypeDefaults;
            return true;
        }
        return false;
    }

    bool PathTypeHandlerBase::UsePathTypeHandlerForObjectLiteral(
        const PropertyIdArray *const propIds,
        bool *const check__proto__Ref)
    {
        Assert(propIds);

        // Always check __proto__ entry, now that object literals always honor __proto__
        const bool check__proto__ = propIds->has__proto__;
        if (check__proto__Ref)
        {
            *check__proto__Ref = check__proto__;
        }

        return !check__proto__ && propIds->count < TypePath::MaxPathTypeHandlerLength && !propIds->hadDuplicates;
    }

    DynamicType* PathTypeHandlerBase::CreateTypeForNewScObject(ScriptContext* scriptContext, DynamicType* type, const Js::PropertyIdArray *propIds, bool shareType)
    {
        Assert(scriptContext);
        uint count = propIds->count;

        bool check__proto__;
        if (UsePathTypeHandlerForObjectLiteral(propIds, &check__proto__))
        {
#ifdef PROFILE_OBJECT_LITERALS
            scriptContext->objectLiteralCount[count]++;
#endif
            for (uint i = 0; i < count; i++)
            {
                PathTypeHandlerBase *pathHandler = (PathTypeHandlerBase *)PointerValue(type->typeHandler);
                Js::PropertyId propertyId = propIds->elements[i];

                PropertyIndex propertyIndex = pathHandler->GetPropertyIndex(propertyId);

                if (propertyIndex != Constants::NoSlot)
                {
                    continue;
                }

#ifdef PROFILE_OBJECT_LITERALS
                {
                    RecyclerWeakReference<DynamicType>* nextTypeWeakRef;
                    if (!pathHandler->GetSuccessor(PathTypeSuccessorKey(propertyId, ObjectSlotAttr_Default), &nextTypeWeakRef) || nextTypeWeakRef->Get() == nullptr)
                    {
                        scriptContext->objectLiteralPathCount++;
                    }
                }
#endif
                type = pathHandler->PromoteType<true>(type, PathTypeSuccessorKey(propertyId, ObjectSlotAttr_Default), shareType, scriptContext, nullptr, &propertyIndex);
            }
        }
        else if (count <= static_cast<uint>(SimpleDictionaryTypeHandler::MaxPropertyIndexSize))
        {
            type = SimpleDictionaryTypeHandler::CreateTypeForNewScObject(scriptContext, type, propIds, shareType, check__proto__);
        }
        else if (count <= static_cast<uint>(BigSimpleDictionaryTypeHandler::MaxPropertyIndexSize))
        {
            type = BigSimpleDictionaryTypeHandler::CreateTypeForNewScObject(scriptContext, type, propIds, shareType, check__proto__);
        }
        else
        {
            Throw::OutOfMemory();
        }

        return type;
    }

    template <bool skipLetAttrForArguments>
    DynamicType * PathTypeHandlerBase::CreateNewScopeObject(ScriptContext *scriptContext, DynamicType *type, const PropertyIdArray *propIds, PropertyAttributes extraAttributes, uint extraAttributesSlotCount)
    {
        uint count = propIds->count;

        Recycler* recycler = scriptContext->GetRecycler();

        SimpleDictionaryTypeHandler* typeHandler = SimpleDictionaryTypeHandler::New(recycler, count, 0, 0, true, true);

        for (uint i = 0; i < count; i++)
        {
            PropertyId propertyId = propIds->elements[i];
            const PropertyRecord* propertyRecord = propertyId == Constants::NoProperty ? NULL : scriptContext->GetPropertyName(propertyId);
            // This will add the property as initialized and non-fixed.  That's fine because we will populate the property values on the
            // scope object right after this (see JavascriptOperators::OP_InitCachedScope).  We will not treat these properties as fixed.
            PropertyAttributes attributes = PropertyWritable | PropertyEnumerable;
            if (i < extraAttributesSlotCount)
            {
                attributes |= extraAttributes;
                if (skipLetAttrForArguments && propertyId == PropertyIds::arguments)
                {
                    // Do not add let attribute for built-in arguments symbol
                    attributes &= ~PropertyLet;
                }
            }
            typeHandler->Add(propertyRecord, attributes, scriptContext);
        }
        AssertMsg((typeHandler->GetFlags() & IsPrototypeFlag) == 0, "Why does a newly created type handler have the IsPrototypeFlag set?");

 #ifdef PROFILE_OBJECT_LITERALS
        scriptContext->objectLiteralSimpleDictionaryCount++;
 #endif

        type = RecyclerNew(recycler, DynamicType, type, typeHandler, /* isLocked = */ true, /* isShared = */ true);

        return type;
    }
    template DynamicType * PathTypeHandlerBase::CreateNewScopeObject<true>(ScriptContext *scriptContext, DynamicType *type, const PropertyIdArray *propIds, PropertyAttributes extraAttributes, uint extraAttributesSlotCount);
    template DynamicType * PathTypeHandlerBase::CreateNewScopeObject<false>(ScriptContext *scriptContext, DynamicType *type, const PropertyIdArray *propIds, PropertyAttributes extraAttributes, uint extraAttributesSlotCount);

    template <bool isObjectLiteral>
    DynamicType* PathTypeHandlerBase::PromoteType(DynamicType* predecessorType, const PathTypeSuccessorKey key, bool shareType, ScriptContext* scriptContext, DynamicObject* instance, PropertyIndex* propertyIndex)
    {
        Assert(propertyIndex != nullptr);
        Assert(isObjectLiteral || instance != nullptr);

        JavascriptLibrary* library = scriptContext->GetLibrary();
        Recycler* recycler = scriptContext->GetRecycler();
        PropertyIndex index;
        DynamicType * nextType;
        RecyclerWeakReference<DynamicType>* nextTypeWeakRef = nullptr;
        const PropertyRecord *propertyRecord = scriptContext->GetPropertyName(key.GetPropertyId());

        PathTypeHandlerBase * nextPath;
        if (!GetSuccessor(key, &nextTypeWeakRef) || (nextType = nextTypeWeakRef->Get()) == nullptr)
        {
            TypePath * newTypePath = GetTypePath();
            uint8 oldPathSize = GetTypePath()->GetPathSize();

            ObjectSlotAttributes *oldAttributes = GetAttributeArray();
            ObjectSlotAttributes *newAttributes = oldAttributes;
            PathTypeSetterSlotIndex *oldSetters = GetSetterSlots();
            PathTypeSetterSlotIndex *newSetters = oldSetters;

            bool branching = GetTypePath()->GetPathLength() > GetPathLength();
            bool growing = !branching && GetTypePath()->GetPathLength() == GetTypePath()->GetPathSize();

#if ENABLE_FIXED_FIELDS
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            DynamicType* oldType = predecessorType;
            RecyclerWeakReference<DynamicObject>* oldSingletonInstance = GetSingletonInstance();
            TraceFixedFieldsBeforeTypeHandlerChange(branching ? _u("branching") : _u("advancing"), _u("PathTypeHandler"), _u("PathTypeHandler"), instance, this, oldType, oldSingletonInstance);
#endif
#endif

            if (branching)
            {
                // We need to branch the type path.

                if (oldSetters)
                {
                    newTypePath = GetTypePath()->Branch<true>(recycler, GetPathLength(), GetIsOrMayBecomeShared() && !IsolatePrototypes(), oldAttributes);
                }
                else
                {
                    newTypePath = GetTypePath()->Branch<false>(recycler, GetPathLength(), GetIsOrMayBecomeShared() && !IsolatePrototypes());
                }

#ifdef PROFILE_TYPES
                scriptContext->branchCount++;
#endif
#ifdef PROFILE_OBJECT_LITERALS
                if (isObjectLiteral)
                {
                    scriptContext->objectLiteralBranchCount++;
                }
#endif

                if (key.GetAttributes() != ObjectSlotAttr_Default || oldAttributes != nullptr)
                {
                    newAttributes = this->UpdateAttributes(recycler, oldAttributes, oldPathSize, newTypePath->GetPathSize());
                }

                if ((key.GetAttributes() == ObjectSlotAttr_Setter) || oldSetters != nullptr)
                {
                    newSetters = this->UpdateSetterSlots(recycler, oldSetters, oldPathSize, newTypePath->GetPathSize());
                }
            }
            else if (growing)
            {
                // We need to grow the type path.

                newTypePath = GetTypePath()->Grow(recycler);

                if (key.GetAttributes() != ObjectSlotAttr_Default || oldAttributes != nullptr)
                {
                    newAttributes = this->UpdateAttributes(recycler, oldAttributes, oldPathSize, newTypePath->GetPathSize());
                }

                if ((key.GetAttributes() == ObjectSlotAttr_Setter) || oldSetters != nullptr)
                {
                    newSetters = this->UpdateSetterSlots(recycler, oldSetters, oldPathSize, newTypePath->GetPathSize());
                }

                // Update all the predecessor types that use this TypePath to the new TypePath.
                // This will allow the old TypePath to be collected, and will ensure that the
                // fixed field info is correct for those types.

                PathTypeHandlerBase * typeHandlerToUpdate = this;
                TypePath * oldTypePath = GetTypePath();
                while (true)
                {
                    typeHandlerToUpdate->SetTypePath(newTypePath);
                    if (oldAttributes && typeHandlerToUpdate->GetAttributeArray() == oldAttributes)
                    {
                        typeHandlerToUpdate->SetAttributeArray(newAttributes);
                    }
                    if (oldSetters && typeHandlerToUpdate->GetSetterSlots() == oldSetters)
                    {
                        typeHandlerToUpdate->SetSetterSlots(newSetters);
                    }

                    DynamicType * currPredecessorType = typeHandlerToUpdate->GetPredecessorType();
                    if (currPredecessorType == nullptr)
                    {
                        break;
                    }

                    Assert(currPredecessorType->GetTypeHandler()->IsPathTypeHandler());
                    typeHandlerToUpdate = PathTypeHandlerBase::FromTypeHandler(currPredecessorType->GetTypeHandler());
                    if (typeHandlerToUpdate->GetTypePath() != oldTypePath)
                    {
                        break;
                    }
                }
            }
            else
            {
                if (key.GetAttributes() != ObjectSlotAttr_Default && oldAttributes == nullptr)
                {
                    newAttributes = this->UpdateAttributes(recycler, nullptr, oldPathSize, newTypePath->GetPathSize());
                }

                if ((key.GetAttributes() == ObjectSlotAttr_Setter) && oldSetters == nullptr)
                {
                    newSetters = this->UpdateSetterSlots(recycler, nullptr, oldPathSize, newTypePath->GetPathSize());
                }
            }

            bool isSetterProperty = key.GetAttributes() == ObjectSlotAttr_Setter;
            if (isSetterProperty)
            {
                // Not actually adding a property ID to the type path map here.
                index = (PropertyIndex)newTypePath->AddInternal<false>(propertyRecord);
            }
            else
            {
                index = (PropertyIndex)newTypePath->AddInternal<true>(propertyRecord);
            }

            const PropertyIndex newPropertyCount = GetPathLength() + 1;
            const PropertyIndex newSlotCapacity = max(newPropertyCount, static_cast<PropertyIndex>(GetSlotCapacity()));
            PropertyIndex newInlineSlotCapacity = GetInlineSlotCapacity();
            uint16 newOffsetOfInlineSlots = GetOffsetOfInlineSlots();
            if(IsObjectHeaderInlinedTypeHandler() && newSlotCapacity > GetSlotCapacity())
            {
                newInlineSlotCapacity -= GetObjectHeaderInlinableSlotCapacity();
                newOffsetOfInlineSlots = sizeof(DynamicObject);
            }
#if ENABLE_FIXED_FIELDS
            bool markTypeAsShared = !FixPropsOnPathTypes() || shareType;
#else
            bool markTypeAsShared = true;
#endif

            if (key.GetAttributes() == ObjectSlotAttr_Default && oldAttributes == nullptr)
            {
                nextPath = PathTypeHandlerNoAttr::New(scriptContext, newTypePath, newPropertyCount, newSlotCapacity, newInlineSlotCapacity, newOffsetOfInlineSlots, true, markTypeAsShared, predecessorType);
            }
            else
            {
                PathTypeSetterSlotIndex newSetterCount = GetSetterCount() + isSetterProperty;
                newAttributes[index] = key.GetAttributes();
                nextPath = PathTypeHandlerWithAttr::New(scriptContext, newTypePath, newAttributes, newSetters, newSetterCount, newPropertyCount, newSlotCapacity, newInlineSlotCapacity, newOffsetOfInlineSlots, true, markTypeAsShared, predecessorType);
            }
            if (!markTypeAsShared) nextPath->SetMayBecomeShared();
            Assert(nextPath->GetHasOnlyWritableDataProperties());
            nextPath->CopyPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, GetPropertyTypes());
            nextPath->SetPropertyTypes(PropertyTypesInlineSlotCapacityLocked, GetPropertyTypes());

#if ENABLE_FIXED_FIELDS
            if (shareType)
            {
                nextPath->AddBlankFieldAt(propertyRecord->GetPropertyId(), index, scriptContext);
            }
#endif

            if (key.GetPropertyId() == PropertyIds::constructor)
            {
                nextPath->isNotPathTypeHandlerOrHasUserDefinedCtor = true;
            }

#ifdef PROFILE_TYPES
            scriptContext->maxPathLength = max(GetPathLength() + 1, scriptContext->maxPathLength);
#endif

            if (isObjectLiteral)
            {
                // The new type isn't shared yet.  We will make it shared when the second instance attains it.
                nextType = RecyclerNew(recycler, DynamicType, predecessorType, nextPath, /* isLocked = */ true, /* isShared = */ markTypeAsShared);
            }
            else
            {
                // The new type isn't shared yet.  We will make it shared when the second instance attains it.
                nextType = instance->DuplicateType();
                // nextType's prototype and predecessorType's prototype can only be different here
                // only for SetPrototype scenario where predecessorType is the cachedType with newPrototype
                nextType->SetPrototype(predecessorType->GetPrototype());
                nextType->typeHandler = nextPath;
                markTypeAsShared ? nextType->SetIsLockedAndShared() : nextType->SetIsLocked();
            }

            SetSuccessor(predecessorType, key, recycler->CreateWeakReferenceHandle<DynamicType>(nextType), scriptContext);
            // We just extended the current type path to a new tip or created a brand new type path.  We should
            // be at the tip of the path and there should be no instances there yet.
            Assert(nextPath->GetPathLength() == newTypePath->GetPathLength());
#if ENABLE_FIXED_FIELDS
            Assert(!FixPropsOnPathTypes() || shareType || nextPath->GetPathLength() > newTypePath->GetMaxInitializedLength());
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            TraceFixedFieldsAfterTypeHandlerChange(instance, this, nextPath, oldType, nextType, oldSingletonInstance);
#endif
#endif
#ifdef PROFILE_TYPES
            scriptContext->promoteCount++;
#endif
#ifdef PROFILE_OBJECT_LITERALS
            if (isObjectLiteral)
            {
                scriptContext->objectLiteralPromoteCount++;
            }
#endif
        }
        else
        {
#ifdef PROFILE_TYPES
            scriptContext->cacheCount++;
#endif

            // Now that the second (or subsequent) instance reached this type, make sure that it's shared.
            nextPath = (PathTypeHandlerBase *)nextType->GetTypeHandler();
            Assert(nextPath->GetIsInlineSlotCapacityLocked() == this->GetIsInlineSlotCapacityLocked());

            index = nextPath->GetPropertyIndex(propertyRecord);

#if ENABLE_FIXED_FIELDS
            Assert((FixPropsOnPathTypes() && nextPath->GetMayBecomeShared()) || (nextPath->GetIsShared() && nextType->GetIsShared()));
            if (FixPropsOnPathTypes() && !nextType->GetIsShared())
            {
                if (!nextPath->GetIsShared())
                {
                    nextPath->AddBlankFieldAt(propertyRecord->GetPropertyId(), index, scriptContext);
                    nextPath->DoShareTypeHandlerInternal<true>(scriptContext);
                }
                nextType->ShareType();
            }
#endif
        }

        Assert(!IsolatePrototypes() || !GetIsOrMayBecomeShared() || !GetIsPrototype());
        nextPath->SetFlags(IsPrototypeFlag, this->GetFlags());
        Assert(this->GetHasOnlyWritableDataProperties() == nextPath->GetHasOnlyWritableDataProperties() || !(key.GetAttributes() & ObjectSlotAttr_Writable) || (key.GetAttributes() & ObjectSlotAttr_Accessor));
        Assert(this->GetIsInlineSlotCapacityLocked() == nextPath->GetIsInlineSlotCapacityLocked());
        nextPath->SetPropertyTypes(PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, this->GetPropertyTypes());

        PropertyAttributes isWritableAttribute = ((key.GetAttributes() & ObjectSlotAttr_Writable) && !(key.GetAttributes() & ObjectSlotAttr_Accessor)) ? PropertyWritable : PropertyNone;
        library->GetTypesWithOnlyWritablePropertyProtoChainCache()->ProcessProperty(nextPath, isWritableAttribute, propertyRecord, scriptContext);
        library->GetTypesWithNoSpecialPropertyProtoChainCache()->ProcessProperty(nextPath, isWritableAttribute, propertyRecord, scriptContext);

        (*propertyIndex) = index;

#if DBG
        AssertMsg(nextPath->GetPredecessorType()->GetTypeHandler() == this, "Promoting this type to a successor with a different predecessor?");
#endif

        return nextType;
    }

    ObjectSlotAttributes * PathTypeHandlerBase::UpdateAttributes(Recycler * recycler, ObjectSlotAttributes * oldAttributes, uint8 oldPathSize, uint8 newTypePathSize)
    {
        ObjectSlotAttributes * newAttributes = RecyclerNewArrayLeaf(recycler, ObjectSlotAttributes, newTypePathSize);
        uint8 initStart;
        if (oldAttributes == nullptr)
        {
            initStart = 0;
        }
        else
        {
            // In branching cases, the new type path may be shorter than the old.
            initStart = min(newTypePathSize, oldPathSize);
            memcpy(newAttributes, oldAttributes, sizeof(ObjectSlotAttributes) * initStart);
        }
        for (uint8 i = initStart; i < newTypePathSize; i++)
        {
            newAttributes[i] = ObjectSlotAttr_Default;
        }

        return newAttributes;
    }

    PathTypeSetterSlotIndex * PathTypeHandlerBase::UpdateSetterSlots(Recycler * recycler, PathTypeSetterSlotIndex * oldSetters, uint8 oldPathSize, uint8 newTypePathSize)
    {
        PathTypeSetterSlotIndex * newSetters = RecyclerNewArrayLeaf(recycler, PathTypeSetterSlotIndex, newTypePathSize);
        uint8 initStart;
        if (oldSetters == nullptr)
        {
            initStart = 0;
        }
        else
        {
            uint16 pathLength = GetPathLength();
            // In branching cases, the new type path may be shorter than the old.
            initStart = min(newTypePathSize, oldPathSize);
            for (uint8 i = 0; i < initStart; i++)
            {
                // Only copy setter indices that refer to the part of the path contained by this handler already.
                // Otherwise, wait for the correct index, which may be different on this path of the branch,
                // to be set when the setter property is added to the handler.
                PathTypeSetterSlotIndex oldIndex = oldSetters[i];
                newSetters[i] = oldIndex < pathLength ? oldIndex : NoSetterSlot;
            }
        }
        for (uint8 i = initStart; i < newTypePathSize; i++)
        {
            newSetters[i] = NoSetterSlot;
        }

        return newSetters;
    }

    void
    PathTypeHandlerBase::ResetTypeHandler(DynamicObject * instance)
    {
#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertPathToDictionaryResetCount++;
#endif
        // The type path is allocated in the type allocator associated with the script context.
        // So we can't reuse it in other context.  Just convert the type to a simple dictionary type
        this->TryConvertToSimpleDictionaryType(instance, GetPathLength());
    }

    void PathTypeHandlerBase::SetAllPropertiesToUndefined(DynamicObject* instance, bool invalidateFixedFields)
    {
        // Note: This method is currently only called from ResetObject, which in turn only applies to external objects.
        // Before using for other purposes, make sure the assumptions made here make sense in the new context.  In particular,
        // the invalidateFixedFields == false is only correct if a) the object is known not to have any, or b) the type of the
        // object has changed and/or property guards have already been invalidated through some other means.
        int propertyCount = GetPathLength();

#if ENABLE_FIXED_FIELDS
        if (invalidateFixedFields)
        {
            Js::ScriptContext* scriptContext = instance->GetScriptContext();
            for (PropertyIndex propertyIndex = 0; propertyIndex < propertyCount; propertyIndex++)
            {
                PropertyId propertyId = this->GetTypePath()->GetPropertyIdUnchecked(propertyIndex)->GetPropertyId();
                InvalidateFixedFieldAt(propertyId, propertyIndex, scriptContext);
            }
        }
#endif

        Js::RecyclableObject* undefined = instance->GetLibrary()->GetUndefined();
        for (PropertyIndex propertyIndex = 0; propertyIndex < propertyCount; propertyIndex++)
        {
            SetSlotUnchecked(instance, propertyIndex, undefined);
        }

    }

    void PathTypeHandlerBase::MarshalAllPropertiesToScriptContext(DynamicObject* instance, ScriptContext* targetScriptContext, bool invalidateFixedFields)
    {
        // Note: This method is currently only called from ResetObject, which in turn only applies to external objects.
        // Before using for other purposes, make sure the assumptions made here make sense in the new context.  In particular,
        // the invalidateFixedFields == false is only correct if a) the object is known not to have any, or b) the type of the
        // object has changed and/or property guards have already been invalidated through some other means.
        int propertyCount = GetPathLength();

#if ENABLE_FIXED_FIELDS
        if (invalidateFixedFields)
        {
            ScriptContext* scriptContext = instance->GetScriptContext();
            for (PropertyIndex propertyIndex = 0; propertyIndex < propertyCount; propertyIndex++)
            {
                PropertyId propertyId = this->GetTypePath()->GetPropertyIdUnchecked(propertyIndex)->GetPropertyId();
                InvalidateFixedFieldAt(propertyId, propertyIndex, scriptContext);
            }
        }
#endif

        for (int slotIndex = 0; slotIndex < propertyCount; slotIndex++)
        {
            SetSlotUnchecked(instance, slotIndex, CrossSite::MarshalVar(targetScriptContext, GetSlot(instance, slotIndex)));
        }
    }

    BOOL PathTypeHandlerBase::AddProperty(DynamicObject * instance, PropertyId propertyId, Js::Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        if (!ObjectSlotAttributesContains(attributes))
        {
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryAttributesCount++;
#endif
            // Setting an attribute that PathTypeHandler can't express
            return TryConvertToSimpleDictionaryType(instance, GetPathLength() + 1)->SetPropertyWithAttributes(instance, propertyId, value, attributes, info, flags, possibleSideEffects);
        }
        return AddPropertyInternal(instance, propertyId, value, PropertyAttributesToObjectSlotAttributes(attributes), info, flags, possibleSideEffects);
    }

    BOOL PathTypeHandlerBase::AddPropertyInternal(DynamicObject * instance, PropertyId propertyId, Js::Var value, ObjectSlotAttributes attr, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        ScriptContext* scriptContext = instance->GetScriptContext();

#if DBG
        uint32 indexVal;
        Assert(GetPropertyIndex(propertyId) == Constants::NoSlot);
        Assert(!scriptContext->IsNumericPropertyId(propertyId, &indexVal));
#endif

        Assert(propertyId != Constants::NoProperty);
        PropertyRecord const* propertyRecord = instance->GetScriptContext()->GetPropertyName(propertyId);

        if (GetPathLength() >= TypePath::MaxPathTypeHandlerLength)
        {
#ifdef PROFILE_TYPES
            scriptContext->convertPathToDictionaryExceededLengthCount++;
#endif
            return TryConvertToSimpleDictionaryType(instance, GetPathLength() + 1)->SetPropertyWithAttributes(instance, propertyId, value, ObjectSlotAttributesToPropertyAttributes(attr), info, PropertyOperation_None, possibleSideEffects);
        }

        PropertyIndex index;
        DynamicType* newType = PromoteType(instance, PathTypeSuccessorKey(propertyId, attr), &index);

        Assert(instance->GetTypeHandler()->IsPathTypeHandler());
        PathTypeHandlerBase* newTypeHandler = (PathTypeHandlerBase*)newType->GetTypeHandler();

        Assert(newType->GetIsShared() == newTypeHandler->GetIsShared());

        newTypeHandler->SetSlotAndCache(instance, propertyId, propertyRecord, index, value, info, flags, possibleSideEffects);

        Assert(!IsolatePrototypes() || ((this->GetFlags() & IsPrototypeFlag) == 0));
        if (this->GetFlags() & IsPrototypeFlag)
        {
            scriptContext->InvalidateProtoCaches(propertyId);
        }
        return true;
    }

    DynamicTypeHandler* PathTypeHandlerBase::ConvertToTypeWithItemAttributes(DynamicObject* instance)
    {
#ifdef PROFILE_TYPES
        instance->GetScriptContext()->convertPathToDictionaryItemAttributesCount++;
#endif
        return JavascriptArray::IsNonES5Array(instance) ?
            ConvertToES5ArrayType(instance) : ConvertToDictionaryType(instance);
    }

    bool PathTypeHandlerBase::GetMaxPathLength(uint16 * maxPathLength)
    {
        if (GetPropertyCount() > *maxPathLength)
        {
            *maxPathLength = GetPathLength();
        }

        bool result = true;
        this->MapSuccessorsUntil([&result, maxPathLength](PathTypeSuccessorKey, RecyclerWeakReference<DynamicType> * typeWeakReference) -> bool
        {
            DynamicType * type = typeWeakReference->Get();
            if (!type)
            {
                return false;
            }
            if (!type->GetTypeHandler()->IsPathTypeHandler())
            {
                result = false;
                return true;
            }
            if (!PathTypeHandlerBase::FromTypeHandler(type->GetTypeHandler())->GetMaxPathLength(maxPathLength))
            {
                result = false;
                return true;
            }

            return false;
        });

        return result;
    }

    void PathTypeHandlerBase::ShrinkSlotAndInlineSlotCapacity()
    {
        if (!GetIsInlineSlotCapacityLocked())
        {
            PathTypeHandlerBase * rootTypeHandler = GetRootPathTypeHandler();

            bool shrunk = false;
            uint16 maxPathLength = 0;
            if (rootTypeHandler->GetMaxPathLength(&maxPathLength))
            {
                uint16 newInlineSlotCapacity =
                    IsObjectHeaderInlinedTypeHandler()
                        ? RoundUpObjectHeaderInlinedInlineSlotCapacity(maxPathLength)
                        : RoundUpInlineSlotCapacity(maxPathLength);
                if (newInlineSlotCapacity < GetInlineSlotCapacity())
                {
                    rootTypeHandler->ShrinkSlotAndInlineSlotCapacity(newInlineSlotCapacity);
                    shrunk = true;
                }
            }

            if (!shrunk)
            {
                rootTypeHandler->LockInlineSlotCapacity();
            }
        }

#if DBG
        PathTypeHandlerBase * rootTypeHandler = GetRootPathTypeHandler();
        rootTypeHandler->VerifyInlineSlotCapacityIsLocked();
#endif
    }

    void PathTypeHandlerBase::ShrinkSlotAndInlineSlotCapacity(uint16 newInlineSlotCapacity)
    {
        Assert(!this->GetIsInlineSlotCapacityLocked());
        this->SetInlineSlotCapacity(newInlineSlotCapacity);
        // Slot capacity should also be shrunk when the inlineSlotCapacity is shrunk.
        this->SetSlotCapacity(newInlineSlotCapacity);
        this->SetIsInlineSlotCapacityLocked();

        this->MapSuccessors([newInlineSlotCapacity](PathTypeSuccessorKey, RecyclerWeakReference<DynamicType> * typeWeakReference)
        {
            DynamicType * type = typeWeakReference->Get();
            if (type)
            {
                PathTypeHandlerBase::FromTypeHandler(type->GetTypeHandler())->ShrinkSlotAndInlineSlotCapacity(newInlineSlotCapacity);
            }
        });
    }

    void PathTypeHandlerBase::LockInlineSlotCapacity()
    {
        Assert(!GetIsInlineSlotCapacityLocked());
        SetIsInlineSlotCapacityLocked();

        this->MapSuccessors([](const PathTypeSuccessorKey, RecyclerWeakReference<DynamicType>* typeWeakReference)
        {
            DynamicType * type = typeWeakReference->Get();
            if (!type)
            {
                return;
            }

            type->GetTypeHandler()->LockInlineSlotCapacity();
        });
    }

    void PathTypeHandlerBase::EnsureInlineSlotCapacityIsLocked()
    {
        EnsureInlineSlotCapacityIsLocked(true);
#if DBG
        VerifyInlineSlotCapacityIsLocked();
#endif
    }

    void PathTypeHandlerBase::EnsureInlineSlotCapacityIsLocked(bool startFromRoot)
    {
        if (startFromRoot)
        {
            GetRootPathTypeHandler()->EnsureInlineSlotCapacityIsLocked(false);
            return;
        }

        Assert(!startFromRoot);

        if (!GetIsInlineSlotCapacityLocked())
        {
            SetIsInlineSlotCapacityLocked();

            this->MapSuccessors([](const PathTypeSuccessorKey, RecyclerWeakReference<DynamicType> * typeWeakReference)
            {
                DynamicType * type = typeWeakReference->Get();
                if (!type)
                {
                    return;
                }

                DynamicTypeHandler* successorTypeHandler = type->GetTypeHandler();
                successorTypeHandler->IsPathTypeHandler() ?
                    PathTypeHandlerBase::FromTypeHandler(successorTypeHandler)->EnsureInlineSlotCapacityIsLocked(false) :
                    successorTypeHandler->EnsureInlineSlotCapacityIsLocked();
            });
        }
    }

    void PathTypeHandlerBase::VerifyInlineSlotCapacityIsLocked()
    {
        VerifyInlineSlotCapacityIsLocked(true);
    }

    void PathTypeHandlerBase::VerifyInlineSlotCapacityIsLocked(bool startFromRoot)
    {
        if (startFromRoot)
        {
            GetRootPathTypeHandler()->VerifyInlineSlotCapacityIsLocked(false);
            return;
        }

        Assert(!startFromRoot);

        Assert(GetIsInlineSlotCapacityLocked());

        this->MapSuccessors([](const PathTypeSuccessorKey, RecyclerWeakReference<DynamicType> * typeWeakReference)
        {
            DynamicType * type = typeWeakReference->Get();
            if (!type)
            {
                return;
            }

            DynamicTypeHandler* successorTypeHandler = type->GetTypeHandler();
            successorTypeHandler->IsPathTypeHandler() ?
                PathTypeHandlerBase::FromTypeHandler(successorTypeHandler)->VerifyInlineSlotCapacityIsLocked(false) :
                successorTypeHandler->VerifyInlineSlotCapacityIsLocked();
        });
    }

    PathTypeHandlerBase *PathTypeHandlerBase::DeoptimizeObjectHeaderInlining(JavascriptLibrary *const library)
    {
        Assert(IsObjectHeaderInlinedTypeHandler());

        // Clone the type Path here to evolve separately
        Recycler * recycler = library->GetRecycler();
        uint16 pathLength = GetPathLength();
        TypePath * clonedPath = TypePath::New(recycler, pathLength);

        ObjectSlotAttributes *attributes = this->GetAttributeArray();
        for (PropertyIndex i = 0; i < pathLength; i++)
        {
            clonedPath->assignments[i] = GetTypePath()->assignments[i];
            if (attributes && attributes[i] == ObjectSlotAttr_Setter)
            {
                clonedPath->AddInternal<false>(clonedPath->assignments[i]);
            }
            else
            {
                clonedPath->AddInternal<true>(clonedPath->assignments[i]);
            }
        }

        // We don't copy the fixed fields, as we will be sharing this type anyways later and the fixed fields vector has to be invalidated.
        PathTypeHandlerBase * clonedTypeHandler;
        if (attributes == nullptr)
        {
            clonedTypeHandler =
                PathTypeHandlerNoAttr::New(
                    library->GetScriptContext(),
                    clonedPath,
                    GetPathLength(),
                    static_cast<PropertyIndex>(GetSlotCapacity()),
                    GetInlineSlotCapacity() - GetObjectHeaderInlinableSlotCapacity(),
                    sizeof(DynamicObject),
                    false,
                    false);
        }
        else
        {
            uint8 newTypePathSize = clonedPath->GetPathSize();

            ObjectSlotAttributes * newAttributes = RecyclerNewArrayLeaf(recycler, ObjectSlotAttributes, newTypePathSize);
            memcpy(newAttributes, attributes, sizeof(ObjectSlotAttributes) * newTypePathSize);

            PathTypeSetterSlotIndex * setters = GetSetterSlots();
            PathTypeSetterSlotIndex * newSetters;
            if (setters == nullptr)
            {
                newSetters = nullptr;
            }
            else
            {
                newSetters = RecyclerNewArrayLeaf(recycler, PathTypeSetterSlotIndex, newTypePathSize);
                memcpy(newSetters, setters, sizeof(PathTypeSetterSlotIndex) * newTypePathSize);
            }

            clonedTypeHandler =
                PathTypeHandlerWithAttr::New(
                    library->GetScriptContext(),
                    clonedPath,
                    newAttributes,
                    newSetters,
                    GetSetterCount(),
                    GetPathLength(),
                    static_cast<PropertyIndex>(GetSlotCapacity()),
                    GetInlineSlotCapacity() - GetObjectHeaderInlinableSlotCapacity(),
                    sizeof(DynamicObject),
                    false,
                    false);
        }
        clonedTypeHandler->SetMayBecomeShared();
        clonedTypeHandler->CopyPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, this->GetPropertyTypes());

        return clonedTypeHandler;
    }

    void PathTypeHandlerBase::SetPrototype(DynamicObject* instance, RecyclableObject* newPrototype)
    {
        // No typesharing for ExternalType
        if (instance->GetType()->IsExternal())
        {
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryProtoCount++;
#endif
            TryConvertToSimpleDictionaryType(instance, GetPathLength())->SetPrototype(instance, newPrototype);
            return;
        }

        const bool useObjectHeaderInlining = IsObjectHeaderInlined(this->GetOffsetOfInlineSlots());
        uint16 requestedInlineSlotCapacity = this->GetInlineSlotCapacity();
        uint16 roundedInlineSlotCapacity = (useObjectHeaderInlining ?
                                            DynamicTypeHandler::RoundUpObjectHeaderInlinedInlineSlotCapacity(requestedInlineSlotCapacity) :
                                            DynamicTypeHandler::RoundUpInlineSlotCapacity(requestedInlineSlotCapacity));
        ScriptContext* scriptContext = instance->GetScriptContext();
        DynamicType* cachedDynamicType = nullptr;
        DynamicType* oldType = instance->GetDynamicType();

        bool useCache = instance->GetScriptContext() == newPrototype->GetScriptContext();

        TypeTransitionMap * oldTypeToPromotedTypeMap = nullptr;
#if DBG
        DynamicType * oldCachedType = nullptr;
        char16 reason[1024];
        swprintf_s(reason, 1024, _u("Cache not populated."));
#endif
        if (useCache && newPrototype->GetInternalProperty(newPrototype, Js::InternalPropertyIds::TypeOfPrototypeObjectDictionary, (Js::Var*)&oldTypeToPromotedTypeMap, nullptr, scriptContext)
            && oldTypeToPromotedTypeMap != nullptr
            )
        {
            AssertOrFailFast((Js::Var)oldTypeToPromotedTypeMap != scriptContext->GetLibrary()->GetUndefined());
            oldTypeToPromotedTypeMap = reinterpret_cast<TypeTransitionMap*>(oldTypeToPromotedTypeMap);

            if (oldTypeToPromotedTypeMap->TryGetValue(oldType, &cachedDynamicType))
            {
#if DBG
                oldCachedType = cachedDynamicType;
#endif
                DynamicTypeHandler *const cachedDynamicTypeHandler = cachedDynamicType->GetTypeHandler();
                if (cachedDynamicTypeHandler->GetOffsetOfInlineSlots() != GetOffsetOfInlineSlots())
                {
                    cachedDynamicType = nullptr;
#if DBG
                    swprintf_s(reason, 1024, _u("OffsetOfInlineSlot mismatch. Required = %d, Cached = %d"), this->GetOffsetOfInlineSlots(), cachedDynamicTypeHandler->GetOffsetOfInlineSlots());
#endif
                }
                else if (cachedDynamicTypeHandler->GetInlineSlotCapacity() != roundedInlineSlotCapacity)
                {
                    Assert(cachedDynamicTypeHandler->GetInlineSlotCapacity() >= roundedInlineSlotCapacity);
                    Assert(cachedDynamicTypeHandler->GetInlineSlotCapacity() >= GetPropertyCount());
                    cachedDynamicTypeHandler->ShrinkSlotAndInlineSlotCapacity();

                    // If slot capacity doesn't match after shrinking, it could be because oldType was shrunk and
                    // newType evolved. In that case, we should update the cache with new type
                    if (cachedDynamicTypeHandler->GetInlineSlotCapacity() != roundedInlineSlotCapacity)
                    {
                        cachedDynamicType = nullptr;
#if DBG
                        swprintf_s(reason, 1024, _u("InlineSlotCapacity mismatch after shrinking. Required = %d, Cached = %d"), roundedInlineSlotCapacity, cachedDynamicTypeHandler->GetInlineSlotCapacity());
#endif
                    }
                }
            }
        }
        else
        {
            Assert(!oldTypeToPromotedTypeMap || (Js::Var)oldTypeToPromotedTypeMap == scriptContext->GetLibrary()->GetUndefined());
            oldTypeToPromotedTypeMap = nullptr;
        }

        if (cachedDynamicType == nullptr)
        {
            PathTypeHandlerBase* newTypeHandler = PathTypeHandlerNoAttr::New(scriptContext, scriptContext->GetLibrary()->GetRootPath(), 0, static_cast<PropertyIndex>(this->GetSlotCapacity()), this->GetInlineSlotCapacity(), this->GetOffsetOfInlineSlots(), GetIsLocked(), GetIsShared());
            newTypeHandler->SetFlags(MayBecomeSharedFlag, GetFlags());

            cachedDynamicType = instance->DuplicateType();
            cachedDynamicType->SetPrototype(newPrototype);
            cachedDynamicType->typeHandler = newTypeHandler;

            // Make type locked, shared only if we are using cache
            if (useCache)
            {
                cachedDynamicType->LockType();
                cachedDynamicType->ShareType();
            }

            // Promote type based on existing properties to get new type which will be cached and shared
            ObjectSlotAttributes * attributes = this->GetAttributeArray();
            for (PropertyIndex propertyIndex = 0; propertyIndex < GetPathLength(); propertyIndex++)
            {
                Js::PropertyId propertyId = GetPropertyId(scriptContext, propertyIndex);
                ObjectSlotAttributes attr = attributes ? attributes[propertyIndex] : ObjectSlotAttr_Default;
                cachedDynamicType = newTypeHandler->PromoteType<false>(cachedDynamicType, PathTypeSuccessorKey(propertyId, attr), true, scriptContext, instance, &propertyIndex);
                newTypeHandler = PathTypeHandlerBase::FromTypeHandler(cachedDynamicType->GetTypeHandler());
                if (attr == ObjectSlotAttr_Setter)
                {
                    newTypeHandler->SetSetterSlot(newTypeHandler->GetTypePath()->LookupInline(propertyId, newTypeHandler->GetPathLength()), (PathTypeSetterSlotIndex)(newTypeHandler->GetPathLength() - 1));
                }
            }
            Assert(newTypeHandler->GetPathLength() == GetPathLength());
            Assert(newTypeHandler->GetPropertyCount() == GetPropertyCount());
            Assert(newTypeHandler->GetSetterCount() == GetSetterCount());

            if (useCache)
            {
                if (oldTypeToPromotedTypeMap == nullptr)
                {
                    oldTypeToPromotedTypeMap = RecyclerNew(instance->GetRecycler(), TypeTransitionMap, instance->GetRecycler(), 2);
                    newPrototype->SetInternalProperty(Js::InternalPropertyIds::TypeOfPrototypeObjectDictionary, (Var)oldTypeToPromotedTypeMap, PropertyOperationFlags::PropertyOperation_Force, nullptr);
                }

                oldTypeToPromotedTypeMap->Item(oldType, cachedDynamicType);
#if DBG
                cachedDynamicType->SetIsCachedForChangePrototype();
#endif
                if (PHASE_TRACE1(TypeShareForChangePrototypePhase))
                {
#if DBG
                    if (PHASE_VERBOSE_TRACE1(TypeShareForChangePrototypePhase))
                    {
                        Output::Print(_u("TypeSharing: Updating prototype [0x%p] object's DictionarySlot in __proto__. Adding (key = 0x%p, value = 0x%p) in map = 0x%p. Reason = %s\n"), newPrototype, oldType, cachedDynamicType, oldTypeToPromotedTypeMap, reason);
                    }
                    else
                    {
#endif
                        Output::Print(_u("TypeSharing: Updating prototype object's DictionarySlot cache in __proto__.\n"));
#if DBG
                    }
#endif
                    Output::Flush();
                }

            }
            else
            {
                if (PHASE_TRACE1(TypeShareForChangePrototypePhase) || PHASE_VERBOSE_TRACE1(TypeShareForChangePrototypePhase))
                {
                    Output::Print(_u("TypeSharing: No Typesharing because instance and newPrototype are from different scriptContext.\n"));
                    Output::Flush();
                }
            }
        }
        else
        {
            Assert(cachedDynamicType->GetIsShared());
            if (PHASE_TRACE1(TypeShareForChangePrototypePhase))
            {
#if DBG
                if (PHASE_VERBOSE_TRACE1(TypeShareForChangePrototypePhase))
                {

                    Output::Print(_u("TypeSharing: Reusing prototype [0x%p] object's DictionarySlot (key = 0x%p, value = 0x%p) from map = 0x%p in __proto__.\n"), newPrototype, oldType, cachedDynamicType, oldTypeToPromotedTypeMap);
                }
                else
                {
#endif
                    Output::Print(_u("TypeSharing: Reusing prototype object's DictionarySlot cache in __proto__.\n"));
#if DBG
                }
#endif
                Output::Flush();
            }
        }


        // Make sure the offsetOfInlineSlots and inlineSlotCapacity matches with currentTypeHandler
        Assert(cachedDynamicType->GetTypeHandler()->GetOffsetOfInlineSlots() == GetOffsetOfInlineSlots());
        Assert(cachedDynamicType->GetTypeHandler()->GetSlotCapacity() == this->GetSlotCapacity());
        Assert(DynamicObject::IsTypeHandlerCompatibleForObjectHeaderInlining(this, cachedDynamicType->GetTypeHandler()));
        Assert(cachedDynamicType->GetPrototype() == newPrototype);
        instance->ReplaceType(cachedDynamicType);
    }

    void PathTypeHandlerBase::SetIsPrototype(DynamicObject* instance)
    {
        // Don't return if IsPrototypeFlag is set, because we may still need to do a type transition and
        // set fixed bits.  If this handler is shared, this instance may not even be a prototype yet.
        // In this case we may need to convert to a non-shared type handler.
        if (!ChangeTypeOnProto() && !(GetIsOrMayBecomeShared() && IsolatePrototypes()))
        {
            SetFlags(IsPrototypeFlag);
            return;
        }

#if ENABLE_FIXED_FIELDS
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        DynamicType* oldTypeDebug = instance->GetDynamicType();
        RecyclerWeakReference<DynamicObject>* oldSingletonInstance = GetSingletonInstance();
#endif
#endif

        if ((GetIsOrMayBecomeShared() && IsolatePrototypes()))
        {
            // The type coming in may not be shared or even locked (i.e. might have been created via DynamicObject::ChangeType()).
            // In that case the type handler change below won't change the type on the object, so we have to force it.

            DynamicType* oldType = instance->GetDynamicType();
#ifdef PROFILE_TYPES
            instance->GetScriptContext()->convertPathToDictionaryProtoCount++;
#endif
            TryConvertToSimpleDictionaryType(instance, GetPathLength());

            if (ChangeTypeOnProto() && instance->GetDynamicType() == oldType)
            {
                instance->ChangeType();
            }
        }
        else
        {
#if ENABLE_FIXED_FIELDS
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            TraceFixedFieldsBeforeSetIsProto(instance, this, oldTypeDebug, oldSingletonInstance);
#endif
#endif

            if (ChangeTypeOnProto())
            {
                // If this handler is shared and we don't isolate prototypes, it's possible that the handler has
                // the prototype flag, but this instance may not yet be a prototype and may not have taken
                // the required type transition.  It would be nice to have a reliable flag on the object
                // indicating whether it's a prototype to avoid multiple type transitions if the same object
                // with shared type handler is used as prototype multiple times.
                if (((GetFlags() & IsPrototypeFlag) == 0) || (GetIsShared() && !IsolatePrototypes()))
                {
                    // We're about to split out the type.  If the original type was shared the handler better be shared as well.
                    // Otherwise, the handler would lose track of being shared between different types and instances.
                    Assert(!instance->HasSharedType() || instance->GetDynamicType()->GetTypeHandler()->GetIsShared());

                    instance->ChangeType();
                    Assert(!instance->HasLockedType() && !instance->HasSharedType());
                }
            }
        }

        DynamicTypeHandler* typeHandler = GetCurrentTypeHandler(instance);
        if (typeHandler != this)
        {
            typeHandler->SetIsPrototype(instance);
        }
        else
        {
            SetFlags(IsPrototypeFlag);

#if ENABLE_FIXED_FIELDS
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            TraceFixedFieldsAfterSetIsProto(instance, this, typeHandler, oldTypeDebug, instance->GetDynamicType(), oldSingletonInstance);
#endif
#endif

        }
    }

    PathTypeHandlerBase* PathTypeHandlerBase::GetRootPathTypeHandler()
    {
        PathTypeHandlerBase* rootTypeHandler = this;
        while (rootTypeHandler->predecessorType != nullptr)
        {
            rootTypeHandler = PathTypeHandlerBase::FromTypeHandler(rootTypeHandler->predecessorType->GetTypeHandler());
        }
        Assert(rootTypeHandler->predecessorType == nullptr);
        return rootTypeHandler;
    }

#if DBG
    bool PathTypeHandlerBase::CanStorePropertyValueDirectly(const DynamicObject* instance, PropertyId propertyId, bool allowLetConst)
    {
        Assert(!allowLetConst);
        // We pass Constants::NoProperty for ActivationObjects for functions with same named formals, but we don't
        // use PathTypeHandlers for those.
        Assert(propertyId != Constants::NoProperty);
        Js::PropertyIndex index = GetPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            if (FixPropsOnPathTypes())
            {
                return index < this->GetTypePath()->GetMaxInitializedLength() && !this->GetTypePath()->GetIsFixedFieldAt(index, this->GetPathLength());
            }
            else
#endif
            {
                return true;
            }
        }
        else
        {
            AssertMsg(false, "Asking about a property this type handler doesn't know about?");
            return false;
        }
    }
#endif

#if ENABLE_FIXED_FIELDS
    BOOL PathTypeHandlerBase::IsFixedProperty(const DynamicObject* instance, PropertyId propertyId)
    {
        if (!FixPropsOnPathTypes())
        {
            return false;
        }

        PropertyIndex index = PathTypeHandlerBase::GetPropertyIndex(propertyId);
        Assert(index != Constants::NoSlot);

        return this->GetTypePath()->GetIsFixedFieldAt(index, GetPathLength());
    }

    bool PathTypeHandlerBase::HasSingletonInstance() const
    {
        Assert(HasSingletonInstanceOnlyIfNeeded());
        if (!FixPropsOnPathTypes())
        {
            return false;
        }

        return this->GetTypePath()->HasSingletonInstance() && GetPathLength() >= this->GetTypePath()->GetMaxInitializedLength();
    }

    void PathTypeHandlerBase::DoShareTypeHandler(ScriptContext* scriptContext)
    {
        DoShareTypeHandlerInternal<true>(scriptContext);
    }

    template <bool invalidateFixedFields>
    void PathTypeHandlerBase::DoShareTypeHandlerInternal(ScriptContext* scriptContext)
    {
        Assert((GetFlags() & (IsLockedFlag | MayBecomeSharedFlag | IsSharedFlag)) == (IsLockedFlag | MayBecomeSharedFlag));
        Assert(!IsolatePrototypes() || !GetIsOrMayBecomeShared() || !GetIsPrototype());

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        // If this handler is becoming shared we need to remove the singleton instance (so that it can be collected
        // if no longer referenced by anything else) and invalidate any fixed fields.
        if (FixPropsOnPathTypes())
        {
            if (invalidateFixedFields)
            {
                if (this->GetTypePath()->GetMaxInitializedLength() < GetPathLength())
                {
                    this->GetTypePath()->SetMaxInitializedLength(GetPathLength());
                }
                for (PropertyIndex index = 0; index < this->GetPathLength(); index++)
                {
                    InvalidateFixedFieldAt(this->GetTypePath()->GetPropertyIdUnchecked(index)->GetPropertyId(), index, scriptContext);
                }
            }

            Assert(HasOnlyInitializedNonFixedProperties());
            Assert(HasSingletonInstanceOnlyIfNeeded());
            if (HasSingletonInstance())
            {
                this->GetTypePath()->ClearSingletonInstance();
            }
        }
#endif
    }

    void PathTypeHandlerBase::InvalidateFixedFieldAt(Js::PropertyId propertyId, Js::PropertyIndex index, ScriptContext* scriptContext)
    {
        if (!FixPropsOnPathTypes())
        {
            return;
        }

        // We are adding a new value where some other instance already has an existing value.  If this is a fixed
        // field we must clear the bit. If the value was hard coded in the JIT-ed code, we must invalidate the guards.
        if (this->GetTypePath()->GetIsUsedFixedFieldAt(index, GetPathLength()))
        {
            // We may be a second instance chasing the singleton and invalidating fixed fields along the way.
            // Assert(newTypeHandler->GetTypePath()->GetSingletonInstance() == instance);

            // Invalidate any JIT-ed code that hard coded this method. No need to invalidate store field
            // inline caches (which might quietly overwrite this fixed fields, because they have never been populated.
#if ENABLE_NATIVE_CODEGEN
            scriptContext->GetThreadContext()->InvalidatePropertyGuards(propertyId);
#endif
        }

        // If we're overwriting an existing value of this property, we don't consider the new one fixed.
        // This also means that it's ok to populate the inline caches for this property from now on.
        this->GetTypePath()->ClearIsFixedFieldAt(index, GetPathLength());
    }

    void PathTypeHandlerBase::AddBlankFieldAt(Js::PropertyId propertyId, Js::PropertyIndex index, ScriptContext* scriptContext)
    {
        if (!FixPropsOnPathTypes())
        {
            return;
        }

        if (index >= this->GetTypePath()->GetMaxInitializedLength())
        {
            // We are adding a property where no instance property has been set before.  We rely on properties being
            // added in order of indexes to be sure that we don't leave any uninitialized properties interspersed with
            // initialized ones, which could lead to incorrect behavior.  See comment in TypePath::Branch.
            AssertMsg(index == this->GetTypePath()->GetMaxInitializedLength(), "Adding properties out of order?");

            this->GetTypePath()->AddBlankFieldAt(index, GetPathLength());
        }
        else
        {
            InvalidateFixedFieldAt(propertyId, index, scriptContext);

            // We have now reached the most advanced instance along this path.  If this instance is not the singleton instance,
            // then the former singleton instance (if any) is no longer a singleton.  This instance could be the singleton
            // instance, if we just happen to set (overwrite) its last property.
            if (index + 1 == this->GetTypePath()->GetMaxInitializedLength())
            {
                // If we cleared the singleton instance while some fields remained fixed, the instance would
                // be collectible, and yet some code would expect to see values and call methods on it. We rely on the
                // fact that we always add properties to (pre-initialized) type handlers in the order they appear
                // on the type path.  By the time we reach the singleton instance, all fixed fields will have been invalidated.
                // Otherwise, some fields could remain fixed (or even uninitialized) and we would have to spin off a loop here
                // to invalidate any remaining fixed fields
                Assert(HasSingletonInstanceOnlyIfNeeded());
                this->GetTypePath()->ClearSingletonInstance();
            }

        }
    }

    bool PathTypeHandlerBase::ProcessFixedFieldChange(DynamicObject* instance, PropertyId propertyId, PropertyIndex slotIndex, Var value, bool isNonFixed,const PropertyRecord * propertyRecord)
    {
        Assert(!instance->GetTypeHandler()->GetIsShared());
        // We don't want fixed properties on external objects, either external properties or expando properties.
        // See DynamicObject::ResetObject for more information.
        Assert(!instance->IsExternal() || isNonFixed);

        if (!FixPropsOnPathTypes())
        {
            return true;
        }

        bool populateInlineCache = true;

        PathTypeHandlerBase* newTypeHandler = (PathTypeHandlerBase*)instance->GetTypeHandler();

        if (slotIndex >= newTypeHandler->GetTypePath()->GetMaxInitializedLength())
        {
            // We are adding a property where no instance property has been set before.  We rely on properties being
            // added in order of indexes to be sure that we don't leave any uninitialized properties interspersed with
            // initialized ones, which could lead to incorrect behavior.  See comment in TypePath::Branch.
            AssertMsg(slotIndex == newTypeHandler->GetTypePath()->GetMaxInitializedLength(), "Adding properties out of order?");

            // Consider: It would be nice to assert the slot is actually null.  However, we sometimes pre-initialize to
            // undefined or even some other special illegal value (for let or const, currently == null)
            // Assert(instance->GetSlot(index) == nullptr);

            if (ShouldFixAnyProperties() && CanBeSingletonInstance(instance))
            {
                bool markAsFixed = !isNonFixed && !IsInternalPropertyId(propertyId) &&
                    (VarIs<JavascriptFunction>(value) ? ShouldFixMethodProperties() || ShouldFixAccessorProperties() :
                                    (ShouldFixDataProperties() && CheckHeuristicsForFixedDataProps(instance, propertyRecord, propertyId, value)));

                // Mark the newly added field as fixed and prevent population of inline caches.

                newTypeHandler->GetTypePath()->AddSingletonInstanceFieldAt(instance, slotIndex, markAsFixed, newTypeHandler->GetPathLength());
            }
            else
            {
                newTypeHandler->GetTypePath()->AddSingletonInstanceFieldAt(slotIndex, newTypeHandler->GetPathLength());
            }

            populateInlineCache = false;
        }
        else
        {
            newTypeHandler->InvalidateFixedFieldAt(propertyId, slotIndex, instance->GetScriptContext());

            // We have now reached the most advanced instance along this path.  If this instance is not the singleton instance,
            // then the former singleton instance (if any) is no longer a singleton.  This instance could be the singleton
            // instance, if we just happen to set (overwrite) its last property.
            if (slotIndex + 1 == newTypeHandler->GetTypePath()->GetMaxInitializedLength())
            {
                // If we cleared the singleton instance while some fields remained fixed, the instance would
                // be collectible, and yet some code would expect to see values and call methods on it. We rely on the
                // fact that we always add properties to (pre-initialized) type handlers in the order they appear
                // on the type path.  By the time we reach the singleton instance, all fixed fields will have been invalidated.
                // Otherwise, some fields could remain fixed (or even uninitialized) and we would have to spin off a loop here
                // to invalidate any remaining fixed fields
                auto singletonWeakRef = newTypeHandler->GetTypePath()->GetSingletonInstance();
                if (singletonWeakRef != nullptr && instance != singletonWeakRef->Get())
                {
                    Assert(newTypeHandler->HasSingletonInstanceOnlyIfNeeded());
                    newTypeHandler->GetTypePath()->ClearSingletonInstance();
                }
            }
        }

        // If we branched and this is the singleton instance, we need to remove it from this type handler.  The only time
        // this can happen is when another not fully initialized instance is ahead of this one on the current path.
        auto singletonWeakRef = this->GetTypePath()->GetSingletonInstance();
        if (newTypeHandler->GetTypePath() != this->GetTypePath() && singletonWeakRef != nullptr && singletonWeakRef->Get() == instance)
        {
            // If this is the singleton instance, there shouldn't be any other initialized instance ahead of it on the old path.
            Assert(GetPathLength() >= this->GetTypePath()->GetMaxInitializedLength());
            Assert(HasSingletonInstanceOnlyIfNeeded());
            this->GetTypePath()->ClearSingletonInstance();
        }

        return populateInlineCache;
    }

    bool PathTypeHandlerBase::TryUseFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, FixedPropertyKind propertyType, ScriptContext * requestContext)
    {
        bool result = TryGetFixedProperty<false, true>(propertyRecord, pProperty, propertyType, requestContext);
        TraceUseFixedProperty(propertyRecord, pProperty, result, _u("PathTypeHandler"), requestContext);
        return result;
    }

    bool PathTypeHandlerBase::TryUseFixedAccessor(PropertyRecord const * propertyRecord, Var * pAccessor, FixedPropertyKind propertyType, bool getter, ScriptContext * requestContext)
    {
        if (PHASE_VERBOSE_TRACE1(Js::FixedMethodsPhase) || PHASE_VERBOSE_TESTTRACE1(Js::FixedMethodsPhase) ||
            PHASE_VERBOSE_TRACE1(Js::UseFixedDataPropsPhase) || PHASE_VERBOSE_TESTTRACE1(Js::UseFixedDataPropsPhase))
        {
            Output::Print(_u("FixedFields: attempt to use fixed accessor %s from PathTypeHandler returned false.\n"), propertyRecord->GetBuffer());
            if (this->HasSingletonInstance() && this->GetSingletonInstance()->Get()->GetScriptContext() != requestContext)
            {
                Output::Print(_u("FixedFields: Cross Site Script Context is used for property %s. \n"), propertyRecord->GetBuffer());
            }
            Output::Flush();
        }
        return false;
    }

#if DBG
    bool PathTypeHandlerBase::HasOnlyInitializedNonFixedProperties()
    {

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
      if (this->GetTypePath()->GetMaxInitializedLength() < GetPathLength())
      {
          return false;
      }

      for (PropertyIndex index = 0; index < this->GetPathLength(); index++)
      {
          if (this->GetTypePath()->GetIsFixedFieldAt(index, this->GetPathLength()))
          {
              return false;
          }
      }
#endif

      return true;
    }

    bool PathTypeHandlerBase::CheckFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, ScriptContext * requestContext)
    {
        return TryGetFixedProperty<true, false>(propertyRecord, pProperty, (Js::FixedPropertyKind)(Js::FixedPropertyKind::FixedMethodProperty | Js::FixedPropertyKind::FixedDataProperty), requestContext);
    }

    bool PathTypeHandlerBase::HasAnyFixedProperties() const
    {
        int pathLength = GetPathLength();
        for (PropertyIndex i = 0; i < pathLength; i++)
        {
            if (this->GetTypePath()->GetIsFixedFieldAt(i, pathLength))
            {
                return true;
            }
        }
        return false;
    }
#endif

    template <bool allowNonExistent, bool markAsUsed>
    bool PathTypeHandlerBase::TryGetFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, Js::FixedPropertyKind propertyType, ScriptContext * requestContext)
    {
        if (!FixPropsOnPathTypes())
        {
            return false;
        }

        PropertyIndex index = this->GetTypePath()->Lookup(propertyRecord->GetPropertyId(), GetPathLength());
        if (index == Constants::NoSlot)
        {
            AssertMsg(allowNonExistent, "Trying to get a fixed function instance for a non-existent property?");
            return false;
        }

        ObjectSlotAttributes * attributes = this->GetAttributeArray();
        ObjectSlotAttributes attr = attributes ? attributes[index] : ObjectSlotAttr_Default;
        if (attr & (ObjectSlotAttr_Deleted | ObjectSlotAttr_Accessor))
        {
            return false;
        }

        Var value = this->GetTypePath()->GetSingletonFixedFieldAt(index, GetPathLength(), requestContext);
        if (value && ((IsFixedMethodProperty(propertyType) && VarIs<JavascriptFunction>(value)) || IsFixedDataProperty(propertyType)))
        {
            *pProperty = value;
            if (markAsUsed)
            {
                this->GetTypePath()->SetIsUsedFixedFieldAt(index, GetPathLength());
            }
            return true;
        }
        else
        {
            return false;
        }
    }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    void PathTypeHandlerBase::DumpFixedFields() const {
        if (FixPropsOnPathTypes())
        {
            for (PropertyIndex i = 0; i < GetPathLength(); i++)
            {
                Output::Print(_u(" %s %d%d%d,"), GetTypePath()->GetPropertyId(i)->GetBuffer(),
                    i < this->GetTypePath()->GetMaxInitializedLength() ? 1 : 0,
                    this->GetTypePath()->GetIsFixedFieldAt(i, GetPathLength()) ? 1 : 0,
                    this->GetTypePath()->GetIsUsedFixedFieldAt(i, GetPathLength()) ? 1 : 0);
            }
        }
        else
        {
            for (PropertyIndex i = 0; i < GetPathLength(); i++)
            {
                Output::Print(_u(" %s %d%d%d,"), GetTypePath()->GetPropertyId(i)->GetBuffer(), 1, 0, 0);
            }
        }
    }

    void PathTypeHandlerBase::TraceFixedFieldsBeforeTypeHandlerChange(
        const char16* conversionName, const char16* oldTypeHandlerName, const char16* newTypeHandlerName,
        DynamicObject* instance, DynamicTypeHandler* oldTypeHandler,
        DynamicType* oldType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore)
    {
        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: %s 0x%p from %s to %s:\n"), conversionName, instance, oldTypeHandlerName, newTypeHandlerName);
            Output::Print(_u("   before: type = 0x%p, type handler = 0x%p, old singleton = 0x%p(0x%p)\n"),
                oldType, oldTypeHandler, oldSingletonInstanceBefore, oldSingletonInstanceBefore != nullptr ? oldSingletonInstanceBefore->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));
            oldTypeHandler->DumpFixedFields();
            Output::Print(_u("\n"));
        }
        if (PHASE_VERBOSE_TESTTRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: %s instance from %s to %s:\n"), conversionName, oldTypeHandlerName, newTypeHandlerName);
            Output::Print(_u("   old singleton before %s null \n"), oldSingletonInstanceBefore == nullptr ? _u("==") : _u("!="));
            Output::Print(_u("   fixed fields before:"));
            oldTypeHandler->DumpFixedFields();
            Output::Print(_u("\n"));
        }
    }

    void PathTypeHandlerBase::TraceFixedFieldsAfterTypeHandlerChange(
        DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicTypeHandler* newTypeHandler,
        DynamicType* oldType, DynamicType* newType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore)
    {
        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            RecyclerWeakReference<DynamicObject>* oldSingletonInstanceAfter = oldTypeHandler->GetSingletonInstance();
            RecyclerWeakReference<DynamicObject>* newSingletonInstanceAfter = newTypeHandler->GetSingletonInstance();
            Output::Print(_u("   after: type = 0x%p, type handler = 0x%p, old singleton = 0x%p(0x%p), new singleton = 0x%p(0x%p)\n"),
                newType, newTypeHandler, oldSingletonInstanceAfter, oldSingletonInstanceAfter != nullptr ? oldSingletonInstanceAfter->Get() : nullptr,
                newSingletonInstanceAfter, newSingletonInstanceAfter != nullptr ? newSingletonInstanceAfter->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));
            newTypeHandler->DumpFixedFields();
            Output::Print(_u("\n"));
            Output::Flush();
        }
        if (PHASE_VERBOSE_TESTTRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("   type %s, typeHandler %s, old singleton after %s null (%s), new singleton after %s null\n"),
                oldTypeHandler != newTypeHandler ? _u("changed") : _u("unchanged"),
                oldType != newType ? _u("changed") : _u("unchanged"),
                oldTypeHandler->GetSingletonInstance() == nullptr ? _u("==") : _u("!="),
                oldSingletonInstanceBefore != oldTypeHandler->GetSingletonInstance() ? _u("changed") : _u("unchanged"),
                newTypeHandler->GetSingletonInstance() == nullptr ? _u("==") : _u("!="));
            Output::Print(_u("   fixed fields after:"));
            newTypeHandler->DumpFixedFields();
            Output::Print(_u("\n"));
            Output::Flush();
        }
    }

    void PathTypeHandlerBase::TraceFixedFieldsBeforeSetIsProto(
        DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicType* oldType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore)
    {
        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: PathTypeHandler::SetIsPrototype(0x%p):\n"), instance);
            Output::Print(_u("   before: type = 0x%p, type handler = 0x%p, old singleton = 0x%p(0x%p)\n"),
                oldType, oldTypeHandler, oldSingletonInstanceBefore, oldSingletonInstanceBefore != nullptr ? oldSingletonInstanceBefore->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));
            oldTypeHandler->DumpFixedFields();
            Output::Print(_u("\n"));
        }
        if (PHASE_VERBOSE_TESTTRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: PathTypeHandler::SetIsPrototype():\n"));
            Output::Print(_u("   old singleton before %s null \n"), oldSingletonInstanceBefore == nullptr ? _u("==") : _u("!="));
            Output::Print(_u("   fixed fields before:"));
            oldTypeHandler->DumpFixedFields();
            Output::Print(_u("\n"));
        }
    }

    void PathTypeHandlerBase::TraceFixedFieldsAfterSetIsProto(
        DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicTypeHandler* newTypeHandler,
        DynamicType* oldType, DynamicType* newType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore)
    {
        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            RecyclerWeakReference<DynamicObject>* oldSingletonInstanceAfter = oldTypeHandler->GetSingletonInstance();
            RecyclerWeakReference<DynamicObject>* newSingletonInstanceAfter = newTypeHandler->GetSingletonInstance();
            Output::Print(_u("   after: type = 0x%p, type handler = 0x%p, old singleton = 0x%p(0x%p), new singleton = 0x%p(0x%p)\n"),
                instance->GetType(), newTypeHandler,
                oldSingletonInstanceAfter, oldSingletonInstanceAfter != nullptr ? oldSingletonInstanceAfter->Get() : nullptr,
                newSingletonInstanceAfter, newSingletonInstanceAfter != nullptr ? newSingletonInstanceAfter->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));
            newTypeHandler->DumpFixedFields();
            Output::Print(_u("\n"));
            Output::Flush();
        }
        if (PHASE_VERBOSE_TESTTRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("   type %s, old singleton after %s null (%s)\n"),
                oldType != newType ? _u("changed") : _u("unchanged"),
                oldSingletonInstanceBefore == nullptr ? _u("==") : _u("!="),
                oldSingletonInstanceBefore != oldTypeHandler->GetSingletonInstance() ? _u("changed") : _u("unchanged"));
            Output::Print(_u("   fixed fields after:"));
            newTypeHandler->DumpFixedFields();
            Output::Print(_u("\n"));
            Output::Flush();
        }
    }
#endif
#endif // ENABLE_FIXED_FIELDS

#if ENABLE_TTD
    void PathTypeHandlerBase::MarkObjectSlots_TTD(TTD::SnapshotExtractor* extractor, DynamicObject* obj) const
    {
        uint32 plength = this->GetPathLength();
        ObjectSlotAttributes * attributes = this->GetAttributeArray();

        for(uint32 index = 0; index < plength; ++index)
        {
            Js::PropertyId pid = GetTypePath()->GetPropertyIdUnchecked(index)->GetPropertyId();
            ObjectSlotAttributes attr = attributes ? attributes[index] : ObjectSlotAttr_Default;

            if (!DynamicTypeHandler::ShouldMarkPropertyId_TTD(pid) || (attr & ObjectSlotAttr_Deleted))
            {
                continue;
            }

            Js::Var value = obj->GetSlot(index);
            extractor->MarkVisitVar(value);
        }
    }

    uint32 PathTypeHandlerBase::ExtractSlotInfo_TTD(TTD::NSSnapType::SnapHandlerPropertyEntry* entryInfo, ThreadContext* threadContext, TTD::SlabAllocator& alloc) const
    {
        uint32 plength = this->GetPathLength();
        ObjectSlotAttributes * attributes = this->GetAttributeArray();

        for(uint32 index = 0; index < plength; ++index)
        {
            ObjectSlotAttributes attr = attributes ? attributes[index] : ObjectSlotAttr_Default;
            PropertyId propertyId = GetTypePath()->GetPropertyIdUnchecked(index)->GetPropertyId();
            TTD::NSSnapType::SnapEntryDataKindTag tag;
            if (attr == ObjectSlotAttr_Setter)
            {
                attr = attributes[GetTypePath()->LookupInline(propertyId, GetPathLength())];
                tag = TTD::NSSnapType::SnapEntryDataKindTag::Setter;
            }
            else if (attr & ObjectSlotAttr_Accessor)
            {
                tag = TTD::NSSnapType::SnapEntryDataKindTag::Getter;
            }
            else
            {
                tag = TTD::NSSnapType::SnapEntryDataKindTag::Data;
            }
            TTD::NSSnapType::ExtractSnapPropertyEntryInfo(entryInfo + index, propertyId, ObjectSlotAttributesToPropertyAttributes(attr), tag);
        }

        return plength;
    }

    Js::BigPropertyIndex PathTypeHandlerBase::GetPropertyIndex_EnumerateTTD(const Js::PropertyRecord* pRecord)
    {
        //The regular LookupInline is fine for path types
        return (Js::BigPropertyIndex)this->GetTypePath()->LookupInline(pRecord->GetPropertyId(), GetPathLength());
    }

    bool PathTypeHandlerBase::IsResetableForTTD(uint32 snapMaxIndex) const
    {
        return snapMaxIndex == this->GetPathLength();
    }
#endif

    PathTypeHandlerNoAttr * PathTypeHandlerNoAttr::New(ScriptContext * scriptContext, TypePath* typePath, uint16 pathLength, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked, bool isShared, DynamicType* predecessorType)
    {
        return New(scriptContext, typePath, pathLength, max(pathLength, inlineSlotCapacity), inlineSlotCapacity, offsetOfInlineSlots, isLocked, isShared, predecessorType);
    }

    PathTypeHandlerNoAttr * PathTypeHandlerNoAttr::New(ScriptContext * scriptContext, TypePath* typePath, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked, bool isShared, DynamicType* predecessorType)
    {
        Assert(typePath != nullptr);
#ifdef PROFILE_TYPES
        scriptContext->pathTypeHandlerCount++;
#endif
        return RecyclerNew(scriptContext->GetRecycler(), PathTypeHandlerNoAttr, typePath, pathLength, slotCapacity, inlineSlotCapacity, offsetOfInlineSlots, isLocked, isShared, predecessorType);
    }

    PathTypeHandlerNoAttr * PathTypeHandlerNoAttr::New(ScriptContext * scriptContext, PathTypeHandlerNoAttr * typeHandler, bool isLocked, bool isShared)
    {
        Assert(typeHandler != nullptr);
        return RecyclerNew(scriptContext->GetRecycler(), PathTypeHandlerNoAttr, typeHandler->GetTypePath(), typeHandler->GetPathLength(), static_cast<PropertyIndex>(typeHandler->GetSlotCapacity()), typeHandler->GetInlineSlotCapacity(), typeHandler->GetOffsetOfInlineSlots(), isLocked, isShared);
    }

    PathTypeHandlerNoAttr::PathTypeHandlerNoAttr(TypePath *typePath, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked, bool isShared, DynamicType* predecessorType) :
        PathTypeHandlerBase(typePath, pathLength, slotCapacity, inlineSlotCapacity, offsetOfInlineSlots, isLocked, isShared, predecessorType)
    {
    }

    PathTypeHandlerWithAttr * PathTypeHandlerWithAttr::New(ScriptContext * scriptContext, TypePath* typePath, ObjectSlotAttributes * attributes, PathTypeSetterSlotIndex * setters, PathTypeSetterSlotIndex setterCount, uint16 pathLength, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked, bool isShared, DynamicType* predecessorType)
    {
        return New(scriptContext, typePath, attributes, setters, setterCount, pathLength, max(pathLength, inlineSlotCapacity), inlineSlotCapacity, offsetOfInlineSlots, isLocked, isShared, predecessorType);
    }

    PathTypeHandlerWithAttr * PathTypeHandlerWithAttr::New(ScriptContext * scriptContext, TypePath* typePath, ObjectSlotAttributes * attributes, PathTypeSetterSlotIndex * setters, PathTypeSetterSlotIndex setterCount, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked, bool isShared, DynamicType* predecessorType)
    {
        Assert(typePath != nullptr);
#ifdef PROFILE_TYPES
        scriptContext->pathTypeHandlerCount++;
#endif
        return RecyclerNew(scriptContext->GetRecycler(), PathTypeHandlerWithAttr, typePath, attributes, setters, setterCount, pathLength, slotCapacity, inlineSlotCapacity, offsetOfInlineSlots, isLocked, isShared, predecessorType);
    }

    PathTypeHandlerWithAttr::PathTypeHandlerWithAttr(TypePath *typePath, ObjectSlotAttributes * attributes, PathTypeSetterSlotIndex * setters, PathTypeSetterSlotIndex setterCount, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked, bool isShared, DynamicType* predecessorType) :
        PathTypeHandlerNoAttr(typePath, pathLength, slotCapacity, inlineSlotCapacity, offsetOfInlineSlots, isLocked, isShared, predecessorType),
        attributes(attributes),
        setters(setters),
        setterCount(setterCount)
    {
    }

    BOOL PathTypeHandlerWithAttr::IsEnumerable(DynamicObject* instance, PropertyId propertyId)
    {
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex != Constants::NoSlot)
        {
            Assert(attributes);
            return (attributes[propertyIndex] & ObjectSlotAttr_Enumerable);
        }
        return true;
    }

    BOOL PathTypeHandlerWithAttr::IsWritable(DynamicObject* instance, PropertyId propertyId)
    {
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex != Constants::NoSlot)
        {
            Assert(attributes);
            return (attributes[propertyIndex] & ObjectSlotAttr_Writable);
        }
        return true;
    }

    BOOL PathTypeHandlerWithAttr::IsConfigurable(DynamicObject* instance, PropertyId propertyId)
    {
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex != Constants::NoSlot)
        {
            Assert(attributes);
            return (attributes[propertyIndex] & ObjectSlotAttr_Configurable);
        }
        return true;
    }

    BOOL PathTypeHandlerWithAttr::SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        // Find the property
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            if (!value)
            {
                // Upgrade type handler if set objectArray item attribute.
                // Only check numeric propertyId if objectArray available.
                if (instance->HasObjectArray())
                {
                    PropertyRecord const* propertyRecord = instance->GetScriptContext()->GetPropertyName(propertyId);
                    if (propertyRecord->IsNumeric())
                    {
                        return ConvertToTypeWithItemAttributes(instance)->SetConfigurable(instance, propertyId, value);
                    }
                }
            }
            return true;
        }

        ObjectSlotAttributes attr =
            (ObjectSlotAttributes)(value ? (attributes[propertyIndex] | ObjectSlotAttr_Configurable) : (attributes[propertyIndex] & ~ObjectSlotAttr_Configurable));
        return SetAttributesHelper(instance, propertyId, propertyIndex, attributes, attr);
    }

    BOOL PathTypeHandlerWithAttr::SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        // Find the property
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            if (!value)
            {
                // Upgrade type handler if set objectArray item attribute.
                // Only check numeric propertyId if objectArray available.
                if (instance->HasObjectArray())
                {
                    PropertyRecord const* propertyRecord = instance->GetScriptContext()->GetPropertyName(propertyId);
                    if (propertyRecord->IsNumeric())
                    {
                        return ConvertToTypeWithItemAttributes(instance)->SetEnumerable(instance, propertyId, value);
                    }
                }
            }
            return true;
        }

        ObjectSlotAttributes attr =
            (ObjectSlotAttributes)(value ? (attributes[propertyIndex] | ObjectSlotAttr_Enumerable) : (attributes[propertyIndex] & ~ObjectSlotAttr_Enumerable));
        return SetAttributesHelper(instance, propertyId, propertyIndex, attributes, attr);
    }

    BOOL PathTypeHandlerWithAttr::SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        // Find the property
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            if (!value)
            {
                // Upgrade type handler if set objectArray item attribute.
                // Only check numeric propertyId if objectArray available.
                if (instance->HasObjectArray())
                {
                    PropertyRecord const* propertyRecord = instance->GetScriptContext()->GetPropertyName(propertyId);
                    if (propertyRecord->IsNumeric())
                    {
                        return ConvertToTypeWithItemAttributes(instance)->SetWritable(instance, propertyId, value);
                    }
                }
            }
            return true;
        }

        ObjectSlotAttributes attr =
            (ObjectSlotAttributes)(value ? (attributes[propertyIndex] | ObjectSlotAttr_Writable) : (attributes[propertyIndex] & ~ObjectSlotAttr_Writable));
        return SetAttributesHelper(instance, propertyId, propertyIndex, attributes, attr);
    }

    _Check_return_ _Success_(return) BOOL PathTypeHandlerWithAttr::GetAccessors(DynamicObject* instance, PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter)
    {
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            return false;
        }
        if (!(attributes[propertyIndex] & ObjectSlotAttr_Accessor))
        {
            return false;
        }
        *getter = instance->GetSlot(propertyIndex);
        *setter = instance->GetSlot(setters[propertyIndex]);
        return true;
    }

    BOOL PathTypeHandlerWithAttr::SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        return SetAccessorsHelper(instance, propertyId, attributes, setters, getter, setter, flags);
    }

#if ENABLE_NATIVE_CODEGEN
    bool PathTypeHandlerWithAttr::IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex)
    {
        return IsObjTypeSpecEquivalentHelper(type, attributes, record, failedPropertyIndex);
    }

    bool PathTypeHandlerWithAttr::IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry *entry)
    {
        return IsObjTypeSpecEquivalentHelper(type, attributes, entry);
    }
#endif

    DescriptorFlags PathTypeHandlerWithAttr::GetSetter(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyIndex propertyIndex = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (propertyIndex == Constants::NoSlot)
        {
            return __super::GetSetter(instance, propertyId, setterValue, info, requestContext);
        }
        ObjectSlotAttributes attr = attributes[propertyIndex];
        if (attr & ObjectSlotAttr_Deleted)
        {
            return None;
        }

        if (attr & ObjectSlotAttr_Accessor)
        {
            Assert(setters != nullptr);
            PathTypeSetterSlotIndex setterSlot = setters[propertyIndex];
            Assert(setterSlot != NoSetterSlot && setterSlot < GetPathLength());
            AssertOrFailFast(VarIsCorrectType(instance));
            *setterValue = instance->GetSlot(setterSlot);
            PropertyValueInfo::Set(info, instance, setterSlot, ObjectSlotAttributesToPropertyAttributes(attr), InlineCacheSetterFlag);
            return Accessor;
        }

        if (attr & ObjectSlotAttr_Writable)
        {
            return WritableData;
        }
        return Data;
    }

    DescriptorFlags PathTypeHandlerWithAttr::GetSetter(DynamicObject* instance, JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        instance->GetScriptContext()->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        return this->GetSetter(instance, propertyRecord->GetPropertyId(), setterValue, info, requestContext);
    }

    int PathTypeHandlerWithAttr::GetPropertyCountForEnum()
    {
        // Enumerate only unique properties, i.e., don't count setters
        return GetPropertyCount() - setterCount;
    }

    BOOL PathTypeHandlerWithAttr::HasProperty(DynamicObject* instance, PropertyId propertyId, __out_opt bool *noRedecl, _Inout_opt_ PropertyValueInfo* info)
    {
        if (noRedecl != nullptr)
        {
            *noRedecl = false;
        }

        PropertyIndex index = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (index == Constants::NoSlot)
        {
            return __super::HasProperty(instance, propertyId, noRedecl, info);
        }

        ObjectSlotAttributes attr = attributes[index];

        if (attr & ObjectSlotAttr_Deleted)
        {
            return false;
        }

        if (attr & ObjectSlotAttr_Accessor)
        {
            // PropertyAttributes is only one byte so it can't carry out data about whether this is an accessor.
            // Accessors must be cached differently than normal properties, so if we want to cache this we must
            // do so here rather than in the caller. However, caching here would require passing originalInstance and
            // requestContext through a wide variety of call paths to this point (like we do for GetProperty), for
            // very little improvement. For now, just block caching this case.
            PropertyValueInfo::SetNoCache(info, instance);
            return true;
        }

        this->SetPropertyValueInfo(info, instance, index, attr);
        return true;
    }

    BOOL PathTypeHandlerWithAttr::GetProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyIndex index = GetTypePath()->LookupInline(propertyId, GetPathLength());
        if (index == Constants::NoSlot)
        {
            return __super::GetProperty(instance, originalInstance, propertyId, value, info, requestContext);
        }

        ObjectSlotAttributes attr = attributes[index];
        if (attr & ObjectSlotAttr_Deleted)
        {
            return false;
        }

        if (attr & ObjectSlotAttr_Accessor)
        {
            // We must update cache before calling a getter, because it can invalidate something. Bug# 593815
            PropertyValueInfo::Set(info, instance, index, ObjectSlotAttributesToPropertyAttributes(attr));
            CacheOperators::CachePropertyReadForGetter(info, originalInstance, propertyId, requestContext);
            PropertyValueInfo::SetNoCache(info, instance); // we already cached getter, so we don't have to do it once more

            RecyclableObject* func = UnsafeVarTo<RecyclableObject>(instance->GetSlot(index));
            *value = JavascriptOperators::CallGetter(func, originalInstance, requestContext);
            return true;
        }

        *value = instance->GetSlot(index);
        this->SetPropertyValueInfo(info, instance, index, attr);
        return true;
    }

    BOOL PathTypeHandlerWithAttr::GetProperty(DynamicObject* instance, Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        // Consider: Implement actual string hash lookup
        Assert(requestContext);
        PropertyRecord const* propertyRecord;
        char16 const * propertyName = propertyNameString->GetString();
        charcount_t const propertyNameLength = propertyNameString->GetLength();

        if (instance->HasObjectArray())
        {
            requestContext->GetOrAddPropertyRecord(propertyName, propertyNameLength, &propertyRecord);
        }
        else
        {
            requestContext->FindPropertyRecord(propertyName, propertyNameLength, &propertyRecord);
            if (propertyRecord == nullptr)
            {
                *value = requestContext->GetMissingPropertyResult();
                return false;
            }
        }
        return PathTypeHandlerWithAttr::GetProperty(instance, originalInstance, propertyRecord->GetPropertyId(), value, info, requestContext);
    }

    BOOL PathTypeHandlerWithAttr::SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PropertyIndex index = PathTypeHandlerBase::GetPropertyIndex(propertyId);
        if (index != Constants::NoSlot)
        {
            if (attributes[index] & ObjectSlotAttr_Accessor)
            {
                Assert(setters[index] != Constants::NoSlot);
                RecyclableObject* func = VarTo<RecyclableObject>(instance->GetSlot(setters[index]));
                JavascriptOperators::CallSetter(func, instance, value, NULL);

                // Wait for the setter to return before setting up the inline cache info, as the setter may change
                // the attributes
                DynamicTypeHandler *typeHandler = instance->GetTypeHandler();
                if (typeHandler == this)
                {
                    // Common path: setter didn't change the handler. Do the rest without virtual calls.
                    if (attributes[index] & ObjectSlotAttr_Deleted)
                    {
                        // Delete and re-add may have changed the index.
                        index = PathTypeHandlerBase::GetPropertyIndex(propertyId);
                        if (attributes[index] & ObjectSlotAttr_Deleted)
                        {
                            // Current state is really "deleted". Don't cache.
                            return TRUE;
                        }
                    }
                    Assert(index == PathTypeHandlerBase::GetPropertyIndex(propertyId));

                    if (attributes[index] & ObjectSlotAttr_Accessor)
                    {
                        PropertyValueInfo::Set(info, instance, setters[index], ObjectSlotAttributesToPropertyAttributes(attributes[index]), InlineCacheSetterFlag);
                    }
                    else
                    {
                        PropertyValueInfo::Set(info, instance, index, ObjectSlotAttributesToPropertyAttributes(attributes[index]), InlineCacheNoFlags);
                    }
                }
                else
                {
                    PropertyAttributes attributes;
                    index = typeHandler->GetPropertyIndex(instance->GetScriptContext()->GetPropertyName(propertyId));
                    if (index == Constants::NoSlot)
                    {
                        return TRUE;
                    }
                    if (typeHandler->GetAttributesWithPropertyIndex(instance, propertyId, index, &attributes) == FALSE)
                    {
                        return TRUE;
                    }
                    if (attributes & PropertyDeleted)
                    {
                        return TRUE;
                    }
                    DescriptorFlags descriptorFlags = typeHandler->GetSetter(instance, propertyId, (Var*)&func, info, instance->GetScriptContext());
                    if (descriptorFlags & Data)
                    {
                        PropertyValueInfo::Set(info, instance, index, attributes, InlineCacheNoFlags);
                    }
                }

                return TRUE;
            }
            else if (attributes[index] & ObjectSlotAttr_Deleted)
            {
                AssertMsg(0, "Re-add of deleted property NYI in PathTypeHandler");
            }
        }
        return SetPropertyInternal<false>(instance, propertyId, index, value, ObjectSlotAttr_Default, info, flags, SideEffects_Any);
    }

    BOOL PathTypeHandlerWithAttr::SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        // Consider: Implement actual string hash lookup
        PropertyRecord const* propertyRecord;
        instance->GetScriptContext()->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        return PathTypeHandlerWithAttr::SetProperty(instance, propertyRecord->GetPropertyId(), value, flags, info);
    }

    BOOL PathTypeHandlerWithAttr::GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes)
    {
        if (index < this->GetPathLength())
        {
            Assert(this->GetPropertyId(instance->GetScriptContext(), index) == propertyId);
            *attributes = ObjectSlotAttributesToPropertyAttributes(this->attributes[index]);
            return true;
        }
        return false;
    }
}

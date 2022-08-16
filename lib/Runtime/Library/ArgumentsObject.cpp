//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#include "ArgumentsObjectEnumerator.h"

namespace Js
{
    BOOL ArgumentsObject::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, EnumeratorCache * enumeratorCache)
    {
        return GetEnumeratorWithPrefix(
            RecyclerNew(GetScriptContext()->GetRecycler(), ArgumentsObjectPrefixEnumerator, this, flags, requestContext),
            enumerator, flags, requestContext, enumeratorCache);
    }

    BOOL ArgumentsObject::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("{...}"));
        return TRUE;
    }

    BOOL ArgumentsObject::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object, (Arguments)"));
        return TRUE;
    }

    HeapArgumentsObject::HeapArgumentsObject(DynamicType * type) : ArgumentsObject(type), frameObject(nullptr), formalCount(0),
        numOfArguments(0), callerDeleted(false), deletedArgs(nullptr)
    {
    }

    HeapArgumentsObject::HeapArgumentsObject(Recycler *recycler, ActivationObject* obj, uint32 formalCount, DynamicType * type)
        : ArgumentsObject(type), frameObject(obj), formalCount(formalCount), numOfArguments(0), callerDeleted(false), deletedArgs(nullptr)
    {
        Assert(!frameObject || VarIsCorrectType(frameObject));
    }

    void HeapArgumentsObject::SetNumberOfArguments(uint32 len)
    {
        numOfArguments = len;
    }

    uint32 HeapArgumentsObject::GetNumberOfArguments() const
    {
        return numOfArguments;
    }

    HeapArgumentsObject* HeapArgumentsObject::As(Var aValue)
    {
        if (VarIs<ArgumentsObject>(aValue) && static_cast<ArgumentsObject*>(VarTo<RecyclableObject>(aValue))->GetHeapArguments() == aValue)
        {
            return static_cast<HeapArgumentsObject*>(VarTo<RecyclableObject>(aValue));
        }
        return NULL;
    }

    BOOL HeapArgumentsObject::AdvanceWalkerToArgsFrame(JavascriptStackWalker *walker)
    {
        // Walk until we find this arguments object on the frame.
        // Note that each frame may have a HeapArgumentsObject
        // associated with it. Look for the HeapArgumentsObject.
        while (walker->Walk())
        {
            if (walker->IsJavascriptFrame() && walker->GetCurrentFunction()->IsScriptFunction())
            {
                Var args = walker->GetPermanentArguments();
                if (args == this)
                {
                    return TRUE;
                }
            }
        }

        return FALSE;
    }

    //
    // Get the next valid formal arg index held in this object.
    //
    uint32 HeapArgumentsObject::GetNextFormalArgIndex(uint32 index, BOOL enumNonEnumerable, PropertyAttributes* attributes) const
    {
        if (attributes != nullptr)
        {
            *attributes = PropertyEnumerable;
        }

        if ( ++index < formalCount )
        {
            // None of the arguments deleted
            if ( deletedArgs == nullptr )
            {
                return index;
            }

            while ( index < formalCount )
            {
                if (!this->deletedArgs->Test(index))
                {
                    return index;
                }

                index++;
            }
        }

        return JavascriptArray::InvalidIndex;
    }

    BOOL HeapArgumentsObject::HasItemAt(uint32 index)
    {
        // If this arg index is bound to a named formal argument, get it from the local frame.
        // If not, report this fact to the caller, which will defer to the normal get-value-by-index means.
        if (IsFormalArgument(index) && (this->deletedArgs == nullptr || !this->deletedArgs->Test(index)) )
        {
            return true;
        }

        return false;
    }

    BOOL HeapArgumentsObject::GetItemAt(uint32 index, Var* value, ScriptContext* requestContext)
    {
        // If this arg index is bound to a named formal argument, get it from the local frame.
        // If not, report this fact to the caller, which will defer to the normal get-value-by-index means.
        if (HasItemAt(index))
        {
            *value = this->frameObject->GetSlot(index);
            return true;
        }

        return false;
    }

    BOOL HeapArgumentsObject::SetItemAt(uint32 index, Var value)
    {
        // If this arg index is bound to a named formal argument, set it in the local frame.
        // If not, report this fact to the caller, which will defer to the normal set-value-by-index means.
        if (HasItemAt(index))
        {
            this->frameObject->SetSlot(SetSlotArguments(Constants::NoProperty, index, value));
            return true;
        }

        return false;
    }

    BOOL HeapArgumentsObject::DeleteItemAt(uint32 index)
    {
        if (index < formalCount)
        {
            if (this->deletedArgs == nullptr)
            {
                Recycler *recycler = GetScriptContext()->GetRecycler();
                deletedArgs = RecyclerNew(recycler, BVSparse<Recycler>, recycler);
            }

            if (!this->deletedArgs->Test(index))
            {
                this->deletedArgs->Set(index);
                return true;
            }
        }

        return false;
    }

    BOOL HeapArgumentsObject::IsFormalArgument(uint32 index)
    {
        return index < this->numOfArguments && index < formalCount;
    }

    BOOL HeapArgumentsObject::IsFormalArgument(PropertyId propertyId)
    {
        uint32 index;
        return IsFormalArgument(propertyId, &index);
    }

    BOOL HeapArgumentsObject::IsFormalArgument(PropertyId propertyId, uint32* pIndex)
    {
        return
            this->GetScriptContext()->IsNumericPropertyId(propertyId, pIndex) &&
            IsFormalArgument(*pIndex);
    }

    BOOL HeapArgumentsObject::IsArgumentDeleted(uint32 index) const
    {
        return this->deletedArgs != NULL && this->deletedArgs->Test(index);
    }

#if ENABLE_TTD
    void HeapArgumentsObject::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        if(this->frameObject != nullptr)
        {
            extractor->MarkVisitVar(this->frameObject);
        }
    }

    TTD::NSSnapObjects::SnapObjectType HeapArgumentsObject::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapHeapArgumentsObject;
    }

    void HeapArgumentsObject::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        this->ExtractSnapObjectDataInto_Helper<TTD::NSSnapObjects::SnapObjectType::SnapHeapArgumentsObject>(objData, alloc);
    }

    template <TTD::NSSnapObjects::SnapObjectType argsKind>
    void HeapArgumentsObject::ExtractSnapObjectDataInto_Helper(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapHeapArgumentsInfo* argsInfo = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapHeapArgumentsInfo>();

        TTDAssert(this->callerDeleted == 0, "This never seems to be set but I want to assert just to be safe.");
        argsInfo->NumOfArguments = this->numOfArguments;
        argsInfo->FormalCount = this->formalCount;

        uint32 depOnCount = 0;
        TTD_PTR_ID* depOnArray = nullptr;

        argsInfo->IsFrameNullPtr = false;
        argsInfo->FrameObject = TTD_INVALID_PTR_ID;
        if(this->frameObject == nullptr)
        {
            argsInfo->IsFrameNullPtr = true;
        }
        else
        {
            argsInfo->FrameObject = TTD_CONVERT_VAR_TO_PTR_ID(this->frameObject);

            //Primitive kinds always inflated first so we only need to deal with complex kinds as depends on
            if(TTD::JsSupport::IsVarComplexKind(this->frameObject))
            {
                depOnCount = 1;
                depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);
                depOnArray[0] = argsInfo->FrameObject;
            }
        }

        argsInfo->DeletedArgFlags = (this->formalCount != 0) ? alloc.SlabAllocateArrayZ<byte>(argsInfo->FormalCount) : nullptr;
        if(this->deletedArgs != nullptr)
        {
            for(uint32 i = 0; i < this->formalCount; ++i)
            {
                if(this->deletedArgs->Test(i))
                {
                    argsInfo->DeletedArgFlags[i] = true;
                }
            }
        }

        if(depOnCount == 0)
        {
            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapHeapArgumentsInfo*, argsKind>(objData, argsInfo);
        }
        else
        {
            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapHeapArgumentsInfo*, argsKind>(objData, argsInfo, alloc, depOnCount, depOnArray);
        }
    }

    ES5HeapArgumentsObject* HeapArgumentsObject::ConvertToES5HeapArgumentsObject_TTD()
    {
        Assert(VirtualTableInfo<HeapArgumentsObject>::HasVirtualTable(this));
        VirtualTableInfo<ES5HeapArgumentsObject>::SetVirtualTable(this);

        return static_cast<ES5HeapArgumentsObject*>(this);
    }
#endif

    ES5HeapArgumentsObject* HeapArgumentsObject::ConvertToUnmappedArgumentsObject(bool overwriteArgsUsingFrameObject)
    {
        ES5HeapArgumentsObject* es5ArgsObj = ConvertToES5HeapArgumentsObject(overwriteArgsUsingFrameObject);

        for (uint i=0; i<this->formalCount && i<numOfArguments; ++i)
        {
            es5ArgsObj->DisconnectFormalFromNamedArgument(i);
        }

        return es5ArgsObj;
    }

    ES5HeapArgumentsObject* HeapArgumentsObject::ConvertToES5HeapArgumentsObject(bool overwriteArgsUsingFrameObject)
    {
        if (!CrossSite::IsCrossSiteObjectTyped(this))
        {
            Assert(VirtualTableInfo<HeapArgumentsObject>::HasVirtualTable(this));
            VirtualTableInfo<ES5HeapArgumentsObject>::SetVirtualTable(this);
        }
        else
        {
            Assert(VirtualTableInfo<CrossSiteObject<HeapArgumentsObject>>::HasVirtualTable(this));
            VirtualTableInfo<CrossSiteObject<ES5HeapArgumentsObject>>::SetVirtualTable(this);
        }

        ES5HeapArgumentsObject* es5This = static_cast<ES5HeapArgumentsObject*>(this);

        if (overwriteArgsUsingFrameObject)
        {
            // Copy existing items to the array so that they are there before the user can call Object.preventExtensions,
            // after which we can no longer add new items to the objectArray.
            for (uint32 i = 0; i < this->formalCount && i < this->numOfArguments; ++i)
            {
                AssertMsg(this->IsFormalArgument(i), "this->IsFormalArgument(i)");
                if (!this->IsArgumentDeleted(i))
                {
                    // At this time the value doesn't matter, use one from slots.
                    this->SetObjectArrayItem(i, this->frameObject->GetSlot(i), PropertyOperation_None);
                }
            }
        }
        return es5This;

    }

    PropertyQueryFlags HeapArgumentsObject::HasPropertyQuery(PropertyId id, _Inout_opt_ PropertyValueInfo* info)
    {
        ScriptContext *scriptContext = GetScriptContext();

        // Try to get a numbered property that maps to an actual argument.
        uint32 index;
        if (scriptContext->IsNumericPropertyId(id, &index) && index < this->HeapArgumentsObject::GetNumberOfArguments())
        {
            return HeapArgumentsObject::HasItemQuery(index);
        }

        return DynamicObject::HasPropertyQuery(id, info);
    }

    PropertyQueryFlags HeapArgumentsObject::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        ScriptContext *scriptContext = GetScriptContext();

        // Try to get a numbered property that maps to an actual argument.
        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index) && index < this->HeapArgumentsObject::GetNumberOfArguments())
        {
            if (this->GetItemAt(index, value, requestContext))
            {
                return PropertyQueryFlags::Property_Found;
            }
        }

        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext)))
        {
            // Property has been pre-set and not deleted. Use it.
            return PropertyQueryFlags::Property_Found;
        }

        *value = requestContext->GetMissingPropertyResult();
        return PropertyQueryFlags::Property_NotFound;
    }

    PropertyQueryFlags HeapArgumentsObject::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {

        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord*");

        if (JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext)))
        {
            // Property has been pre-set and not deleted. Use it.
            return PropertyQueryFlags::Property_Found;
        }

        *value = requestContext->GetMissingPropertyResult();
        return PropertyQueryFlags::Property_NotFound;
    }

    PropertyQueryFlags HeapArgumentsObject::GetPropertyReferenceQuery(Var originalInstance, PropertyId id, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return HeapArgumentsObject::GetPropertyQuery(originalInstance, id, value, info, requestContext);
    }

    BOOL HeapArgumentsObject::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        // Try to set a numbered property that maps to an actual argument.
        ScriptContext *scriptContext = GetScriptContext();

        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index) && index < this->HeapArgumentsObject::GetNumberOfArguments())
        {
            if (this->SetItemAt(index, value))
            {
                return true;
            }
        }

        // TODO: In strict mode, "callee" and "caller" cannot be set.

        // length is also set in the dynamic object
        return DynamicObject::SetProperty(propertyId, value, flags, info);
    }

    BOOL HeapArgumentsObject::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord*");

        // TODO: In strict mode, "callee" and "caller" cannot be set.

        // length is also set in the dynamic object
        return DynamicObject::SetProperty(propertyNameString, value, flags, info);
    }

    PropertyQueryFlags HeapArgumentsObject::HasItemQuery(uint32 index)
    {
        if (this->HasItemAt(index))
        {
            return PropertyQueryFlags::Property_Found;
        }
        return DynamicObject::HasItemQuery(index);
    }

    PropertyQueryFlags HeapArgumentsObject::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        if (this->GetItemAt(index, value, requestContext))
        {
            return PropertyQueryFlags::Property_Found;
        }
        return DynamicObject::GetItemQuery(originalInstance, index, value, requestContext);
    }

    PropertyQueryFlags HeapArgumentsObject::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return HeapArgumentsObject::GetItemQuery(originalInstance, index, value, requestContext);
    }

    BOOL HeapArgumentsObject::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        if (this->SetItemAt(index, value))
        {
            return true;
        }
        return DynamicObject::SetItem(index, value, flags);
    }

    BOOL HeapArgumentsObject::DeleteItem(uint32 index, PropertyOperationFlags flags)
    {
        if (this->DeleteItemAt(index))
        {
            return true;
        }
        return DynamicObject::DeleteItem(index, flags);
    }

    BOOL HeapArgumentsObject::SetConfigurable(PropertyId propertyId, BOOL value)
    {
        // Try to set a numbered property that maps to an actual argument.
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            // From now on, make sure that frame object is ES5HeapArgumentsObject.
            return this->ConvertToES5HeapArgumentsObject()->SetConfigurableForFormal(index, propertyId, value);
        }

        // Use 'this' dynamic object.
        // This will use type handler and convert its objectArray to ES5Array is not already converted.
        return __super::SetConfigurable(propertyId, value);
    }

    BOOL HeapArgumentsObject::SetEnumerable(PropertyId propertyId, BOOL value)
    {
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return this->ConvertToES5HeapArgumentsObject()->SetEnumerableForFormal(index, propertyId, value);
        }
        return __super::SetEnumerable(propertyId, value);
    }

    BOOL HeapArgumentsObject::SetWritable(PropertyId propertyId, BOOL value)
    {
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return this->ConvertToES5HeapArgumentsObject()->SetWritableForFormal(index, propertyId, value);
        }
        return __super::SetWritable(propertyId, value);
    }

    BOOL HeapArgumentsObject::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return this->ConvertToES5HeapArgumentsObject()->SetAccessorsForFormal(index, propertyId, getter, setter, flags);
        }
        return __super::SetAccessors(propertyId, getter, setter, flags);
    }

    BOOL HeapArgumentsObject::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        // This is called by defineProperty in order to change the value in objectArray.
        // We have to intercept this call because
        // in case when we are connected (objectArray item is not used) we need to update the slot value (which is actually used).
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return this->ConvertToES5HeapArgumentsObject()->SetPropertyWithAttributesForFormal(
                index, propertyId, value, attributes, info, flags, possibleSideEffects);
        }
        return __super::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    // This disables adding new properties to the object.
    BOOL HeapArgumentsObject::PreventExtensions()
    {
        // It's possible that after call to preventExtensions, the user can change the attributes;
        // In this case if we don't covert to ES5 version now, later we would not be able to add items to objectArray,
        // which would cause not being able to change attributes.
        // So, convert to ES5 now which will make sure the items are there.
        return this->ConvertToES5HeapArgumentsObject()->PreventExtensions();
    }

    // This is equivalent to .preventExtensions semantics with addition of setting configurable to false for all properties.
    BOOL HeapArgumentsObject::Seal()
    {
        // Same idea as with PreventExtensions: we have to make sure that items in objectArray for formals
        // are there before seal, otherwise we will not be able to add them later.
        return this->ConvertToES5HeapArgumentsObject()->Seal();
    }

    // This is equivalent to .seal semantics with addition of setting writable to false for all properties.
    BOOL HeapArgumentsObject::Freeze()
    {
        // Same idea as with PreventExtensions: we have to make sure that items in objectArray for formals
        // are there before seal, otherwise we will not be able to add them later.
        return this->ConvertToES5HeapArgumentsObject()->Freeze();
    }

    //---------------------- ES5HeapArgumentsObject -------------------------------

    BOOL ES5HeapArgumentsObject::SetConfigurable(PropertyId propertyId, BOOL value)
    {
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return this->SetConfigurableForFormal(index, propertyId, value);
        }
        return this->DynamicObject::SetConfigurable(propertyId, value);
    }

    BOOL ES5HeapArgumentsObject::SetEnumerable(PropertyId propertyId, BOOL value)
    {
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return this->SetEnumerableForFormal(index, propertyId, value);
        }
        return this->DynamicObject::SetEnumerable(propertyId, value);
    }

    BOOL ES5HeapArgumentsObject::SetWritable(PropertyId propertyId, BOOL value)
    {
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return this->SetWritableForFormal(index, propertyId, value);
        }
        return this->DynamicObject::SetWritable(propertyId, value);
    }

    BOOL ES5HeapArgumentsObject::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return SetAccessorsForFormal(index, propertyId, getter, setter);
        }
        return this->DynamicObject::SetAccessors(propertyId, getter, setter, flags);
    }

    BOOL ES5HeapArgumentsObject::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        // This is called by defineProperty in order to change the value in objectArray.
        // We have to intercept this call because
        // in case when we are connected (objectArray item is not used) we need to update the slot value (which is actually used).
        uint32 index;
        if (this->IsFormalArgument(propertyId, &index))
        {
            return this->SetPropertyWithAttributesForFormal(index, propertyId, value, attributes, info, flags, possibleSideEffects);
        }

        return this->DynamicObject::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    BOOL ES5HeapArgumentsObject::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, EnumeratorCache * enumeratorCache)
    {
        ES5ArgumentsObjectEnumerator * es5HeapArgumentsObjectEnumerator = ES5ArgumentsObjectEnumerator::New(this, flags, requestContext, enumeratorCache);
        if (es5HeapArgumentsObjectEnumerator == nullptr)
        {
            return false;
        }

        return enumerator->Initialize(es5HeapArgumentsObjectEnumerator, nullptr, nullptr, flags, requestContext, nullptr);
    }

    BOOL ES5HeapArgumentsObject::PreventExtensions()
    {
        return this->DynamicObject::PreventExtensions();
    }

    BOOL ES5HeapArgumentsObject::Seal()
    {
        return this->DynamicObject::Seal();
    }

    BOOL ES5HeapArgumentsObject::Freeze()
    {
        return this->DynamicObject::Freeze();
    }

    BOOL ES5HeapArgumentsObject::GetItemAt(uint32 index, Var* value, ScriptContext* requestContext)
    {
        return this->IsFormalDisconnectedFromNamedArgument(index) ?
            JavascriptConversion::PropertyQueryFlagsToBoolean(this->DynamicObject::GetItemQuery(this, index, value, requestContext)) :
            __super::GetItemAt(index, value, requestContext);
    }

    BOOL ES5HeapArgumentsObject::SetItemAt(uint32 index, Var value)
    {
        return this->IsFormalDisconnectedFromNamedArgument(index) ?
            this->DynamicObject::SetItem(index, value, PropertyOperation_None) :
            __super::SetItemAt(index, value);
    }

    BOOL ES5HeapArgumentsObject::DeleteItemAt(uint32 index)
    {
        BOOL result = __super::DeleteItemAt(index);
        if (result && IsFormalArgument(index))
        {
            AssertMsg(this->IsFormalDisconnectedFromNamedArgument(index), "__super::DeleteItemAt must perform the disconnect.");
            // Make sure that objectArray does not have the item ().
            if (this->HasObjectArrayItem(index))
            {
                this->DeleteObjectArrayItem(index, PropertyOperation_None);
            }
        }

        return result;
    }

    //
    // Get the next valid formal arg index held in this object.
    //
    uint32 ES5HeapArgumentsObject::GetNextFormalArgIndex(uint32 index, BOOL enumNonEnumerable, PropertyAttributes* attributes) const
    {
        return GetNextFormalArgIndexHelper(index, enumNonEnumerable, attributes);
    }

    uint32 ES5HeapArgumentsObject::GetNextFormalArgIndexHelper(uint32 index, BOOL enumNonEnumerable, PropertyAttributes* attributes) const
    {
        // Formals:
        // - deleted => not in objectArray && not connected -- do not enum, do not advance.
        // - connected,     if in objectArray -- if (enumerable) enum it, advance objectEnumerator.
        // - disconnected =>in objectArray -- if (enumerable) enum it, advance objectEnumerator.

        // We use GetFormalCount and IsEnumerableByIndex which don't change the object
        // but are not declared as const, so do a const cast.
        ES5HeapArgumentsObject* mutableThis = const_cast<ES5HeapArgumentsObject*>(this);
        uint32 formalCount = this->GetFormalCount();
        while (++index < formalCount)
        {
            bool isDeleted = mutableThis->IsFormalDisconnectedFromNamedArgument(index) &&
                !mutableThis->HasObjectArrayItem(index);

            if (!isDeleted)
            {
                BOOL isEnumerable = mutableThis->IsEnumerableByIndex(index);

                if (enumNonEnumerable || isEnumerable)
                {
                    if (attributes != nullptr && isEnumerable)
                    {
                        *attributes = PropertyEnumerable;
                    }

                    return index;
                }
            }
        }

        return JavascriptArray::InvalidIndex;
    }

    // Disconnects indexed argument from named argument for frame object property.
    // Remove association from them map. From now on (or still) for this argument,
    // named argument's value is no longer associated with arguments[] item.
    void ES5HeapArgumentsObject::DisconnectFormalFromNamedArgument(uint32 index)
    {
        AssertMsg(this->IsFormalArgument(index), "SetAccessorsForFormal: called for non-formal");

        if (!IsFormalDisconnectedFromNamedArgument(index))
        {
            __super::DeleteItemAt(index);
        }
    }

    BOOL ES5HeapArgumentsObject::IsFormalDisconnectedFromNamedArgument(uint32 index)
    {
        return this->IsArgumentDeleted(index);
    }

    // Wrapper over IsEnumerable by uint32 index.
    BOOL ES5HeapArgumentsObject::IsEnumerableByIndex(uint32 index)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        Var indexNumber = JavascriptNumber::New(index, scriptContext);
        JavascriptString* indexPropertyName = JavascriptConversion::ToString(indexNumber, scriptContext);
        PropertyRecord const * propertyRecord;
        scriptContext->GetOrAddPropertyRecord(indexPropertyName, &propertyRecord);
        return this->IsEnumerable(propertyRecord->GetPropertyId());
    }

    // Helper method, just to avoid code duplication.
    BOOL ES5HeapArgumentsObject::SetConfigurableForFormal(uint32 index, PropertyId propertyId, BOOL value)
    {
        AssertMsg(this->IsFormalArgument(index), "SetAccessorsForFormal: called for non-formal");

        // In order for attributes to work, the item must exist. Make sure that's the case.
        // If we were connected, we have to copy the value from frameObject->slots, otherwise which value doesn't matter.
        AutoObjectArrayItemExistsValidator autoItemAddRelease(this, index);

        BOOL result = this->DynamicObject::SetConfigurable(propertyId, value);
        autoItemAddRelease.m_isReleaseItemNeeded = !result;

        return result;
    }

    // Helper method, just to avoid code duplication.
    BOOL ES5HeapArgumentsObject::SetEnumerableForFormal(uint32 index, PropertyId propertyId, BOOL value)
    {
        AssertMsg(this->IsFormalArgument(index), "SetAccessorsForFormal: called for non-formal");

        AutoObjectArrayItemExistsValidator autoItemAddRelease(this, index);

        BOOL result = this->DynamicObject::SetEnumerable(propertyId, value);
        autoItemAddRelease.m_isReleaseItemNeeded = !result;

        return result;
    }

    // Helper method, just to avoid code duplication.
    BOOL ES5HeapArgumentsObject::SetWritableForFormal(uint32 index, PropertyId propertyId, BOOL value)
    {
        AssertMsg(this->IsFormalArgument(index), "SetAccessorsForFormal: called for non-formal");

        AutoObjectArrayItemExistsValidator autoItemAddRelease(this, index);

        BOOL isDisconnected = IsFormalDisconnectedFromNamedArgument(index);
        if (!isDisconnected && !value)
        {
            // Settings writable to false causes disconnect.
            // It will be too late to copy the value after setting writable = false, as we would not be able to.
            // Since we are connected, it does not matter the value is, so it's safe (no matter if SetWritable fails) to copy it here.
            this->SetObjectArrayItem(index, this->frameObject->GetSlot(index), PropertyOperation_None);
        }

        BOOL result = this->DynamicObject::SetWritable(propertyId, value); // Note: this will convert objectArray to ES5Array.
        if (result && !value && !isDisconnected)
        {
            this->DisconnectFormalFromNamedArgument(index);
        }
        autoItemAddRelease.m_isReleaseItemNeeded = !result;

        return result;
    }

    // Helper method for SetPropertyWithAttributes, just to avoid code duplication.
    BOOL ES5HeapArgumentsObject::SetAccessorsForFormal(uint32 index, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        AssertMsg(this->IsFormalArgument(index), "SetAccessorsForFormal: called for non-formal");

        AutoObjectArrayItemExistsValidator autoItemAddRelease(this, index);

        BOOL result = this->DynamicObject::SetAccessors(propertyId, getter, setter, flags);
        if (result)
        {
            this->DisconnectFormalFromNamedArgument(index);
        }
        autoItemAddRelease.m_isReleaseItemNeeded = !result;

        return result;
    }

    // Helper method for SetPropertyWithAttributes, just to avoid code duplication.
    BOOL ES5HeapArgumentsObject::SetPropertyWithAttributesForFormal(uint32 index, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        AssertMsg(this->IsFormalArgument(propertyId), "SetPropertyWithAttributesForFormal: called for non-formal");

        BOOL result = this->DynamicObject::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects);
        if (result)
        {
            if ((attributes & PropertyWritable) == 0)
            {
                // Setting writable to false should cause disconnect.
                this->DisconnectFormalFromNamedArgument(index);
            }

            if (!this->IsFormalDisconnectedFromNamedArgument(index))
            {
                // Update the (connected with named param) value, as above call could cause change of the value.
                BOOL tempResult = this->SetItemAt(index, value);  // Update the value in frameObject.
                AssertMsg(tempResult, "this->SetItem(index, value)");
            }
        }

        return result;
    }

    //---------------------- ES5HeapArgumentsObject::AutoObjectArrayItemExistsValidator -------------------------------
    ES5HeapArgumentsObject::AutoObjectArrayItemExistsValidator::AutoObjectArrayItemExistsValidator(ES5HeapArgumentsObject* args, uint32 index)
        : m_args(args), m_index(index), m_isReleaseItemNeeded(false)
    {
        AssertMsg(args, "args");
        AssertMsg(args->IsFormalArgument(index), "AutoObjectArrayItemExistsValidator: called for non-formal");

        if (!args->HasObjectArrayItem(index))
        {
            m_isReleaseItemNeeded = args->SetObjectArrayItem(index, args->frameObject->GetSlot(index), PropertyOperation_None) != FALSE;
        }
    }

    ES5HeapArgumentsObject::AutoObjectArrayItemExistsValidator::~AutoObjectArrayItemExistsValidator()
    {
        if (m_isReleaseItemNeeded)
        {
           m_args->DeleteObjectArrayItem(m_index, PropertyOperation_None);
        }
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType ES5HeapArgumentsObject::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapES5HeapArgumentsObject;
    }

    void ES5HeapArgumentsObject::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        this->ExtractSnapObjectDataInto_Helper<TTD::NSSnapObjects::SnapObjectType::SnapES5HeapArgumentsObject>(objData, alloc);
    }
#endif
}

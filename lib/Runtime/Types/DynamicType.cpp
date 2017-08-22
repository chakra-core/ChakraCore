//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    DEFINE_RECYCLER_TRACKER_PERF_COUNTER(DynamicType);
    DEFINE_RECYCLER_TRACKER_WEAKREF_PERF_COUNTER(DynamicType);

    DynamicType::DynamicType(DynamicType * type, DynamicTypeHandler *typeHandler, bool isLocked, bool isShared)
        : Type(type), typeHandler(typeHandler), isLocked(isLocked), isShared(isShared)
#if DBG
        , isCachedForChangePrototype(false)
#endif
    {
        Assert(!this->isLocked || this->typeHandler->GetIsLocked());
        Assert(!this->isShared || this->typeHandler->GetIsShared());
    }


    DynamicType::DynamicType(ScriptContext* scriptContext, TypeId typeId, RecyclableObject* prototype, JavascriptMethod entryPoint, DynamicTypeHandler * typeHandler, bool isLocked, bool isShared)
        : Type(scriptContext, typeId, prototype, entryPoint) , typeHandler(typeHandler), isLocked(isLocked), isShared(isShared), hasNoEnumerableProperties(false)
#if DBG
        , isCachedForChangePrototype(false)
#endif
    {
        Assert(typeHandler != nullptr);
        Assert(!this->isLocked || this->typeHandler->GetIsLocked());
        Assert(!this->isShared || this->typeHandler->GetIsShared());
    }

    DynamicType *
    DynamicType::New(ScriptContext* scriptContext, TypeId typeId, RecyclableObject* prototype, JavascriptMethod entryPoint, DynamicTypeHandler * typeHandler, bool isLocked, bool isShared)
    {
        return RecyclerNew(scriptContext->GetRecycler(), DynamicType, scriptContext, typeId, prototype, entryPoint, typeHandler, isLocked, isShared);
    }

    bool
    DynamicType::Is(TypeId typeId)
    {
        return !StaticType::Is(typeId);
    }

    /*
    The LockTypeOnly method does the following:
    1. Locks the type.
    2. Does NOT lock the type handler.

    Use this method sparingly for cases where the intention is to ONLY lock the type and NOT lock the type handler. When in doubt use the LockType() method instead of LockTypeOnly.

    Currently, whenever we lock the type the intention is to lock BOTH the type and the type handler. However, almost always when we check whether the type is locked we intend to check whether
    the type handler is locked. The only exception is when we add a type to the EquivalentTypeCache.

    When we add a type to the EquivalentTypeCache the intention is to lock the Type to ensure that when a cached type evolves the cache gets invalidated. For shared types, this also means we
    need to create a new type handler when the type evolves. However, for non-shared types, we can still reuse the type handler. This provides a significant performance benefit for cases
    where objects with large number of properties are created once during start up (e.g. page load) and used later, by avoiding the cost of re-creating the type hanlder and copying a large
    number of properties over to the new type handler. To get this benefit, we only lock the type while adding it to the equivalent type cache. If the type becomes shared; after it was added
    to the equivalent type cache; its type handler will then get locked as part of ShareType() and any further evolution of the type will result in creating a new type handler.
    */
    bool DynamicType::LockTypeOnly()
    {
        if (GetIsLocked())
        {
            Assert(this->GetTypeHandler()->IsLockable());
            return true;
        }
        if (this->GetTypeHandler()->IsLockable())
        {
            this->isLocked = true;
            return true;
        }
        return false;
    }

    /*
    The LockType method does the following:
    1. Locks the type.
    2. Also, locks the type handler.

    Currently, whenever we lock the type the intention is to lock BOTH the type and the type handler. However, almost always when we check whether the type is locked we intend to check whether
    the type handler is locked. The only exception is when we add a type to the EquivalentTypeCache. See LockTypeOnly() method for details on the EquivalentTypeCache case.

    Currently we lock Both the Type and the Type Handler in the following cases:
    1. When a type is Shared.
    2. Snapshot Enumeration: DynamicType::PrepareForTypeSnapshotEnumeration()
    To support snapshot enumeration we need to remember the type handler at the start of the enumeration in case the type evolves during enumeration. To achieve this we lock BOTH the
    type and the type handler in preparation for the enumeration.
    3. While setting prototype: PathTypeHandlerBase::SetPrototype()
    While setting prototype if we create a new type and put the old type to promoted type mapping in the TypeOfPrototypeObjectDictionary property we lock and share the new type.
    4. PreventExtensions, Seal and Freeze:
    Each of the operations PreventExtension, Seal and Freeze change the behavior of the type in ways that needs the type handler to be locked. The PreventExtensions operation prevents
    adding any new properties to the type so any future attempts to add new property to its type handler needs the type handler to be converted. For a shared type, this will happen by design,
    but if we end up creating a new non-shared type handler during PreventExtensions or Seal or Freeze then we explicitly need to lock the type handler to indicate that any future mutation
    should convert this newly created type handler. Seal and Freeze operations also need locking the type handler for similar reasons. In addition, Freeze also needs to invalidate the inline
    caches for the type, so it also needs to change the type during this operation.
    */
    bool DynamicType::LockType()
    {
        if (GetIsLocked() && this->GetTypeHandler()->GetIsLocked())
        {
            Assert(this->GetTypeHandler()->IsLockable());
            return true;
        }
        if (this->GetTypeHandler()->IsLockable())
        {
            this->GetTypeHandler()->LockTypeHandler();
            this->isLocked = true;
            return true;
        }
        return false;
    }

    /*
    The ShareType() method does the following:
    1. Marks the Type as Locked
    2. Marks the Type Handler as Locked
    3. Marks the Type as Shared
    4. Marks the Type Handler as Shared

    Currently, we share type in the following cases:
    1. While promoting a type: PathTypeHandlerBase::PromoteType
    If we find a successor for the PropertyRecord while promoting a type the successor's type should now be marked as Shared as a new object has reached the type.
    2. While setting prototype: PathTypeHandlerBase::SetPrototype()
    While setting prototype if we create a new type and put the old type to promoted type mapping in the TypeOfPrototypeObjectDictionary property we lock and share the new type.
    3. While creating new object literals: JavascriptOperators::EnsureObjectLiteralType
    When a new object literal is created with a type from FunctionBody::GetObjectLiteralTypeRef() already allocated by the ByteCodeWriter in AllocateObjectLiteralTypeArray() its marked
    as Shared.
    4. While updating the constructor function cache: JavascriptOperators::UpdateNewScObjectCache
    When we cache constructor return values for a type that has a sharable type handler we share the type in cases where we can do the "this assignment optimization".
    5. While deoptimizing object header inlining: DynamicObject::DeoptimizeObjectHeaderInlining
    While deoptimizing object header inlining the we create a new type and mark it as shared.
    6. While deleting last property on an object: PathTypeHandlerBase::DeleteLastProperty
    While deleting last property on an object if we end up moving auxSlots to object header (inline, reoptimize) the object's type get changed to the predecessor type. As the
    predecessor's type is now shared we mark it as shared.
    7. PreventExtensions, Seal and Freeze:
    Each of the operations PreventExtension, Seal and Freeze change the behavior of the type in ways that needs the type handler to be locked. The PreventExtensions operation prevents
    adding any new properties to the type so any future attempts to add new property to its type handler needs the type handler to be converted. For a shared type, this will happen by
    design, but if we end up creating a new non-shared type handler during PreventExtensions or Seal or Freeze then we explicitly need to lock the type handler to indicate that any
    future mutation should convert this newly created type handler. Seal and Freeze operations also need locking the type handler for similar reasons. In addition, Freeze also needs
    to invalidate the inline caches for the type, so it also needs to change the type during this operation.
    8. Module Namespace type: JavascriptLibrary::InitializeTypes
    ES6 module namespace type is shared in JavascriptLibrary::InitializeTypes.
    */
    bool DynamicType::ShareType()
    {
        if (this->GetIsShared())
        {
            Assert(this->GetTypeHandler()->IsSharable());
            return true;
        }
        if (this->GetTypeHandler()->IsSharable())
        {
            LockType();
            this->GetTypeHandler()->ShareTypeHandler(this->GetScriptContext());
            this->isShared = true;
            return true;
        }
        return false;
    }

    bool
    DynamicType::SetHasNoEnumerableProperties(bool value)
    {
        if (!value)
        {
            this->hasNoEnumerableProperties = value;
            return false;
        }

#if DEBUG
        PropertyIndex propertyIndex = (PropertyIndex)-1;
        JavascriptString* propertyString = nullptr;
        PropertyId propertyId = Constants::NoProperty;
        PropertyValueInfo info;
        Assert(!this->GetTypeHandler()->FindNextProperty(this->GetScriptContext(), propertyIndex, &propertyString, &propertyId, nullptr, this, this, EnumeratorFlags::None, nullptr, &info));
#endif

        this->hasNoEnumerableProperties = true;
        return true;
    }

    bool DynamicType::PrepareForTypeSnapshotEnumeration()
    {
        if (CONFIG_FLAG(TypeSnapshotEnumeration))
        {
            // Lock the type and handler, enabling us to enumerate properties of the type snapshotted
            // at the beginning of enumeration, despite property changes made by script during enumeration.
            return LockType(); // Note: this only works for type handlers that support locking.
        }
        return false;
    }

    void DynamicObject::InitSlots(DynamicObject* instance)
    {
        InitSlots(instance, GetScriptContext());
    }

    void DynamicObject::InitSlots(DynamicObject * instance, ScriptContext * scriptContext)
    {
        Recycler * recycler = scriptContext->GetRecycler();
        int slotCapacity = GetTypeHandler()->GetSlotCapacity();
        int inlineSlotCapacity = GetTypeHandler()->GetInlineSlotCapacity();
        if (slotCapacity > inlineSlotCapacity)
        {
            instance->auxSlots = RecyclerNewArrayZ(recycler, Field(Var), slotCapacity - inlineSlotCapacity);
        }
    }

    int DynamicObject::GetPropertyCount()
    {
        if (!this->GetTypeHandler()->EnsureObjectReady(this))
        {
            return 0;
        }
        return GetTypeHandler()->GetPropertyCount();
    }

    PropertyId DynamicObject::GetPropertyId(PropertyIndex index)
    {
        return GetTypeHandler()->GetPropertyId(this->GetScriptContext(), index);
    }

    PropertyId DynamicObject::GetPropertyId(BigPropertyIndex index)
    {
        return GetTypeHandler()->GetPropertyId(this->GetScriptContext(), index);
    }

    PropertyIndex DynamicObject::GetPropertyIndex(PropertyId propertyId)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        Assert(propertyId != Constants::NoProperty);
        return GetTypeHandler()->GetPropertyIndex(this->GetScriptContext()->GetPropertyName(propertyId));
    }

    PropertyQueryFlags DynamicObject::HasPropertyQuery(PropertyId propertyId)
    {
        // HasProperty can be invoked with propertyId = NoProperty in some cases, namely cross-thread and DOM
        // This is done to force creation of a type handler in case the type handler is deferred
        Assert(!Js::IsInternalPropertyId(propertyId) || propertyId == Js::Constants::NoProperty);
        return JavascriptConversion::BooleanToPropertyQueryFlags(GetTypeHandler()->HasProperty(this, propertyId));
    }

    // HasOwnProperty and HasProperty is the same for most objects except globalobject (moduleroot as well in legacy)
    // Note that in GlobalObject, HasProperty and HasRootProperty is not quite the same as it's handling let/const global etc.
    BOOL DynamicObject::HasOwnProperty(PropertyId propertyId)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return HasProperty(propertyId);
    }

    PropertyQueryFlags DynamicObject::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return JavascriptConversion::BooleanToPropertyQueryFlags(GetTypeHandler()->GetProperty(this, originalInstance, propertyId, value, info, requestContext));
    }

    PropertyQueryFlags DynamicObject::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord* before calling GetProperty");

        return JavascriptConversion::BooleanToPropertyQueryFlags(GetTypeHandler()->GetProperty(this, originalInstance, propertyNameString, value, info, requestContext));
    }

    BOOL DynamicObject::GetInternalProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        Assert(Js::IsInternalPropertyId(propertyId));
        return GetTypeHandler()->GetProperty(this, originalInstance, propertyId, value, nullptr, requestContext);
    }

    PropertyQueryFlags DynamicObject::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return JavascriptConversion::BooleanToPropertyQueryFlags(GetTypeHandler()->GetProperty(this, originalInstance, propertyId, value, info, requestContext));
    }

    BOOL DynamicObject::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return GetTypeHandler()->SetProperty(this, propertyId, value, flags, info);
    }

    BOOL DynamicObject::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord* before calling SetProperty");

        return GetTypeHandler()->SetProperty(this, propertyNameString, value, flags, info);
    }

    BOOL DynamicObject::SetInternalProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        Assert(Js::IsInternalPropertyId(propertyId));
        return GetTypeHandler()->SetProperty(this, propertyId, value, flags, nullptr);
    }

    DescriptorFlags DynamicObject::GetSetter(PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return GetTypeHandler()->GetSetter(this, propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags DynamicObject::GetSetter(JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord* before calling GetSetter");

        return GetTypeHandler()->GetSetter(this, propertyNameString, setterValue, info, requestContext);
    }

    BOOL DynamicObject::InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return GetTypeHandler()->InitProperty(this, propertyId, value, flags, info);
    }

    BOOL DynamicObject::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return GetTypeHandler()->DeleteProperty(this, propertyId, flags);
    }

    BOOL DynamicObject::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags)
    {
        return GetTypeHandler()->DeleteProperty(this, propertyNameString, flags);
    }

#if ENABLE_FIXED_FIELDS
    BOOL DynamicObject::IsFixedProperty(PropertyId propertyId)
    {
        Assert(!Js::IsInternalPropertyId(propertyId));
        return GetTypeHandler()->IsFixedProperty(this, propertyId);
    }
#endif

    PropertyQueryFlags DynamicObject::HasItemQuery(uint32 index)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(GetTypeHandler()->HasItem(this, index));
    }

    BOOL DynamicObject::HasOwnItem(uint32 index)
    {
        return HasItem(index);
    }

    PropertyQueryFlags DynamicObject::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(GetTypeHandler()->GetItem(this, originalInstance, index, value, requestContext));
    }

    PropertyQueryFlags DynamicObject::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(GetTypeHandler()->GetItem(this, originalInstance, index, value, requestContext));
    }

    DescriptorFlags DynamicObject::GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext)
    {
        return GetTypeHandler()->GetItemSetter(this, index, setterValue, requestContext);
    }

    BOOL DynamicObject::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        return GetTypeHandler()->SetItem(this, index, value, flags);
    }

    BOOL DynamicObject::DeleteItem(uint32 index, PropertyOperationFlags flags)
    {
        return GetTypeHandler()->DeleteItem(this, index, flags);
    }

    BOOL DynamicObject::ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext)
    {
        if(hint == JavascriptHint::HintString)
        {
            return ToPrimitiveImpl<PropertyIds::toString>(result, requestContext)
                || ToPrimitiveImpl<PropertyIds::valueOf>(result, requestContext);
        }
        else
        {
            Assert(hint == JavascriptHint::None || hint == JavascriptHint::HintNumber);
            return ToPrimitiveImpl<PropertyIds::valueOf>(result, requestContext)
                || ToPrimitiveImpl<PropertyIds::toString>(result, requestContext);

        }
    }

    template <PropertyId propertyId>
    BOOL DynamicObject::ToPrimitiveImpl(Var* result, ScriptContext * requestContext)
    {
        CompileAssert(propertyId == PropertyIds::valueOf || propertyId == PropertyIds::toString);
        InlineCache * inlineCache = propertyId == PropertyIds::valueOf ? requestContext->GetValueOfInlineCache() : requestContext->GetToStringInlineCache();
        // Use per script context inline cache for valueOf and toString
        Var aValue = JavascriptOperators::PatchGetValueUsingSpecifiedInlineCache(inlineCache, this, this, propertyId, requestContext);

        // Fast path to the default valueOf/toString implementation
        if (propertyId == PropertyIds::valueOf)
        {
            if (aValue == requestContext->GetLibrary()->GetObjectValueOfFunction())
            {
                Assert(JavascriptConversion::IsCallable(aValue));
                // The default Object.prototype.valueOf will in turn just call ToObject().
                // The result is always an object if it is not undefined or null (which "this" is not)
                return false;
            }
        }
        else
        {
            if (aValue == requestContext->GetLibrary()->GetObjectToStringFunction())
            {
                Assert(JavascriptConversion::IsCallable(aValue));
                // These typeIds should never be here (they override ToPrimitive or they don't derive to DynamicObject::ToPrimitive)
                // Otherwise, they may case implicit call in ToStringHelper
                Assert(this->GetTypeId() != TypeIds_HostDispatch
                    && this->GetTypeId() != TypeIds_HostObject);
                *result = JavascriptObject::ToStringHelper(this, requestContext);
                return true;
            }
        }

        return CallToPrimitiveFunction(aValue, propertyId, result, requestContext);
    }
    BOOL DynamicObject::CallToPrimitiveFunction(Var toPrimitiveFunction, PropertyId propertyId, Var* result, ScriptContext * requestContext)
    {
        if (JavascriptConversion::IsCallable(toPrimitiveFunction))
        {
            RecyclableObject* toStringFunction = RecyclableObject::FromVar(toPrimitiveFunction);

            ThreadContext * threadContext = requestContext->GetThreadContext();
            Var aResult = threadContext->ExecuteImplicitCall(toStringFunction, ImplicitCall_ToPrimitive, [=]() -> Js::Var
            {
                // Stack object should have a pre-op bail on implicit call.  We shouldn't see them here.
                Assert(!ThreadContext::IsOnStack(this) || threadContext->HasNoSideEffect(toStringFunction));
                return CALL_FUNCTION(threadContext, toStringFunction, CallInfo(CallFlags_Value, 1), this);
            });

            if (!aResult)
            {
                // There was an implicit call and implicit calls are disabled. This would typically cause a bailout.
                Assert(threadContext->IsDisableImplicitCall());
                *result = requestContext->GetLibrary()->GetNull();
                return true;
            }

            if (JavascriptOperators::GetTypeId(aResult) <= TypeIds_LastToPrimitiveType)
            {
                *result = aResult;
                return true;
            }
        }
        return false;
    }

    BOOL DynamicObject::GetEnumeratorWithPrefix(JavascriptEnumerator * prefixEnumerator, JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext * requestContext, ForInCache * forInCache)
    {
        Js::ArrayObject * arrayObject = nullptr;
        if (this->HasObjectArray())
        {
            arrayObject = this->GetObjectArrayOrFlagsAsArray();
            Assert(arrayObject->GetPropertyCount() == 0);
        }
        return enumerator->Initialize(prefixEnumerator, arrayObject, this, flags, requestContext, forInCache);
    }

    BOOL DynamicObject::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext * requestContext, ForInCache * forInCache)
    {
        return GetEnumeratorWithPrefix(nullptr, enumerator, flags, requestContext, forInCache);
    }

    BOOL DynamicObject::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        return GetTypeHandler()->SetAccessors(this, propertyId, getter, setter, flags);
    }

    BOOL DynamicObject::GetAccessors(PropertyId propertyId, Var *getter, Var *setter, ScriptContext * requestContext)
    {
        return GetTypeHandler()->GetAccessors(this, propertyId, getter, setter);
    }

    BOOL DynamicObject::PreventExtensions()
    {
        return GetTypeHandler()->PreventExtensions(this);
    }

    BOOL DynamicObject::Seal()
    {
        return GetTypeHandler()->Seal(this);
    }

    BOOL DynamicObject::Freeze()
    {
        Type* oldType = this->GetType();
        BOOL ret = GetTypeHandler()->Freeze(this);

        // We just made all properties on this object non-writable.
        // Make sure the type is evolved so that the property string caches
        // are no longer hit.
        if (this->GetType() == oldType)
        {
            this->ChangeType();
        }

        return ret;
    }

    BOOL DynamicObject::IsSealed()
    {
        return GetTypeHandler()->IsSealed(this);
    }

    BOOL DynamicObject::IsFrozen()
    {
        return GetTypeHandler()->IsFrozen(this);
    }

    BOOL DynamicObject::IsWritable(PropertyId propertyId)
    {
        return GetTypeHandler()->IsWritable(this, propertyId);
    }

    BOOL DynamicObject::IsConfigurable(PropertyId propertyId)
    {
        return GetTypeHandler()->IsConfigurable(this, propertyId);
    }

    BOOL DynamicObject::IsEnumerable(PropertyId propertyId)
    {
        return GetTypeHandler()->IsEnumerable(this, propertyId);
    }

    BOOL DynamicObject::SetEnumerable(PropertyId propertyId, BOOL value)
    {
        return GetTypeHandler()->SetEnumerable(this, propertyId, value);
    }

    BOOL DynamicObject::SetWritable(PropertyId propertyId, BOOL value)
    {
        return GetTypeHandler()->SetWritable(this, propertyId, value);
    }

    BOOL DynamicObject::SetConfigurable(PropertyId propertyId, BOOL value)
    {
        return GetTypeHandler()->SetConfigurable(this, propertyId, value);
    }

    BOOL DynamicObject::SetAttributes(PropertyId propertyId, PropertyAttributes attributes)
    {
        return GetTypeHandler()->SetAttributes(this, propertyId, attributes);
    }

    BOOL DynamicObject::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("{...}"));
        return TRUE;
    }

    BOOL DynamicObject::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object"));
        return TRUE;
    }

    Var DynamicObject::GetTypeOfString(ScriptContext * requestContext)
    {
        return requestContext->GetLibrary()->GetObjectTypeDisplayString();
    }

    // If this object is not extensible and the property being set does not already exist,
    // if throwIfNotExtensible is
    // * true, a type error will be thrown
    // * false, FALSE will be returned (unless strict mode is enabled, in which case a type error will be thrown).
    // Either way, the property will not be set.
    //
    // throwIfNotExtensible should always be false for non-numeric properties.
    BOOL DynamicObject::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        return GetTypeHandler()->SetPropertyWithAttributes(this, propertyId, value, attributes, info, flags, possibleSideEffects);
    }

#if DBG
    bool DynamicObject::CanStorePropertyValueDirectly(PropertyId propertyId, bool allowLetConst)
    {
        return GetTypeHandler()->CanStorePropertyValueDirectly(this, propertyId, allowLetConst);
    }
#endif

    void DynamicObject::RemoveFromPrototype(ScriptContext * requestContext)
    {
        GetTypeHandler()->RemoveFromPrototype(this, requestContext);
    }

    void DynamicObject::AddToPrototype(ScriptContext * requestContext)
    {
        GetTypeHandler()->AddToPrototype(this, requestContext);
    }

    void DynamicObject::SetPrototype(RecyclableObject* newPrototype)
    {
        // Mark newPrototype it is being set as prototype
        newPrototype->SetIsPrototype();

        GetTypeHandler()->SetPrototype(this, newPrototype);
    }
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "Types/PathTypeHandler.h"

using namespace Js;

JsSetterGetterInterceptor::JsSetterGetterInterceptor() :
    getTrap(nullptr),
    setTrap(nullptr),
    deletePropertyTrap(nullptr),
    enumerateTrap(nullptr),
    ownKeysTrap(nullptr),
    hasTrap(nullptr),
    getOwnPropertyDescriptorTrap(nullptr),
    definePropertyTrap(nullptr)
{
}

JsSetterGetterInterceptor::JsSetterGetterInterceptor(
    Js::JsSetterGetterInterceptor * setterGetterInterceptor) :
    getTrap(setterGetterInterceptor->getTrap),
    setTrap(setterGetterInterceptor->setTrap),
    deletePropertyTrap(setterGetterInterceptor->deletePropertyTrap),
    enumerateTrap(setterGetterInterceptor->enumerateTrap),
    ownKeysTrap(setterGetterInterceptor->ownKeysTrap),
    hasTrap(setterGetterInterceptor->hasTrap),
    getOwnPropertyDescriptorTrap(setterGetterInterceptor->getOwnPropertyDescriptorTrap),
    definePropertyTrap(setterGetterInterceptor->definePropertyTrap)
{
}

bool JsSetterGetterInterceptor::AreInterceptorsRequired()
{
    return (this->getTrap != nullptr ||
        this->setTrap != nullptr ||
        this->deletePropertyTrap != nullptr ||
        this->enumerateTrap != nullptr ||
        this->ownKeysTrap != nullptr ||
        this->hasTrap != nullptr ||
        this->getOwnPropertyDescriptorTrap != nullptr ||
        this->definePropertyTrap != nullptr);
    /*
    CHAKRA: functionCallDelegate is intentionaly not added as interceptors because it can be invoked
    through Object::CallAsFunction or Object::CallAsConstructor
    */
}

CustomExternalWrapperType::CustomExternalWrapperType(Js::ScriptContext* scriptContext, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, Js::RecyclableObject * prototype)
    : Js::DynamicType(
        scriptContext,
        Js::TypeIds_Object,
        prototype,
        nullptr,
        Js::PathTypeHandlerNoAttr::New(scriptContext, scriptContext->GetLibrary()->GetRootPath(), 0, 0, 0, true, true),
        true,
        true)
    , jsTraceCallback(traceCallback)
    , jsFinalizeCallback(finalizeCallback)
{
    this->jsSetterGetterInterceptor = RecyclerNew(scriptContext->GetRecycler(), JsSetterGetterInterceptor);
    this->flags |= TypeFlagMask_JsrtExternal;
}

CustomExternalWrapperObject::CustomExternalWrapperObject(CustomExternalWrapperType * type, void *data, uint inlineSlotSize) :
    Js::DynamicObject(type, false/* initSlots*/)
{
    if (inlineSlotSize != 0)
    {
        this->slotType = SlotType::Inline;
        this->u.inlineSlotSize = inlineSlotSize;
        if (data)
        {
            memcpy_s(this->GetInlineSlots(), inlineSlotSize, data, inlineSlotSize);
        }
    }
    else
    {
        this->slotType = SlotType::External;
        this->u.slot = data;
    }
}

/* static */
CustomExternalWrapperObject* CustomExternalWrapperObject::Create(void *data, uint inlineSlotSize, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, void ** setterGetterInterceptor, Js::RecyclableObject * prototype, Js::ScriptContext *scriptContext)
{
    if (prototype == nullptr)
    {
        prototype = scriptContext->GetLibrary()->GetObjectPrototype();
    }

    Js::DynamicType * dynamicType = scriptContext->GetLibrary()->GetCachedCustomExternalWrapperType(reinterpret_cast<uintptr_t>(traceCallback), reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(*setterGetterInterceptor), reinterpret_cast<uintptr_t>(prototype));

    if (dynamicType == nullptr)
    {
        dynamicType = RecyclerNew(scriptContext->GetRecycler(), CustomExternalWrapperType, scriptContext, traceCallback, finalizeCallback, prototype);
        *setterGetterInterceptor = reinterpret_cast<CustomExternalWrapperType *>(dynamicType)->GetJsSetterGetterInterceptor();
        scriptContext->GetLibrary()->CacheCustomExternalWrapperType(reinterpret_cast<uintptr_t>(traceCallback), reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(*setterGetterInterceptor), reinterpret_cast<uintptr_t>(prototype), dynamicType);
    }
    else
    {
        if (*setterGetterInterceptor == nullptr)
        {
            *setterGetterInterceptor = reinterpret_cast<CustomExternalWrapperType *>(dynamicType)->GetJsSetterGetterInterceptor();
        }
        else
        {
            Assert(*setterGetterInterceptor == reinterpret_cast<CustomExternalWrapperType *>(dynamicType)->GetJsSetterGetterInterceptor());
        }
    }

    Assert(dynamicType->IsJsrtExternal());

    CustomExternalWrapperObject * externalObject;
    if (traceCallback != nullptr)
    {
        externalObject = RecyclerNewTrackedPlus(scriptContext->GetRecycler(), inlineSlotSize, CustomExternalWrapperObject, static_cast<CustomExternalWrapperType*>(dynamicType), data, inlineSlotSize);
    }
    else if (finalizeCallback != nullptr)
    {
        externalObject = RecyclerNewFinalizedPlus(scriptContext->GetRecycler(), inlineSlotSize, CustomExternalWrapperObject, static_cast<CustomExternalWrapperType*>(dynamicType), data, inlineSlotSize);
    }
    else
    {
        externalObject = RecyclerNewPlus(scriptContext->GetRecycler(), inlineSlotSize, CustomExternalWrapperObject, static_cast<CustomExternalWrapperType*>(dynamicType), data, inlineSlotSize);
    }

    return externalObject;
}

BOOL CustomExternalWrapperObject::IsObjectAlive()
{
    return !this->GetScriptContext()->IsClosed();
}

BOOL CustomExternalWrapperObject::VerifyObjectAlive()
{
    Js::ScriptContext* scriptContext = GetScriptContext();
    if (!scriptContext->VerifyAlive())
    {
        return FALSE;
    }

    // Perform an extended host object invalidation check only for external host objects.
    if (scriptContext->IsInvalidatedForHostObjects())
    {
        if (!scriptContext->GetThreadContext()->RecordImplicitException())
            return FALSE;
        Js::JavascriptError::MapAndThrowError(scriptContext, E_ACCESSDENIED);
    }
    return TRUE;
}

BOOL CustomExternalWrapperObject::Is(_In_ Js::Var value)
{
    if (Js::TaggedNumber::Is(value))
    {
        return false;
    }

    return (VirtualTableInfo<CustomExternalWrapperObject>::HasVirtualTable(value)) ||
        (VirtualTableInfo<Js::CrossSiteObject<CustomExternalWrapperObject>>::HasVirtualTable(value));
}

BOOL CustomExternalWrapperObject::Is(_In_ RecyclableObject * value)
{
    if (Js::TaggedNumber::Is(value))
    {
        return false;
    }

    return (VirtualTableInfo<CustomExternalWrapperObject>::HasVirtualTable(value)) ||
        (VirtualTableInfo<Js::CrossSiteObject<CustomExternalWrapperObject>>::HasVirtualTable(value));
}

BOOL CustomExternalWrapperObject::Equals(__in Var other, __out BOOL* value, ScriptContext* requestContext)
{
    // Reject implicit call
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        *value = FALSE;
        return FALSE;
    }

    // We need to implement comparison to other by reference in case the object
    // is in the left side of the comparison, and does not call a toString
    // method (like different objects) when compared to string. For Node, such
    // comparision is used for Buffers.
    *value = (other == this);
    return true;
}

BOOL CustomExternalWrapperObject::StrictEquals(__in Var other, __out BOOL* value, ScriptContext* requestContext)
{
    *value = FALSE;
    // Reject implicit call
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    // We need to implement comparison to other by reference in case the object
    // is in the left side of the comparison, and does not call a toString
    // method (like different objects) when compared to string. For Node, such
    // comparision is used for Buffers.
    *value = (other == this);
    return true;
}

CustomExternalWrapperObject * CustomExternalWrapperObject::FromVar(Js::Var value)
{
    AssertOrFailFast(Is(value));
    return static_cast<CustomExternalWrapperObject *>(value);
}

CustomExternalWrapperObject * CustomExternalWrapperObject::UnsafeFromVar(Js::Var value)
{
    Assert(Is(value));
    return static_cast<CustomExternalWrapperObject *>(value);
}

void CustomExternalWrapperObject::Mark(Recycler * recycler)
{
    JsTraceCallback traceCallback = this->GetExternalType()->GetJsTraceCallback();
    Assert(nullptr != traceCallback);
    traceCallback(this->GetSlotData());
}

void CustomExternalWrapperObject::Finalize(bool isShutdown)
{
    JsFinalizeCallback finalizeCallback = this->GetExternalType()->GetJsFinalizeCallback();
    Assert(this->GetExternalType()->GetJsTraceCallback() != nullptr || finalizeCallback != nullptr);
    if (nullptr != finalizeCallback)
    {
        finalizeCallback(this->GetSlotData());
    }
}

void CustomExternalWrapperObject::Dispose(bool isShutdown)
{
}

void * CustomExternalWrapperObject::GetSlotData() const
{
    return this->slotType == SlotType::External
        ? this->u.slot
        : GetInlineSlots();
}

void CustomExternalWrapperObject::SetSlotData(void * data)
{
    this->slotType = SlotType::External;
    this->u.slot = data;
}

int CustomExternalWrapperObject::GetInlineSlotSize() const
{
    return this->slotType == SlotType::External
        ? 0
        : this->u.inlineSlotSize;
}

void* CustomExternalWrapperObject::GetInlineSlots() const
{
    return this->slotType == SlotType::External
        ? nullptr
        : (void*)((uintptr_t)this + sizeof(CustomExternalWrapperObject));
}

Js::DynamicType* CustomExternalWrapperObject::DuplicateType()
{
    return RecyclerNew(this->GetScriptContext()->GetRecycler(), CustomExternalWrapperType, this->GetExternalType());
}

void CustomExternalWrapperObject::SetPrototype(RecyclableObject* newPrototype)
{
    if (!this->VerifyObjectAlive())
    {
        return;
    }

    DynamicObject::SetPrototype(newPrototype);
}

Js::Var CustomExternalWrapperObject::GetValueFromDescriptor(Js::Var instance, Js::PropertyDescriptor propertyDescriptor, Js::ScriptContext * requestContext)
{
    if (propertyDescriptor.ValueSpecified())
    {
        return Js::CrossSite::MarshalVar(requestContext, propertyDescriptor.GetValue());
    }
    if (propertyDescriptor.GetterSpecified())
    {
        return Js::JavascriptOperators::CallGetter(RecyclableObject::FromVar(propertyDescriptor.GetGetter()), instance, requestContext);
    }
    Assert(FALSE);
    return requestContext->GetLibrary()->GetUndefined();
}

void CustomExternalWrapperObject::PropertyIdFromInt(uint32 index, Js::PropertyRecord const** propertyRecord)
{
    char16 buffer[22];
    int pos = Js::TaggedInt::ToBuffer(index, buffer, _countof(buffer));

    GetScriptContext()->GetOrAddPropertyRecord((LPCWSTR)buffer + pos, (_countof(buffer) - 1) - pos, propertyRecord);
}

Js::Var CustomExternalWrapperObject::GetName(Js::ScriptContext* requestContext, Js::PropertyId propertyId)
{
    const Js::PropertyRecord* propertyRecord = requestContext->GetThreadContext()->GetPropertyName(propertyId);
    Js::Var name;
    if (propertyRecord->IsSymbol())
    {
        name = requestContext->GetSymbol(propertyRecord);
    }
    else
    {
        name = requestContext->GetPropertyString(propertyRecord);
    }
    return name;
}

template <class Fn, class GetPropertyIdFunc>
BOOL CustomExternalWrapperObject::GetPropertyTrap(Js::Var instance, Js::PropertyDescriptor * propertyDescriptor, Fn fn, GetPropertyIdFunc getPropertyId, Js::ScriptContext * requestContext)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    // Reject implicit call
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    Js::RecyclableObject * targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* getGetMethod = nullptr;
    if (type->GetJsSetterGetterInterceptor()->getTrap != nullptr)
    {
        getGetMethod = Js::JavascriptFunction::FromVar(type->GetJsSetterGetterInterceptor()->getTrap);
    }

    if (nullptr == getGetMethod || requestContext->IsHeapEnumInProgress())
    {
        return fn(targetObj);
    }

    Js::PropertyId propertyId = getPropertyId();
    Js::Var propertyName = GetName(requestContext, propertyId);

    Js::Var getGetResult = threadContext->ExecuteImplicitCall(getGetMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, getGetMethod, Js::CallInfo(Js::CallFlags_Value, 3), targetObj, propertyName, instance);
    });

    Js::TypeId getResultTypeId = Js::JavascriptOperators::GetTypeId(getGetResult);
    if (getResultTypeId == Js::TypeIds_Undefined)
    {
        return FALSE;
    }

    propertyDescriptor->SetValue(getGetResult);
    return TRUE;
}

template <class Fn, class GetPropertyIdFunc>
BOOL CustomExternalWrapperObject::HasPropertyTrap(Fn fn, GetPropertyIdFunc getPropertyId)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    // Reject implicit call
    ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    // Caller does not pass requestContext. Retrieve from host scriptContext stack.
    Js::ScriptContext* requestContext =
        threadContext->GetPreviousHostScriptContext()->GetScriptContext();

    Js::RecyclableObject *targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* hasMethod = nullptr;
    if (type->GetJsSetterGetterInterceptor()->hasTrap != nullptr)
    {
        hasMethod = Js::JavascriptFunction::FromVar(type->GetJsSetterGetterInterceptor()->hasTrap);
    }

    if (nullptr == hasMethod || requestContext->IsHeapEnumInProgress())
    {
        return fn(targetObj);
    }

    Js::PropertyId propertyId = getPropertyId();
    Js::Var propertyName = GetName(requestContext, propertyId);

    Js::Var getHasResult = threadContext->ExecuteImplicitCall(hasMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, hasMethod, Js::CallInfo(Js::CallFlags_Value, 2), targetObj, propertyName);
    });

    BOOL hasProperty = Js::JavascriptConversion::ToBoolean(getHasResult, requestContext);
    return hasProperty;
}

Js::PropertyQueryFlags CustomExternalWrapperObject::HasPropertyQuery(Js::PropertyId propertyId, _Inout_opt_ Js::PropertyValueInfo* info)
{
    auto fn = [&](RecyclableObject* object)->BOOL {
        return Js::JavascriptOperators::HasProperty(object, propertyId);
    };
    auto getPropertyId = [&]() -> Js::PropertyId {
        return propertyId;
    };
    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(HasPropertyTrap(fn, getPropertyId));
}

BOOL CustomExternalWrapperObject::GetPropertyDescriptorTrap(Js::PropertyId propertyId, Js::PropertyDescriptor* resultDescriptor, Js::ScriptContext* requestContext)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    // Reject implicit call
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    Js::RecyclableObject * targetObj = this;

    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* gOPDMethod = nullptr;
    if (type->GetJsSetterGetterInterceptor()->getOwnPropertyDescriptorTrap != nullptr)
    {
        gOPDMethod = Js::JavascriptFunction::FromVar(type->GetJsSetterGetterInterceptor()->getOwnPropertyDescriptorTrap);
    }

    if (GetScriptContext()->IsHeapEnumInProgress())
    {
        return FALSE;
    }

    if (nullptr == gOPDMethod)
    {
        Var getter, setter;
        if (false == JavascriptOperators::GetOwnAccessors(this, propertyId, &getter, &setter, requestContext))
        {
            Var value = nullptr;
            if (false == JavascriptOperators::GetOwnProperty(this, propertyId, &value, requestContext, nullptr))
            {
                return FALSE;
            }
            if (nullptr != value)
            {
                resultDescriptor->SetValue(value);
            }

            resultDescriptor->SetWritable(FALSE != this->IsWritable(propertyId));
        }
        else
        {
            if (nullptr == getter)
            {
                getter = requestContext->GetLibrary()->GetUndefined();
            }
            resultDescriptor->SetGetter(getter);

            if (nullptr == setter)
            {
                setter = requestContext->GetLibrary()->GetUndefined();
            }
            resultDescriptor->SetSetter(setter);
        }

        resultDescriptor->SetConfigurable(FALSE != this->IsConfigurable(propertyId));
        resultDescriptor->SetEnumerable(FALSE != this->IsEnumerable(propertyId));
        return TRUE;
    }

    Js::Var propertyName = GetName(requestContext, propertyId);

    Assert(Js::JavascriptString::Is(propertyName) || Js::JavascriptSymbol::Is(propertyName));

    Js::Var getResult = threadContext->ExecuteImplicitCall(gOPDMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, gOPDMethod, Js::CallInfo(Js::CallFlags_Value, 2), targetObj, propertyName);
    });

    Js::TypeId getResultTypeId = Js::JavascriptOperators::GetTypeId(getResult);
    if (Js::StaticType::Is(getResultTypeId) && getResultTypeId != Js::TypeIds_Undefined)
    {
        Js::JavascriptError::ThrowTypeError(requestContext, JSERR_NeedObject, _u("getOwnPropertyDescriptor"));
    }

    BOOL toProperty = Js::JavascriptOperators::ToPropertyDescriptor(getResult, resultDescriptor, requestContext);
    Js::JavascriptOperators::CompletePropertyDescriptor(resultDescriptor, nullptr, requestContext);
    return toProperty;
}

BOOL CustomExternalWrapperObject::GetOwnPropertyDescriptor(Js::RecyclableObject * obj, Js::PropertyId propertyId, Js::ScriptContext * requestContext, Js::PropertyDescriptor * propertyDescriptor)
{
    CustomExternalWrapperObject * customObject = CustomExternalWrapperObject::FromVar(obj);
    return customObject->GetPropertyDescriptorTrap(propertyId, propertyDescriptor, requestContext);
}

BOOL CustomExternalWrapperObject::DefineOwnPropertyDescriptor(Js::RecyclableObject * obj, Js::PropertyId propId, const Js::PropertyDescriptor& descriptor, bool throwOnError, Js::ScriptContext * requestContext)
{
    PROBE_STACK(requestContext, Js::Constants::MinStackDefault);

    // Reject implicit call
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    CustomExternalWrapperObject * customObject = CustomExternalWrapperObject::FromVar(obj);

    Js::RecyclableObject * targetObj = obj;

    CustomExternalWrapperType * type = customObject->GetExternalType();
    Js::JavascriptFunction* defineOwnPropertyMethod = nullptr;
    if (type->GetJsSetterGetterInterceptor()->definePropertyTrap != nullptr)
    {
        defineOwnPropertyMethod = Js::JavascriptFunction::FromVar(type->GetJsSetterGetterInterceptor()->definePropertyTrap);
    }

    Assert(!requestContext->IsHeapEnumInProgress());
    if (nullptr == defineOwnPropertyMethod)
    {
        PropertyDescriptor currentDescriptor;
        BOOL isCurrentDescriptorDefined = JavascriptOperators::GetOwnPropertyDescriptor(obj, propId, requestContext, &currentDescriptor);

        bool isExtensible = !!obj->IsExtensible();
        return Js::JavascriptOperators::ValidateAndApplyPropertyDescriptor<true>(obj, propId, descriptor, isCurrentDescriptorDefined ? &currentDescriptor : nullptr, isExtensible, throwOnError, requestContext);
    }

    Js::Var descVar = descriptor.GetOriginal();
    if (descVar == nullptr)
    {
        descVar = Js::JavascriptOperators::FromPropertyDescriptor(descriptor, requestContext);
    }

    Js::Var propertyName = customObject->GetName(requestContext, propId);

    Js::Var definePropertyResult = threadContext->ExecuteImplicitCall(defineOwnPropertyMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, defineOwnPropertyMethod, Js::CallInfo(Js::CallFlags_Value, 3), targetObj, propertyName, descVar);
    });

    BOOL defineResult = Js::JavascriptConversion::ToBoolean(definePropertyResult, requestContext);
    if (defineResult)
    {
        return defineResult;
    }

    Js::PropertyDescriptor targetDescriptor;
    BOOL hasProperty = Js::JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propId, requestContext, &targetDescriptor);
    BOOL isExtensible = targetObj->IsExtensible();
    BOOL settingConfigFalse = (descriptor.ConfigurableSpecified() && !descriptor.IsConfigurable());
    if (!hasProperty)
    {
        if (!isExtensible || settingConfigFalse)
        {
            Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("defineProperty"));
        }
    }
    else
    {
        if (!Js::JavascriptOperators::IsCompatiblePropertyDescriptor(descriptor, hasProperty ? &targetDescriptor : nullptr, !!isExtensible, true, requestContext))
        {
            Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("defineProperty"));
        }
        if (settingConfigFalse && targetDescriptor.IsConfigurable())
        {
            Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("defineProperty"));
        }
    }
    return hasProperty;
}

BOOL CustomExternalWrapperObject::SetPropertyTrap(Js::Var receiver, SetPropertyTrapKind setPropertyTrapKind, Js::JavascriptString * propertyNameString, Js::Var newValue, Js::ScriptContext * requestContext, Js::PropertyOperationFlags propertyOperationFlags)
{
    const Js::PropertyRecord* propertyRecord;
    requestContext->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
    return SetPropertyTrap(receiver, setPropertyTrapKind, propertyRecord->GetPropertyId(), newValue, requestContext, propertyOperationFlags);
}

BOOL CustomExternalWrapperObject::SetPropertyTrap(Js::Var receiver, SetPropertyTrapKind setPropertyTrapKind, Js::PropertyId propertyId, Js::Var newValue, Js::ScriptContext * requestContext, Js::PropertyOperationFlags propertyOperationFlags, BOOL skipPrototypeCheck)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    // Reject implicit call
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    Js::RecyclableObject *targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* setMethod = nullptr;
    if (type->GetJsSetterGetterInterceptor()->setTrap != nullptr)
    {
        setMethod = Js::JavascriptFunction::FromVar(type->GetJsSetterGetterInterceptor()->setTrap);
    }

    Assert(!GetScriptContext()->IsHeapEnumInProgress());
    Js::Var propertyName = GetName(requestContext, propertyId);

    if (nullptr != setMethod)
    {
        Js::Var setPropertyResult = threadContext->ExecuteImplicitCall(setMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, setMethod, Js::CallInfo(Js::CallFlags_Value, 4), targetObj, propertyName, newValue, receiver);
        });

        BOOL setResult = Js::JavascriptConversion::ToBoolean(setPropertyResult, requestContext);
        return setResult;
    }

    return FALSE;
}

Js::PropertyQueryFlags CustomExternalWrapperObject::GetPropertyQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext)
{
    if (!this->VerifyObjectAlive()) return Js::PropertyQueryFlags::Property_NotFound;
    originalInstance = Js::CrossSite::MarshalVar(GetScriptContext(), originalInstance);

    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return Js::JavascriptOperators::GetProperty(originalInstance, object, propertyId, value, requestContext, nullptr);
    };
    auto getPropertyId = [&]()->Js::PropertyId {return propertyId; };
    Js::PropertyDescriptor result;
    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
    if (!foundProperty)
    {
        *value = requestContext->GetMissingPropertyResult();
    }
    else
    {
        *value = GetValueFromDescriptor(originalInstance, result, requestContext);
    }
    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
}

Js::PropertyQueryFlags CustomExternalWrapperObject::GetPropertyQuery(Js::Var originalInstance, Js::JavascriptString* propertyNameString, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext)
{
    if (!this->VerifyObjectAlive()) return Js::PropertyQueryFlags::Property_NotFound;
    originalInstance = Js::CrossSite::MarshalVar(GetScriptContext(), originalInstance);

    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return Js::JavascriptOperators::GetPropertyWPCache<false /* OutputExistence */>(originalInstance, object, propertyNameString, value, requestContext, info);
    };
    auto getPropertyId = [&]()->Js::PropertyId {
        const Js::PropertyRecord* propertyRecord;
        requestContext->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        return propertyRecord->GetPropertyId();
    };
    Js::PropertyDescriptor result;
    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
    if (!foundProperty)
    {
        *value = requestContext->GetMissingPropertyResult();
    }
    else
    {
        *value = GetValueFromDescriptor(originalInstance, result, requestContext);
    }
    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
}

Js::PropertyQueryFlags CustomExternalWrapperObject::GetPropertyReferenceQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext)
{
    if (!this->VerifyObjectAlive()) return Js::PropertyQueryFlags::Property_NotFound;
    originalInstance = Js::CrossSite::MarshalVar(GetScriptContext(), originalInstance);

    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return Js::JavascriptOperators::GetPropertyReference(originalInstance, object, propertyId, value, requestContext, nullptr);
    };
    auto getPropertyId = [&]() -> Js::PropertyId {return propertyId; };
    Js::PropertyDescriptor result;
    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
    if (!foundProperty)
    {
        *value = requestContext->GetMissingPropertyResult();
    }
    else
    {
        *value = GetValueFromDescriptor(originalInstance, result, requestContext);
    }
    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
}

Js::PropertyQueryFlags CustomExternalWrapperObject::HasItemQuery(uint32 index)
{
    const Js::PropertyRecord * propertyRecord;
    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return Js::JavascriptOperators::HasItem(object, index);
    };
    auto getPropertyId = [&]() -> Js::PropertyId {
        PropertyIdFromInt(index, &propertyRecord);
        return propertyRecord->GetPropertyId();
    };
    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(HasPropertyTrap(fn, getPropertyId));
}

BOOL CustomExternalWrapperObject::HasOwnItem(uint32 index)
{
    const Js::PropertyRecord* propertyRecord;
    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return Js::JavascriptOperators::HasOwnItem(object, index);
    };
    auto getPropertyId = [&]() -> Js::PropertyId {
        PropertyIdFromInt(index, &propertyRecord);
        return propertyRecord->GetPropertyId();
    };
    return HasPropertyTrap(fn, getPropertyId);
}

Js::PropertyQueryFlags CustomExternalWrapperObject::GetItemQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * requestContext)
{
    if (!this->VerifyObjectAlive()) return Js::PropertyQueryFlags::Property_NotFound;
    originalInstance = Js::CrossSite::MarshalVar(GetScriptContext(), originalInstance);

    const Js::PropertyRecord* propertyRecord;
    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return Js::JavascriptOperators::GetItem(originalInstance, object, index, value, requestContext);
    };
    auto getPropertyId = [&]() -> Js::PropertyId {
        PropertyIdFromInt(index, &propertyRecord);
        return propertyRecord->GetPropertyId();
    };

    Js::PropertyDescriptor result;
    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
    if (!foundProperty)
    {
        *value = requestContext->GetMissingItemResult();
    }
    else
    {
        *value = GetValueFromDescriptor(originalInstance, result, requestContext);
    }
    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
}

Js::PropertyQueryFlags CustomExternalWrapperObject::GetItemReferenceQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * requestContext)
{
    if (!this->VerifyObjectAlive()) return Js::PropertyQueryFlags::Property_NotFound;
    originalInstance = Js::CrossSite::MarshalVar(GetScriptContext(), originalInstance);

    const Js::PropertyRecord* propertyRecord;
    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return Js::JavascriptOperators::GetItem(originalInstance, object, index, value, requestContext);
    };
    auto getPropertyId = [&]() -> Js::PropertyId {
        PropertyIdFromInt(index, &propertyRecord);
        return propertyRecord->GetPropertyId();
    };

    Js::PropertyDescriptor result;
    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
    if (!foundProperty)
    {
        *value = requestContext->GetMissingItemResult();
    }
    else
    {
        *value = GetValueFromDescriptor(originalInstance, result, requestContext);
    }
    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
}

BOOL CustomExternalWrapperObject::SetItem(uint32 index, Js::Var value, Js::PropertyOperationFlags flags)
{
    if (!this->VerifyObjectAlive()) return FALSE;
    //value = Js::CrossSite::MarshalVar(GetScriptContext(), value);

    const PropertyRecord* propertyRecord = nullptr;
    PropertyIdFromInt(index, &propertyRecord);

    BOOL trapResult = SetPropertyTrap(this, CustomExternalWrapperObject::SetPropertyTrapKind::SetItemKind, propertyRecord->GetPropertyId(), value, GetScriptContext(), flags);
    if (!trapResult)
    {
        return Js::DynamicObject::SetItem(index, value, flags);
    }

    return TRUE;
}

BOOL CustomExternalWrapperObject::DeleteItem(uint32 index, Js::PropertyOperationFlags flags)
{
    if (!this->VerifyObjectAlive()) return FALSE;
    const PropertyRecord* propertyRecord = nullptr;
    PropertyIdFromInt(index, &propertyRecord);
    return this->DeleteProperty(propertyRecord->GetPropertyId(), flags);
}

BOOL CustomExternalWrapperObject::GetEnumerator(Js::JavascriptStaticEnumerator * enumerator, Js::EnumeratorFlags flags, Js::ScriptContext* requestContext, Js::EnumeratorCache * enumeratorCache)
{
    if (!this->VerifyObjectAlive()) return FALSE;
    // Reject implicit call
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* getEnumeratorMethod = nullptr;
    if (type->GetJsSetterGetterInterceptor()->enumerateTrap != nullptr)
    {
        getEnumeratorMethod = Js::JavascriptFunction::FromVar(type->GetJsSetterGetterInterceptor()->enumerateTrap);
    }

    Js::RecyclableObject * targetObj = this;
    JavascriptArray * trapResult = nullptr;
    if (getEnumeratorMethod == nullptr)
    {
        trapResult = JavascriptOperators::GetOwnPropertyNames(this, requestContext);
    }
    else
    {
        Js::Var getGetEnumeratorResult = threadContext->ExecuteImplicitCall(getEnumeratorMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, getEnumeratorMethod, Js::CallInfo(Js::CallFlags_Value, 1), targetObj);
        });

        Js::TypeId getResultTypeId = Js::JavascriptOperators::GetTypeId(getGetEnumeratorResult);
        if (getResultTypeId == Js::TypeIds_Undefined)
        {
            return FALSE;
        }

        trapResult = JavascriptOperators::TryFromVar<JavascriptArray>(getGetEnumeratorResult);
        if (trapResult == nullptr)
        {
            return FALSE;
        }
    }

    struct WrapperOwnKeysEnumerator : public JavascriptEnumerator
    {
        typedef JsUtil::BaseHashSet<JsUtil::CharacterBuffer<WCHAR>, Recycler> VisitedNamesHashSet;
        Field(VisitedNamesHashSet*) visited;
        Field(JavascriptArray*) trapResult;
        Field(CustomExternalWrapperObject*) wrapper;
        FieldNoBarrier(ScriptContext*) scriptContext;
        Field(uint32) index;

        DEFINE_VTABLE_CTOR_ABSTRACT(WrapperOwnKeysEnumerator, JavascriptEnumerator)

            WrapperOwnKeysEnumerator(ScriptContext* scriptContext, CustomExternalWrapperObject* wrapper, JavascriptArray* trapResult)
            :JavascriptEnumerator(scriptContext), scriptContext(scriptContext), wrapper(wrapper), trapResult(trapResult)
        {
            visited = RecyclerNew(scriptContext->GetRecycler(), VisitedNamesHashSet, scriptContext->GetRecycler());
        }
        virtual void Reset() override
        {
            index = 0;
            visited->Reset();
        }

        virtual JavascriptString * MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override
        {
            propertyId = Constants::NoProperty;
            if (attributes != nullptr)
            {
                *attributes = PropertyEnumerable;
            }
            // 13.7.5.15 EnumerateObjectProperties(O) (https://tc39.github.io/ecma262/#sec-enumerate-object-properties)
            // for (let key of Reflect.ownKeys(obj)) {
            if (trapResult != nullptr)
            {
                uint32 len = trapResult->GetLength();
                while (index < len)
                {
                    Var var = trapResult->DirectGetItem(index++);
                    if (var)
                    {
                        // if (typeof key === "string") {
                        if (JavascriptString::Is(var))
                        {
                            JavascriptString* propertyName = JavascriptString::FromVar(var);
                            // let desc = Reflect.getOwnPropertyDescriptor(obj, key);
                            Js::PropertyDescriptor desc;
                            BOOL ret = JavascriptOperators::GetOwnPropertyDescriptor(wrapper, propertyName, scriptContext, &desc);
                            const JsUtil::CharacterBuffer<WCHAR> propertyString(propertyName->GetString(), propertyName->GetLength());
                            // if (desc && !visited.has(key)) {
                            if (ret && !visited->Contains(propertyString))
                            {
                                visited->Add(propertyString);
                                // if (desc.enumerable) yield key;
                                if (desc.IsEnumerable())
                                {
                                    return JavascriptString::FromVar(CrossSite::MarshalVar(
                                        scriptContext, propertyName, propertyName->GetScriptContext()));
                                }
                            }
                        }
                    }
                }
            }
            return nullptr;
        }
    };

    WrapperOwnKeysEnumerator* ownKeysEnum = RecyclerNew(requestContext->GetRecycler(), WrapperOwnKeysEnumerator, requestContext, this, trapResult);

    return enumerator->Initialize(ownKeysEnum, nullptr, nullptr, flags, requestContext, enumeratorCache);
}

BOOL CustomExternalWrapperObject::SetProperty(Js::PropertyId propertyId, Js::Var value, Js::PropertyOperationFlags flags, Js::PropertyValueInfo* info)
{
    if (!this->VerifyObjectAlive()) return FALSE;
    //value = Js::CrossSite::MarshalVar(GetScriptContext(), value);
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    BOOL trapResult = SetPropertyTrap(this, CustomExternalWrapperObject::SetPropertyTrapKind::SetPropertyKind, propertyId, value, GetScriptContext(), flags);
    if (!trapResult)
    {
        ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
        ScriptContext* requestContext =
            threadContext->GetPreviousHostScriptContext()->GetScriptContext();

        // Set implicit call flag so we bailout and not do copy-prop on field
        Js::ImplicitCallFlags saveImplicitCallFlags = threadContext->GetImplicitCallFlags();
        threadContext->SetImplicitCallFlags((Js::ImplicitCallFlags)(saveImplicitCallFlags | ImplicitCall_Accessor));

        PropertyDescriptor wrapperPropertyDescriptor;
        if (!JavascriptOperators::GetOwnPropertyDescriptor(this, propertyId, requestContext, &wrapperPropertyDescriptor))
        {
            PropertyDescriptor resultDescriptor;
            resultDescriptor.SetConfigurable(true);
            resultDescriptor.SetWritable(true);
            resultDescriptor.SetEnumerable(true);
            resultDescriptor.SetValue(value);
            return Js::JavascriptOperators::DefineOwnPropertyDescriptor(this, propertyId, resultDescriptor, true, requestContext);
        }
        else
        {
            if (wrapperPropertyDescriptor.IsAccessorDescriptor())
            {
                return FALSE;
            }

            if (wrapperPropertyDescriptor.WritableSpecified() && !wrapperPropertyDescriptor.IsWritable())
            {
                return FALSE;
            }

            wrapperPropertyDescriptor.SetValue(value);
            wrapperPropertyDescriptor.SetOriginal(nullptr);
            return Js::JavascriptOperators::DefineOwnPropertyDescriptor(this, propertyId, wrapperPropertyDescriptor, true, requestContext);
        }
    }

    return TRUE;
}

BOOL CustomExternalWrapperObject::SetProperty(Js::JavascriptString* propertyNameString, Js::Var value, Js::PropertyOperationFlags flags, Js::PropertyValueInfo* info)
{
    const PropertyRecord* propertyRecord;
    GetScriptContext()->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
    return SetProperty(propertyRecord->GetPropertyId(), value, flags, info);
}

BOOL CustomExternalWrapperObject::SetInternalProperty(Js::PropertyId internalPropertyId, Js::Var value, Js::PropertyOperationFlags flags, Js::PropertyValueInfo* info)
{
    return this->SetProperty(internalPropertyId, value, flags, info);
}

BOOL CustomExternalWrapperObject::EnsureNoRedeclProperty(Js::PropertyId propertyId)
{
    CustomExternalWrapperObject::HasOwnPropertyCheckNoRedecl(propertyId);
    return true;
}

BOOL CustomExternalWrapperObject::DeleteProperty(Js::PropertyId propertyId, Js::PropertyOperationFlags flags)
{
    if (!this->VerifyObjectAlive()) return FALSE;

    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    // Reject implicit call
    ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    // Caller does not pass requestContext. Retrieve from host scriptContext stack.
    Js::ScriptContext* requestContext =
        threadContext->GetPreviousHostScriptContext()->GetScriptContext();

    RecyclableObject * targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* deleteMethod = nullptr;
    if (type->GetJsSetterGetterInterceptor()->deletePropertyTrap != nullptr)
    {
        deleteMethod = Js::JavascriptFunction::FromVar(type->GetJsSetterGetterInterceptor()->deletePropertyTrap);
    }

    Assert(!GetScriptContext()->IsHeapEnumInProgress());
    if (nullptr == deleteMethod)
    {
        return __super::DeleteProperty(propertyId, flags);
    }

    Js::Var propertyName = GetName(requestContext, propertyId);

    Js::Var deletePropertyResult = threadContext->ExecuteImplicitCall(deleteMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, deleteMethod, Js::CallInfo(Js::CallFlags_Value, 2), targetObj, propertyName);
    });

    BOOL trapResult = Js::JavascriptConversion::ToBoolean(deletePropertyResult, requestContext);
    if (!trapResult)
    {
        if (flags & Js::PropertyOperation_StrictMode)
        {
            Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("deleteProperty"));
        }
        return trapResult;
    }

    Js::PropertyDescriptor targetPropertyDescriptor;
    if (!Js::JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propertyId, requestContext, &targetPropertyDescriptor))
    {
        return TRUE;
    }
    if (!targetPropertyDescriptor.IsConfigurable())
    {
        Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("deleteProperty"));
    }
    return TRUE;
}

BOOL CustomExternalWrapperObject::DeleteProperty(Js::JavascriptString *propertyNameString, Js::PropertyOperationFlags flags)
{
    if (!this->VerifyObjectAlive()) return FALSE;
    Js::PropertyRecord const *propertyRecord = nullptr;
    if (Js::JavascriptOperators::ShouldTryDeleteProperty(this, propertyNameString, &propertyRecord))
    {
        Assert(propertyRecord);
        return DeleteProperty(propertyRecord->GetPropertyId(), flags);
    }

    return TRUE;
}

Js::JavascriptArray * CustomExternalWrapperObject::PropertyKeysTrap(KeysTrapKind keysTrapKind, Js::ScriptContext * requestContext)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    // Reject implicit call
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return nullptr;
    }

    Js::RecyclableObject * targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* ownKeysMethod = nullptr;
    if (type->GetJsSetterGetterInterceptor()->ownKeysTrap != nullptr)
    {
        ownKeysMethod = Js::JavascriptFunction::FromVar(type->GetJsSetterGetterInterceptor()->ownKeysTrap);
    }

    Assert(!GetScriptContext()->IsHeapEnumInProgress());

    //TODO:akatti: Do we need to do anything more if the ownKeysTrap isn't defined?
    if (nullptr == ownKeysMethod)
    {
        return nullptr;
    }

    Js::Var ownKeysResult = threadContext->ExecuteImplicitCall(ownKeysMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, ownKeysMethod, Js::CallInfo(Js::CallFlags_Value, 1), targetObj);
    });

    if (!Js::JavascriptOperators::IsObject(ownKeysResult))
    {
        Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
    }

    Js::RecyclableObject* trapResultArray = Js::RecyclableObject::FromVar(ownKeysResult);
    Js::JavascriptArray* trapResult = requestContext->GetLibrary()->CreateArray(0);
    bool isConfigurableKeyMissingFromTrapResult = false;
    bool isNonconfigurableKeyMissingFromTrapResult = false;
    bool isKeyMissingFromTrapResult = false;
    bool isKeyMissingFromTargetResult = false;
    bool isAnyNonconfigurableKeyPresent = false;
    Js::Var element;
    Js::PropertyId propertyId;
    const Js::PropertyRecord* propertyRecord = nullptr;
    BOOL isTargetExtensible = FALSE;

    BEGIN_TEMP_ALLOCATOR(tempAllocator, requestContext, _u("Runtime"))
    {
        // Dictionary containing intersection of keys present in targetKeys and trapResult
        Js::Var lenValue = Js::JavascriptOperators::OP_GetLength(trapResultArray, requestContext);
        uint32 len = (uint32)Js::JavascriptConversion::ToLength(lenValue, requestContext);
        JsUtil::BaseDictionary<Js::PropertyId, bool, Memory::ArenaAllocator> targetToTrapResultMap(tempAllocator, len);

        // Trap result to return.
        // Note : This will not necessarily have all elements present in trapResultArray. E.g. If trap was called from GetOwnPropertySymbols()
        // trapResult will only contain symbol elements from trapResultArray.
        switch (keysTrapKind)
        {
        case GetOwnPropertyNamesKind:
            GetOwnPropertyKeysHelper(requestContext, trapResultArray, len, trapResult, targetToTrapResultMap,
                [&](const Js::PropertyRecord *propertyRecord)->bool
            {
                return !propertyRecord->IsSymbol();
            });
            break;
        case GetOwnPropertySymbolKind:
            GetOwnPropertyKeysHelper(requestContext, trapResultArray, len, trapResult, targetToTrapResultMap,
                [&](const Js::PropertyRecord *propertyRecord)->bool
            {
                return propertyRecord->IsSymbol();
            });
            break;
        case KeysKind:
            GetOwnPropertyKeysHelper(requestContext, trapResultArray, len, trapResult, targetToTrapResultMap,
                [&](const Js::PropertyRecord *propertyRecord)->bool
            {
                return true;
            });
            break;
        }

        isTargetExtensible = targetObj->IsExtensible();
        Js::JavascriptArray * targetKeys = Js::JavascriptOperators::GetOwnPropertyKeys(targetObj, requestContext);

        for (uint32 i = 0; i < targetKeys->GetLength(); i++)
        {
            element = targetKeys->DirectGetItem(i);
            AssertMsg(Js::JavascriptSymbol::Is(element) || Js::JavascriptString::Is(element), "Invariant check during ownKeys wrapper trap should make sure we only get property key here. (symbol or string primitives)");
            Js::JavascriptConversion::ToPropertyKey(element, requestContext, &propertyRecord, nullptr);
            propertyId = propertyRecord->GetPropertyId();

            if (propertyId == Js::Constants::NoProperty)
                continue;

            // If not present in intersection means either the property is not present in targetKeys or
            // we have already visited the property in targetKeys
            if (targetToTrapResultMap.ContainsKey(propertyId))
            {
                isKeyMissingFromTrapResult = false;
                targetToTrapResultMap.Remove(propertyId);
            }
            else
            {
                isKeyMissingFromTrapResult = true;
            }

            Js::PropertyDescriptor targetKeyPropertyDescriptor;
            if (Js::JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propertyId, requestContext, &targetKeyPropertyDescriptor) && !targetKeyPropertyDescriptor.IsConfigurable())
            {
                isAnyNonconfigurableKeyPresent = true;
                if (isKeyMissingFromTrapResult)
                {
                    isNonconfigurableKeyMissingFromTrapResult = true;
                }
            }
            else
            {
                if (isKeyMissingFromTrapResult)
                {
                    isConfigurableKeyMissingFromTrapResult = true;
                }
            }
        }
        // Keys that were not found in targetKeys will continue to remain in the map
        isKeyMissingFromTargetResult = targetToTrapResultMap.Count() != 0;
    }
    END_TEMP_ALLOCATOR(tempAllocator, requestContext)


    // 19.
    if (isTargetExtensible && !isAnyNonconfigurableKeyPresent)
    {
        return trapResult;
    }

    // 21.
    if (isNonconfigurableKeyMissingFromTrapResult)
    {
        Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
    }

    // 22.
    if (isTargetExtensible)
    {
        return trapResult;
    }

    // 23.
    if (isConfigurableKeyMissingFromTrapResult)
    {
        Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
    }

    // 24.
    if (isKeyMissingFromTargetResult)
    {
        Js::JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
    }

    return trapResult;
}

#if ENABLE_TTD
TTD::NSSnapObjects::SnapObjectType CustomExternalWrapperObject::GetSnapTag_TTD() const
{
    //TODO:akatti: TTD. Do we need to define a new TTD object type for CustomExternalWrapperObject?
    return TTD::NSSnapObjects::SnapObjectType::SnapExternalObject;
}

void CustomExternalWrapperObject::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
{
    TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<void*, TTD::NSSnapObjects::SnapObjectType::SnapExternalObject>(objData, nullptr);
}
#endif

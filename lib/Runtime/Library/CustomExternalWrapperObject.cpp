//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef _CHAKRACOREBUILD
#include "Types/PathTypeHandler.h"

using namespace Js;

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
    this->jsGetterSetterInterceptor = RecyclerNewStructZ(scriptContext->GetRecycler(), JsGetterSetterInterceptor);
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

CustomExternalWrapperObject::CustomExternalWrapperObject(CustomExternalWrapperObject * instance, bool deepCopy) :
    Js::DynamicObject(instance, deepCopy),
    initialized(instance->initialized)
{
    if (instance->GetInlineSlotSize() != 0)
    {
        this->slotType = SlotType::Inline;
        this->u.inlineSlotSize = instance->GetInlineSlotSize();
        if (instance->GetInlineSlots())
        {
            memcpy_s(this->GetInlineSlots(), this->GetInlineSlotSize(), instance->GetInlineSlots(), instance->GetInlineSlotSize());
        }
    }
    else
    {
        this->slotType = SlotType::External;
        this->u.slot = instance->GetInlineSlots();
    }
}

/* static */
CustomExternalWrapperObject* CustomExternalWrapperObject::Create(void *data, uint inlineSlotSize, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, JsGetterSetterInterceptor ** getterSetterInterceptor, Js::RecyclableObject * prototype, Js::ScriptContext *scriptContext)
{
    if (prototype == nullptr)
    {
        prototype = scriptContext->GetLibrary()->GetObjectPrototype();
    }

    Js::DynamicType * dynamicType = scriptContext->GetLibrary()->GetCachedCustomExternalWrapperType(reinterpret_cast<uintptr_t>(traceCallback), reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(*getterSetterInterceptor), reinterpret_cast<uintptr_t>(prototype));
    if (dynamicType == nullptr)
    {
        dynamicType = RecyclerNew(scriptContext->GetRecycler(), CustomExternalWrapperType, scriptContext, traceCallback, finalizeCallback, prototype);
        *getterSetterInterceptor = reinterpret_cast<CustomExternalWrapperType *>(dynamicType)->GetJsGetterSetterInterceptor();
        scriptContext->GetLibrary()->CacheCustomExternalWrapperType(reinterpret_cast<uintptr_t>(traceCallback), reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(*getterSetterInterceptor), reinterpret_cast<uintptr_t>(prototype), dynamicType);
    }
    else
    {
        if (*getterSetterInterceptor == nullptr)
        {
            *getterSetterInterceptor = reinterpret_cast<CustomExternalWrapperType *>(dynamicType)->GetJsGetterSetterInterceptor();
        }
        else
        {
            Assert(*getterSetterInterceptor == reinterpret_cast<CustomExternalWrapperType *>(dynamicType)->GetJsGetterSetterInterceptor());
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

BOOL CustomExternalWrapperObject::EnsureInitialized(ScriptContext* requestContext)
{
    ThreadContext* threadContext = requestContext->GetThreadContext();
    if (this->initialized)
    {
        return TRUE;
    }
    void* initializeTrap = this->GetExternalType()->GetJsGetterSetterInterceptor()->initializeTrap;
    if (initializeTrap == nullptr)
    {
        return TRUE;
    }
    if (threadContext->IsDisableImplicitCall())
    {
        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
        return FALSE;
    }

    this->initialized = true;
    JavascriptFunction * initializeMethod = Js::VarTo<JavascriptFunction>(initializeTrap);

    Js::RecyclableObject * targetObj = this;
    threadContext->ExecuteImplicitCall(initializeMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, initializeMethod, Js::CallInfo(Js::CallFlags_Value, 1), targetObj);
    });
    return TRUE;
}

CustomExternalWrapperObject* CustomExternalWrapperObject::Copy(bool deepCopy)
{
    Recycler* recycler = this->GetRecycler();
    CustomExternalWrapperType* externalType = this->GetExternalType();
    int inlineSlotSize = this->GetInlineSlotSize();

    if (externalType->GetJsTraceCallback() != nullptr)
    {
        return RecyclerNewTrackedPlus(recycler, inlineSlotSize, CustomExternalWrapperObject, this, deepCopy);
    }
    else if (externalType->GetJsFinalizeCallback() != nullptr)
    {
        return RecyclerNewFinalizedPlus(recycler, inlineSlotSize, CustomExternalWrapperObject, this, deepCopy);
    }
    else
    {
        return RecyclerNewPlus(recycler, inlineSlotSize, CustomExternalWrapperObject, this, deepCopy);
    }
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

BOOL CustomExternalWrapperObject::Equals(_In_ Var other, _Out_ BOOL* value, ScriptContext* requestContext)
{
    // We need to implement comparison to other by reference in case the object
    // is in the left side of the comparison, and does not call a toString
    // method (like different objects) when compared to string. For Node, such
    // comparision is used for Buffers.
    *value = (other == this);
    return true;
}

BOOL CustomExternalWrapperObject::StrictEquals(_In_ Var other, _Out_ BOOL* value, ScriptContext* requestContext)
{
    // We need to implement comparison to other by reference in case the object
    // is in the left side of the comparison, and does not call a toString
    // method (like different objects) when compared to string. For Node, such
    // comparision is used for Buffers.
    *value = (other == this);
    return true;
}

void CustomExternalWrapperObject::Mark(Recycler * recycler)
{
    recycler->SetNeedExternalWrapperTracing();
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
        ? unsafe_write_barrier_cast<void *>(this->u.slot)
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

    // setting the value could be deferred and now we are on a different context from
    // make sure the value has the same context as the containing object.
    newPrototype = VarTo<RecyclableObject>(Js::CrossSite::MarshalVar(this->GetScriptContext(), newPrototype));
    DynamicObject::SetPrototype(newPrototype);
}

Js::Var CustomExternalWrapperObject::GetValueFromDescriptor(Js::Var instance, Js::PropertyDescriptor propertyDescriptor, Js::ScriptContext * requestContext)
{
    if (propertyDescriptor.ValueSpecified())
    {
        return propertyDescriptor.GetValue();
    }
    if (propertyDescriptor.GetterSpecified())
    {
        return Js::JavascriptOperators::CallGetter(VarTo<RecyclableObject>(propertyDescriptor.GetGetter()), instance, requestContext);
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

Js::Var CustomExternalWrapperObject::GetName(Js::ScriptContext* requestContext, Js::PropertyId propertyId, Var * isPropertyNameNumeric, Var * propertyNameNumericValue)
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

    *isPropertyNameNumeric = propertyRecord->IsNumeric() ? requestContext->GetLibrary()->GetTrue() : requestContext->GetLibrary()->GetFalse();
    *propertyNameNumericValue = propertyRecord->IsNumeric() ? JavascriptNumber::ToVar(propertyRecord->GetNumericValue(), requestContext) : nullptr;
    return name;
}

template <class Fn, class GetPropertyNameFunc>
BOOL CustomExternalWrapperObject::GetPropertyTrap(Js::Var instance, Js::PropertyDescriptor * propertyDescriptor, Fn fn, GetPropertyNameFunc getPropertyName, Js::ScriptContext * requestContext, Js::PropertyValueInfo* info)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    ThreadContext* threadContext = requestContext->GetThreadContext();

    if (!this->EnsureInitialized(requestContext))
    {
        return FALSE;
    }

    Js::RecyclableObject * targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* getGetMethod = nullptr;
    if (type->GetJsGetterSetterInterceptor()->getTrap != nullptr)
    {
        getGetMethod = Js::VarTo<JavascriptFunction>(type->GetJsGetterSetterInterceptor()->getTrap);
    }

    if (nullptr == getGetMethod || requestContext->IsHeapEnumInProgress())
    {
        return fn(targetObj);
    }

    Js::Var isPropertyNameNumeric;
    Js::Var propertyNameNumericValue;
    Js::Var propertyName = getPropertyName(requestContext, &isPropertyNameNumeric, &propertyNameNumericValue);

    PropertyValueInfo::SetNoCache(info, targetObj);
    PropertyValueInfo::DisablePrototypeCache(info, targetObj);
    Js::Var getGetResult = threadContext->ExecuteImplicitCall(getGetMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, getGetMethod, Js::CallInfo(Js::CallFlags_Value, 4), targetObj, propertyName, isPropertyNameNumeric, propertyNameNumericValue);
    });

    Js::TypeId getResultTypeId = Js::JavascriptOperators::GetTypeId(getGetResult);
    if (getResultTypeId == Js::TypeIds_Undefined)
    {
        return FALSE;
    }

    propertyDescriptor->SetValue(getGetResult);
    return TRUE;
}

template <class Fn, class GetPropertyNameFunc>
BOOL CustomExternalWrapperObject::HasPropertyTrap(Fn fn, GetPropertyNameFunc getPropertyName, Js::PropertyValueInfo* info)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    ThreadContext* threadContext = GetScriptContext()->GetThreadContext();

    // Caller does not pass requestContext. Retrieve from host scriptContext stack.
    Js::ScriptContext* requestContext =
        threadContext->GetPreviousHostScriptContext()->GetScriptContext();

    if (!this->EnsureInitialized(requestContext))
    {
        return FALSE;
    }

    Js::RecyclableObject *targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* hasMethod = nullptr;
    if (type->GetJsGetterSetterInterceptor()->hasTrap != nullptr)
    {
        hasMethod = Js::VarTo<JavascriptFunction>(type->GetJsGetterSetterInterceptor()->hasTrap);
    }

    if (nullptr == hasMethod || requestContext->IsHeapEnumInProgress())
    {
        return fn(targetObj);
    }

    Js::Var isPropertyNameNumeric;
    Js::Var propertyNameNumericValue;
    Js::Var propertyName = getPropertyName(requestContext, &isPropertyNameNumeric, &propertyNameNumericValue);

    PropertyValueInfo::SetNoCache(info, targetObj);
    PropertyValueInfo::DisablePrototypeCache(info, targetObj);
    Js::Var getHasResult = threadContext->ExecuteImplicitCall(hasMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, hasMethod, Js::CallInfo(Js::CallFlags_Value, 4), targetObj, propertyName, isPropertyNameNumeric, propertyNameNumericValue);
    });

    BOOL hasProperty = Js::JavascriptConversion::ToBoolean(getHasResult, requestContext);
    return hasProperty;
}

Js::PropertyQueryFlags CustomExternalWrapperObject::HasPropertyQuery(Js::PropertyId propertyId, _Inout_opt_ Js::PropertyValueInfo* info)
{
    auto fn = [&](RecyclableObject* object)->BOOL {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasPropertyQuery(propertyId, info));
    };
    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        return GetName(requestContext, propertyId, isPropertyNameNumeric, propertyNameNumericValue);
    };
    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(HasPropertyTrap(fn, getPropertyName, info));
}

BOOL CustomExternalWrapperObject::GetPropertyDescriptorTrap(Js::PropertyId propertyId, Js::PropertyDescriptor* resultDescriptor, Js::ScriptContext* requestContext)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    ThreadContext* threadContext = requestContext->GetThreadContext();

    if (!this->EnsureInitialized(requestContext))
    {
        return FALSE;
    }

    Js::RecyclableObject * targetObj = this;

    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* gOPDMethod = nullptr;
    if (type->GetJsGetterSetterInterceptor()->getOwnPropertyDescriptorTrap != nullptr)
    {
        gOPDMethod = Js::VarTo<JavascriptFunction>(type->GetJsGetterSetterInterceptor()->getOwnPropertyDescriptorTrap);
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

    Js::Var isPropertyNameNumeric;
    Js::Var propertyNameNumericValue;
    Js::Var propertyName = GetName(requestContext, propertyId, &isPropertyNameNumeric, &propertyNameNumericValue);

    Assert(Js::VarIs<JavascriptString>(propertyName) || Js::VarIs<JavascriptSymbol>(propertyName));

    Js::Var getResult = threadContext->ExecuteImplicitCall(gOPDMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, gOPDMethod, Js::CallInfo(Js::CallFlags_Value, 4), targetObj, propertyName, isPropertyNameNumeric, propertyNameNumericValue);
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
    CustomExternalWrapperObject * customObject = VarTo<CustomExternalWrapperObject>(obj);
    return customObject->GetPropertyDescriptorTrap(propertyId, propertyDescriptor, requestContext);
}

BOOL CustomExternalWrapperObject::DefineOwnPropertyDescriptor(Js::RecyclableObject * obj, Js::PropertyId propId, const Js::PropertyDescriptor& descriptor, bool throwOnError, Js::ScriptContext * requestContext)
{
    PROBE_STACK(requestContext, Js::Constants::MinStackDefault);

    ThreadContext* threadContext = requestContext->GetThreadContext();

    CustomExternalWrapperObject * customObject = VarTo<CustomExternalWrapperObject>(obj);

    if (!customObject->EnsureInitialized(requestContext))
    {
        return FALSE;
    }

    Js::RecyclableObject * targetObj = obj;

    CustomExternalWrapperType * type = customObject->GetExternalType();
    Js::JavascriptFunction* defineOwnPropertyMethod = nullptr;
    if (type->GetJsGetterSetterInterceptor()->definePropertyTrap != nullptr)
    {
        defineOwnPropertyMethod = Js::VarTo<JavascriptFunction>(type->GetJsGetterSetterInterceptor()->definePropertyTrap);
    }

    Assert(!requestContext->IsHeapEnumInProgress());
    if (nullptr == defineOwnPropertyMethod)
    {
        Js::PropertyDescriptor desc = descriptor;

        if(desc.ValueSpecified())
        {
            // setting the value could be deferred and now we are on a different context from
            // make sure the value has the same context as the containing object.
            desc.SetValue(Js::CrossSite::MarshalVar(targetObj->GetScriptContext(), descriptor.GetValue()));
        }

        PropertyDescriptor currentDescriptor;
        BOOL isCurrentDescriptorDefined = JavascriptOperators::GetOwnPropertyDescriptor(obj, propId, requestContext, &currentDescriptor);

        bool isExtensible = !!obj->IsExtensible();
        return Js::JavascriptOperators::ValidateAndApplyPropertyDescriptor<true>(obj, propId, desc, isCurrentDescriptorDefined ? &currentDescriptor : nullptr, isExtensible, throwOnError, requestContext);
    }

    Js::Var descVar = descriptor.GetOriginal();
    if (descVar == nullptr)
    {
        descVar = Js::JavascriptOperators::FromPropertyDescriptor(descriptor, requestContext);
    }

    Js::Var isPropertyNameNumeric;
    Js::Var propertyNameNumericValue;
    Js::Var propertyName = customObject->GetName(requestContext, propId, &isPropertyNameNumeric, &propertyNameNumericValue);

    Js::Var definePropertyResult = threadContext->ExecuteImplicitCall(defineOwnPropertyMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, defineOwnPropertyMethod, Js::CallInfo(Js::CallFlags_Value, 5), targetObj, propertyName, isPropertyNameNumeric, propertyNameNumericValue, descVar);
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

BOOL CustomExternalWrapperObject::SetPropertyTrap(Js::Var receiver, SetPropertyTrapKind setPropertyTrapKind, Js::JavascriptString * propertyNameString, Js::Var newValue, Js::ScriptContext * requestContext, Js::PropertyOperationFlags propertyOperationFlags, Js::PropertyValueInfo* info)
{
    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        const Js::PropertyRecord* propertyRecord;
        propertyNameString->GetPropertyRecord(&propertyRecord);
        return GetName(requestContext, propertyRecord->GetPropertyId(), isPropertyNameNumeric, propertyNameNumericValue);
    };
    auto fn = [&]()->BOOL
    {
        // setting the value could be deferred and now we are on a different context from
        // make sure the value has the same context as the containing object.
        newValue = Js::CrossSite::MarshalVar(this->GetScriptContext(), newValue);
        return DynamicObject::SetProperty(propertyNameString, newValue, propertyOperationFlags, nullptr);
    };
    return SetPropertyTrap(receiver, setPropertyTrapKind, getPropertyName, newValue, requestContext, propertyOperationFlags, FALSE, fn, info);
}

template <class Fn, class GetPropertyNameFunc>
BOOL CustomExternalWrapperObject::SetPropertyTrap(Js::Var receiver, SetPropertyTrapKind setPropertyTrapKind, GetPropertyNameFunc getPropertyName, Js::Var newValue, Js::ScriptContext * requestContext, Js::PropertyOperationFlags propertyOperationFlags, BOOL skipPrototypeCheck, Fn fn, Js::PropertyValueInfo* info)
{
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    ThreadContext* threadContext = requestContext->GetThreadContext();

    if (!this->EnsureInitialized(requestContext))
    {
        return FALSE;
    }

    Js::RecyclableObject *targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* setMethod = nullptr;
    if (type->GetJsGetterSetterInterceptor()->setTrap != nullptr)
    {
        setMethod = Js::VarTo<JavascriptFunction>(type->GetJsGetterSetterInterceptor()->setTrap);
    }
    else
    {
        return fn();
    }

    Assert(!GetScriptContext()->IsHeapEnumInProgress());
    Js::Var isPropertyNameNumeric;
    Js::Var propertyNameNumericValue;
    Js::Var propertyName = getPropertyName(requestContext, &isPropertyNameNumeric, &propertyNameNumericValue);

    PropertyValueInfo::SetNoCache(info, targetObj);
    PropertyValueInfo::DisablePrototypeCache(info, targetObj);
    if (nullptr != setMethod)
    {
        Js::Var setPropertyResult = threadContext->ExecuteImplicitCall(setMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, setMethod, Js::CallInfo(Js::CallFlags_Value, 5), targetObj, propertyName, isPropertyNameNumeric, propertyNameNumericValue, newValue);
        });

        BOOL setResult = Js::JavascriptConversion::ToBoolean(setPropertyResult, requestContext);
        return setResult;
    }

    return FALSE;
}

Js::PropertyQueryFlags CustomExternalWrapperObject::GetPropertyQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext)
{
    if (!this->VerifyObjectAlive()) return Js::PropertyQueryFlags::Property_NotFound;
    Js::PropertyDescriptor result;

    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        BOOL found = JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext));
        result.SetValue(*value);
        return found;
    };
    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        return GetName(requestContext, propertyId, isPropertyNameNumeric, propertyNameNumericValue);
    };
    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyName, requestContext, info);
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
    Js::PropertyDescriptor result;

    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        BOOL found = JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext));
        result.SetValue(*value);
        return found;
    };

    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        const Js::PropertyRecord* propertyRecord;
        propertyNameString->GetPropertyRecord(&propertyRecord);
        return GetName(requestContext, propertyRecord->GetPropertyId(), isPropertyNameNumeric, propertyNameNumericValue);
    };

    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyName, requestContext, info);
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
    Js::PropertyDescriptor result;

    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        BOOL found = JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetPropertyReferenceQuery(originalInstance, propertyId, value, info, requestContext));
        result.SetValue(*value);
        return found;
    };

    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        return GetName(requestContext, propertyId, isPropertyNameNumeric, propertyNameNumericValue);
    };

    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyName, requestContext, info);
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
    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::HasItemQuery(index));
    };

    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        *isPropertyNameNumeric = requestContext->GetLibrary()->GetTrue();
        *propertyNameNumericValue = JavascriptNumber::ToVar(index, requestContext);
        return nullptr;
    };

    return Js::JavascriptConversion::BooleanToPropertyQueryFlags(HasPropertyTrap(fn, getPropertyName, nullptr));
}

BOOL CustomExternalWrapperObject::HasOwnItem(uint32 index)
{
    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        return DynamicObject::HasOwnItem(index);
    };
    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        *isPropertyNameNumeric = requestContext->GetLibrary()->GetTrue();
        *propertyNameNumericValue = JavascriptNumber::ToVar(index, requestContext);
        return nullptr;
    };

    return HasPropertyTrap(fn, getPropertyName, nullptr);
}

Js::PropertyQueryFlags CustomExternalWrapperObject::GetItemQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * requestContext)
{
    if (!this->VerifyObjectAlive()) return Js::PropertyQueryFlags::Property_NotFound;
    Js::PropertyDescriptor result;

    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        BOOL found = JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetItemQuery(originalInstance, index, value, requestContext));
        result.SetValue(*value);
        return found;
    };

    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        *isPropertyNameNumeric = requestContext->GetLibrary()->GetTrue();
        *propertyNameNumericValue = JavascriptNumber::ToVar(index, requestContext);
        return nullptr;
    };

    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyName, requestContext, nullptr);
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

    Js::PropertyDescriptor result;
    auto fn = [&](Js::RecyclableObject* object)-> BOOL {
        BOOL found = JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetItemReferenceQuery(originalInstance, index, value, requestContext));
        result.SetValue(*value);
        return found;
    };

    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        *isPropertyNameNumeric = requestContext->GetLibrary()->GetTrue();
        *propertyNameNumericValue = JavascriptNumber::ToVar(index, requestContext);
        return nullptr;
    };

    BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyName, requestContext, nullptr);
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

    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        *isPropertyNameNumeric = requestContext->GetLibrary()->GetTrue();
        *propertyNameNumericValue = JavascriptNumber::ToVar(index, requestContext);
        return nullptr;
    };

    auto fn = [&]()->BOOL
    {
        // setting the value could be deferred and now we are on a different context from
        // make sure the value has the same context as the containing object.
        value = Js::CrossSite::MarshalVar(this->GetScriptContext(), value);
        return DynamicObject::SetItem(index, value, flags);
    };
    BOOL trapResult = SetPropertyTrap(this, CustomExternalWrapperObject::SetPropertyTrapKind::SetItemKind, getPropertyName, value, GetScriptContext(), flags, FALSE, fn, nullptr);
    if (!trapResult)
    {
        // setting the value could be deferred and now we are on a different context from
        // make sure the value has the same context as the containing object.
        value = Js::CrossSite::MarshalVar(this->GetScriptContext(), value);
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
    ThreadContext* threadContext = requestContext->GetThreadContext();

    if (!this->EnsureInitialized(requestContext))
    {
        return FALSE;
    }

    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* getEnumeratorMethod = nullptr;
    if (type->GetJsGetterSetterInterceptor()->enumerateTrap != nullptr)
    {
        getEnumeratorMethod = Js::VarTo<JavascriptFunction>(type->GetJsGetterSetterInterceptor()->enumerateTrap);
    }

    Js::RecyclableObject * targetObj = this;
    JavascriptArray * trapResult = nullptr;
    if (getEnumeratorMethod == nullptr)
    {
        return DynamicObject::GetEnumerator(enumerator, flags, requestContext, enumeratorCache);
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
                        if (VarIs<JavascriptString>(var))
                        {
                            JavascriptString* propertyName = VarTo<JavascriptString>(var);
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
                                    //TODO: (vsadov) not sure if should marshal here, it is "getting"
                                    return VarTo<JavascriptString>(CrossSite::MarshalVar(
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
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        return GetName(requestContext, propertyId, isPropertyNameNumeric, propertyNameNumericValue);
    };

    auto fn = [&]()->BOOL
    {
        // setting the value could be deferred and now we are on a different context from
        // make sure the value has the same context as the containing object.
        value = Js::CrossSite::MarshalVar(this->GetScriptContext(), value);
        return DynamicObject::SetProperty(propertyId, value, flags, info);
    };

    return SetPropertyTrap(this, CustomExternalWrapperObject::SetPropertyTrapKind::SetPropertyKind, getPropertyName, value, GetScriptContext(), flags, FALSE, fn, info);
}

BOOL CustomExternalWrapperObject::SetProperty(Js::JavascriptString* propertyNameString, Js::Var value, Js::PropertyOperationFlags flags, Js::PropertyValueInfo* info)
{
    if (!this->VerifyObjectAlive()) return FALSE;
    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    auto getPropertyName = [&](Js::ScriptContext * requestContext, Js::Var * isPropertyNameNumeric, Js::Var * propertyNameNumericValue)->Js::Var
    {
        return propertyNameString;
    };

    auto fn = [&]()->BOOL
    {
        // setting the value could be deferred and now we are on a different context from
        // make sure the value has the same context as the containing object.
        value = Js::CrossSite::MarshalVar(this->GetScriptContext(), value);
        return DynamicObject::SetProperty(propertyNameString, value, flags, info);
    };

    return SetPropertyTrap(this, CustomExternalWrapperObject::SetPropertyTrapKind::SetPropertyKind, getPropertyName, value, GetScriptContext(), flags, FALSE, fn, info);
}

BOOL CustomExternalWrapperObject::EnsureNoRedeclProperty(Js::PropertyId propertyId)
{
    CustomExternalWrapperObject::HasOwnPropertyCheckNoRedecl(propertyId);
    return true;
}

BOOL CustomExternalWrapperObject::InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
{
    if (!this->VerifyObjectAlive()) return FALSE;

    ThreadContext* threadContext = GetScriptContext()->GetThreadContext();

    // Caller does not pass requestContext. Retrieve from host scriptContext stack.
    Js::ScriptContext* requestContext =
        threadContext->GetPreviousHostScriptContext()->GetScriptContext();

    if (!this->EnsureInitialized(requestContext))
    {
        return FALSE;
    }

    // setting the value could be deferred and now we are on a different context from
    // make sure the value has the same context as the containing object.
    value = Js::CrossSite::MarshalVar(this->GetScriptContext(), value);
    return DynamicObject::InitProperty(propertyId, value, flags, info);
}

BOOL CustomExternalWrapperObject::DeleteProperty(Js::PropertyId propertyId, Js::PropertyOperationFlags flags)
{
    if (!this->VerifyObjectAlive()) return FALSE;

    PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

    ThreadContext* threadContext = GetScriptContext()->GetThreadContext();

    // Caller does not pass requestContext. Retrieve from host scriptContext stack.
    Js::ScriptContext* requestContext =
        threadContext->GetPreviousHostScriptContext()->GetScriptContext();

    if (!this->EnsureInitialized(requestContext))
    {
        return FALSE;
    }

    RecyclableObject * targetObj = this;
    CustomExternalWrapperType * type = this->GetExternalType();
    Js::JavascriptFunction* deleteMethod = nullptr;
    if (type->GetJsGetterSetterInterceptor()->deletePropertyTrap != nullptr)
    {
        deleteMethod = Js::VarTo<JavascriptFunction>(type->GetJsGetterSetterInterceptor()->deletePropertyTrap);
    }

    Assert(!GetScriptContext()->IsHeapEnumInProgress());
    if (nullptr == deleteMethod)
    {
        return __super::DeleteProperty(propertyId, flags);
    }

    Js::Var isPropertyNameNumeric;
    Js::Var propertyNameNumericValue;
    Js::Var propertyName = GetName(requestContext, propertyId, &isPropertyNameNumeric, &propertyNameNumericValue);

    Js::Var deletePropertyResult = threadContext->ExecuteImplicitCall(deleteMethod, Js::ImplicitCall_Accessor, [=]()->Js::Var
    {
        return CALL_FUNCTION(threadContext, deleteMethod, Js::CallInfo(Js::CallFlags_Value, 4), targetObj, propertyName, isPropertyNameNumeric, propertyNameNumericValue);
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
#endif // _CHAKRACOREBUILD

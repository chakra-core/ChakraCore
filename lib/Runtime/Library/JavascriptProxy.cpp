//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    bool JavascriptProxy::IsRevoked() const
    {
        if (target == nullptr)
        {
            Assert(handler == nullptr);
            return true;
        }
        return false;
    }

    RecyclableObject* JavascriptProxy::GetTarget()
    {
        if (IsRevoked())
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u(""));
        }
        return target;
    }

    RecyclableObject* JavascriptProxy::GetHandler()
    {
        if (IsRevoked())
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u(""));
        }
        return handler;
    }


    Var JavascriptProxy::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Proxy, scriptContext);

        if (!(args.Info.Flags & CallFlags_New))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ErrorOnNew, _u("Proxy"));
        }

        JavascriptProxy* proxy = JavascriptProxy::Create(scriptContext, args);
        return proxy;
    }

    JavascriptProxy* JavascriptProxy::Create(ScriptContext* scriptContext, Arguments args)
    {
        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch.

        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::IsConstructorSuperCall(args);

        RecyclableObject* target, *handler;

        if (args.Info.Count < 3)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedProxyArgument);
        }

        if (!JavascriptOperators::IsObjectType(JavascriptOperators::GetTypeId(args[1])))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidProxyArgument, _u("target"));
        }
        target = VarTo<DynamicObject>(args[1]);
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(target);
#endif
        if (VarIs<JavascriptProxy>(target))
        {
            if (UnsafeVarTo<JavascriptProxy>(target)->IsRevoked())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidProxyArgument, _u("target"));
            }
        }

        if (!JavascriptOperators::IsObjectType(JavascriptOperators::GetTypeId(args[2])))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidProxyArgument, _u("handler"));
        }
        handler = VarTo<DynamicObject>(args[2]);
        if (VarIs<JavascriptProxy>(handler))
        {
            if (UnsafeVarTo<JavascriptProxy>(handler)->IsRevoked())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidProxyArgument, _u("handler"));
            }
        }

        JavascriptProxy* newProxy = RecyclerNew(scriptContext->GetRecycler(), JavascriptProxy, scriptContext->GetLibrary()->GetProxyType(), scriptContext, target, handler);
        if (JavascriptConversion::IsCallable(target))
        {
            newProxy->ChangeType();
            newProxy->GetDynamicType()->SetEntryPoint(JavascriptProxy::FunctionCallTrap);
        }
        return isCtorSuperCall ?
            VarTo<JavascriptProxy>(JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), newProxy, nullptr, scriptContext)) :
            newProxy;
    }

    Var JavascriptProxy::EntryRevocable(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Proxy.revocable"));

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        if (args.Info.Flags & CallFlags_New)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ErrorOnNew, _u("Proxy.revocable"));
        }

        JavascriptProxy* proxy = JavascriptProxy::Create(scriptContext, args);
        JavascriptLibrary* library = scriptContext->GetLibrary();
        DynamicType* type = library->CreateFunctionWithConfigurableLengthType(&EntryInfo::Revoke);
        RuntimeFunction* revoker = RecyclerNewEnumClass(scriptContext->GetRecycler(),
            JavascriptLibrary::EnumFunctionClass, RuntimeFunction,
            type, &EntryInfo::Revoke);

        revoker->SetPropertyWithAttributes(Js::PropertyIds::length, Js::TaggedInt::ToVarUnchecked(2), PropertyConfigurable, NULL);
        revoker->SetInternalProperty(Js::InternalPropertyIds::RevocableProxy, proxy, PropertyOperationFlags::PropertyOperation_Force, nullptr);

        DynamicObject* obj = scriptContext->GetLibrary()->CreateObject(true, 2);
        JavascriptOperators::SetProperty(obj, obj, PropertyIds::proxy, proxy, scriptContext);
        JavascriptOperators::SetProperty(obj, obj, PropertyIds::revoke, revoker, scriptContext);
        return obj;
    }

    Var JavascriptProxy::EntryRevoke(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Proxy.revoke"));

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Var revokableProxy;
        if (!function->GetInternalProperty(function, Js::InternalPropertyIds::RevocableProxy, &revokableProxy, nullptr, scriptContext))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidProxyArgument, _u(""));
        }
        TypeId typeId = JavascriptOperators::GetTypeId(revokableProxy);
        if (typeId == TypeIds_Null)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
        if (typeId != TypeIds_Proxy)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidProxyArgument, _u(""));
        }
        function->SetInternalProperty(Js::InternalPropertyIds::RevocableProxy, scriptContext->GetLibrary()->GetNull(), PropertyOperationFlags::PropertyOperation_Force, nullptr);
        (VarTo<JavascriptProxy>(revokableProxy))->RevokeObject();

        return scriptContext->GetLibrary()->GetUndefined();
    }

    JavascriptProxy::JavascriptProxy(DynamicType * type) :
        DynamicObject(type),
        handler(nullptr),
        target(nullptr)
    {
        type->SetHasSpecialPrototype(true);
    }

    JavascriptProxy::JavascriptProxy(DynamicType * type, ScriptContext * scriptContext, RecyclableObject* target, RecyclableObject* handler) :
        DynamicObject(type),
        handler(handler),
        target(target)
    {
        type->SetHasSpecialPrototype(true);
    }

    void JavascriptProxy::RevokeObject()
    {
        handler = nullptr;
        target = nullptr;
    }

    BOOL JavascriptProxy::GetPropertyDescriptorTrap(PropertyId propertyId, PropertyDescriptor* resultDescriptor, ScriptContext* requestContext)
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return FALSE;
        }

        //1. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        Js::RecyclableObject *handlerObj = this->MarshalHandler(requestContext);

        //2. If handler is null, then throw a TypeError exception.
        if (handlerObj == nullptr)
        {
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, _u("getOwnPropertyDescriptor"));
        }
        //3. Let target be the value of the[[ProxyTarget]] internal slot of O.
        Js::RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        Assert((static_cast<DynamicType*>(GetType()))->GetTypeHandler()->GetPropertyCount() == 0 ||
            (static_cast<DynamicType*>(GetType()))->GetTypeHandler()->GetPropertyId(GetScriptContext(), 0) == InternalPropertyIds::WeakMapKeyMap);

        JavascriptFunction* gOPDMethod = GetMethodHelper(PropertyIds::getOwnPropertyDescriptor, requestContext);

        //7. If trap is undefined, then
        //    a.Return the result of calling the[[GetOwnProperty]] internal method of target with argument P.
        if (nullptr == gOPDMethod || GetScriptContext()->IsHeapEnumInProgress())
        {
            resultDescriptor->SetFromProxy(false);
            return JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propertyId, requestContext, resultDescriptor);
        }

        Var propertyName = GetName(requestContext, propertyId);

        Assert(VarIs<JavascriptString>(propertyName) || VarIs<JavascriptSymbol>(propertyName));
        //8. Let trapResultObj be the result of calling the[[Call]] internal method of trap with handler as the this value and a new List containing target and P.
        //9. ReturnIfAbrupt(trapResultObj).
        //10. If Type(trapResultObj) is neither Object nor Undefined, then throw a TypeError exception.

        Var getResult = threadContext->ExecuteImplicitCall(gOPDMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, gOPDMethod, CallInfo(CallFlags_Value, 3), handlerObj, targetObj, propertyName);
        });

        TypeId getResultTypeId = JavascriptOperators::GetTypeId(getResult);
        if (StaticType::Is(getResultTypeId) && getResultTypeId != TypeIds_Undefined)
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_NeedObject, _u("getOwnPropertyDescriptor"));
        }
        //11. Let targetDesc be the result of calling the[[GetOwnProperty]] internal method of target with argument P.
        //12. ReturnIfAbrupt(targetDesc).
        PropertyDescriptor targetDescriptor;
        BOOL hasProperty = JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propertyId, requestContext, &targetDescriptor);

        //13. If trapResultObj is undefined, then
        //a.If targetDesc is undefined, then return undefined.
        //b.If targetDesc.[[Configurable]] is false, then throw a TypeError exception.
        //c.Let extensibleTarget be the result of IsExtensible(target).
        //d.ReturnIfAbrupt(extensibleTarget).
        //e.If ToBoolean(extensibleTarget) is false, then throw a TypeError exception.
        //f.Return undefined.
        if (getResultTypeId == TypeIds_Undefined)
        {
            if (!hasProperty)
            {
                return FALSE;
            }
            if (!targetDescriptor.IsConfigurable())
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("getOwnPropertyDescriptor"));
            }

            // do not use "target" here, the trap may have caused it to change
            if (!targetObj->IsExtensible())
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("getOwnPropertyDescriptor"));
            }

            return FALSE;
        }

        //14. Let extensibleTarget be the result of IsExtensible(target).
        //15. ReturnIfAbrupt(extensibleTarget).
        //16. Let resultDesc be ToPropertyDescriptor(trapResultObj).
        //17. ReturnIfAbrupt(resultDesc).
        //18. Call CompletePropertyDescriptor(resultDesc, targetDesc).
        //19. Let valid be the result of IsCompatiblePropertyDescriptor(extensibleTarget, resultDesc, targetDesc).
        //20. If valid is false, then throw a TypeError exception.
        //21. If resultDesc.[[Configurable]] is false, then
        //a.If targetDesc is undefined or targetDesc.[[Configurable]] is true, then
        //i.Throw a TypeError exception.
        //22. Return resultDesc.

        // do not use "target" here, the trap may have caused it to change
        BOOL isTargetExtensible = targetObj->IsExtensible();
        BOOL toProperty = JavascriptOperators::ToPropertyDescriptor(getResult, resultDescriptor, requestContext);
        if (!toProperty && isTargetExtensible)
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("getOwnPropertyDescriptor"));
        }

        JavascriptOperators::CompletePropertyDescriptor(resultDescriptor, nullptr, requestContext);
        if (!JavascriptOperators::IsCompatiblePropertyDescriptor(*resultDescriptor, hasProperty ? &targetDescriptor : nullptr, !!isTargetExtensible, true, requestContext))
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("getOwnPropertyDescriptor"));
        }
        if (!resultDescriptor->IsConfigurable())
        {
            if (!hasProperty || targetDescriptor.IsConfigurable())
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("getOwnPropertyDescriptor"));
            }
        }
        resultDescriptor->SetFromProxy(true);
        return toProperty;
    }

    template <class Fn, class GetPropertyIdFunc>
    BOOL JavascriptProxy::GetPropertyTrap(Var instance, PropertyDescriptor* propertyDescriptor, Fn fn, GetPropertyIdFunc getPropertyId, ScriptContext* requestContext)
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return FALSE;
        }

        Js::RecyclableObject *handlerObj = this->MarshalHandler(requestContext);

        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, _u("get"));
        }

        RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        JavascriptFunction* getGetMethod = GetMethodHelper(PropertyIds::get, requestContext);

        if (nullptr == getGetMethod || requestContext->IsHeapEnumInProgress())
        {
            propertyDescriptor->SetFromProxy(false);
            return fn(targetObj);
        }

        PropertyId propertyId = getPropertyId();
        propertyDescriptor->SetFromProxy(true);
        Var propertyName = GetName(requestContext, propertyId);

        Var getGetResult = threadContext->ExecuteImplicitCall(getGetMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, getGetMethod, CallInfo(CallFlags_Value, 4), handlerObj, targetObj, propertyName, instance);
        });

        //    9. Let targetDesc be the result of calling the[[GetOwnProperty]] internal method of target with argument P.
        //    10. ReturnIfAbrupt(targetDesc).
        //    11. If targetDesc is not undefined, then
        //    a.If IsDataDescriptor(targetDesc) and targetDesc.[[Configurable]] is false and targetDesc.[[Writable]] is false, then
        //        i.If SameValue(trapResult, targetDesc.[[Value]]) is false, then throw a TypeError exception.
        //    b.If IsAccessorDescriptor(targetDesc) and targetDesc.[[Configurable]] is false and targetDesc.[[Get]] is undefined, then
        //        i.If trapResult is not undefined, then throw a TypeError exception.
        //    12. Return trapResult.
        PropertyDescriptor targetDescriptor;
        Var defaultAccessor = requestContext->GetLibrary()->GetDefaultAccessorFunction();
        if (JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propertyId, requestContext, &targetDescriptor))
        {
            JavascriptOperators::CompletePropertyDescriptor(&targetDescriptor, nullptr, requestContext);
            if (targetDescriptor.ValueSpecified() && !targetDescriptor.IsConfigurable() && !targetDescriptor.IsWritable())
            {
                if (!JavascriptConversion::SameValue(getGetResult, targetDescriptor.GetValue()))
                {
                    JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("get"));
                }
            }
            else if (targetDescriptor.GetterSpecified() || targetDescriptor.SetterSpecified())
            {
                if (!targetDescriptor.IsConfigurable() &&
                    targetDescriptor.GetGetter() == defaultAccessor &&
                    JavascriptOperators::GetTypeId(getGetResult) != TypeIds_Undefined)
                {
                    JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("get"));
                }
            }
        }
        propertyDescriptor->SetValue(getGetResult);

        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);

        return TRUE;
    }

    template <class Fn, class GetPropertyIdFunc>
    BOOL JavascriptProxy::HasPropertyTrap(Fn fn, GetPropertyIdFunc getPropertyId)
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
        ScriptContext* requestContext =
            threadContext->GetPreviousHostScriptContext()->GetScriptContext();

        Js::RecyclableObject *handlerObj = this->MarshalHandler(requestContext);

        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, _u("has"));
        }

        Js::RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        JavascriptFunction* hasMethod = GetMethodHelper(PropertyIds::has, requestContext);

        if (nullptr == hasMethod || requestContext->IsHeapEnumInProgress())
        {
            return fn(targetObj);
        }

        PropertyId propertyId = getPropertyId();
        Var propertyName = GetName(requestContext, propertyId);

        Var getHasResult = threadContext->ExecuteImplicitCall(hasMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, hasMethod, CallInfo(CallFlags_Value, 3), handlerObj, targetObj, propertyName);
        });

        //9. Let booleanTrapResult be ToBoolean(trapResult).
        //10. ReturnIfAbrupt(booleanTrapResult).
        //11. If booleanTrapResult is false, then
        //    a.Let targetDesc be the result of calling the[[GetOwnProperty]] internal method of target with argument P.
        //    b.ReturnIfAbrupt(targetDesc).
        //    c.If targetDesc is not undefined, then
        //        i.If targetDesc.[[Configurable]] is false, then throw a TypeError exception.
        //        ii.Let extensibleTarget be the result of IsExtensible(target).
        //        iii.ReturnIfAbrupt(extensibleTarget).
        //        iv.If ToBoolean(extensibleTarget) is false, then throw a TypeError exception
        BOOL hasProperty = JavascriptConversion::ToBoolean(getHasResult, requestContext);
        if (!hasProperty)
        {
            PropertyDescriptor targetDescriptor;
            BOOL hasTargetProperty = JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propertyId, requestContext, &targetDescriptor);
            if (hasTargetProperty)
            {
                if (!targetDescriptor.IsConfigurable() || !targetObj->IsExtensible())
                {
                    JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("has"));
                }
            }
        }
        return hasProperty;
    }

    PropertyQueryFlags JavascriptProxy::HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info)
    {
        if (info)
        {
            // Prevent caching. See comment in GetPropertyQuery for more detail.
            PropertyValueInfo::SetNoCache(info, this);
            PropertyValueInfo::DisablePrototypeCache(info, this);
        }
        auto fn = [&](RecyclableObject* object)->BOOL {
            return JavascriptOperators::HasProperty(object, propertyId);
        };
        auto getPropertyId = [&]() ->PropertyId {
            return propertyId;
        };
        return JavascriptConversion::BooleanToPropertyQueryFlags(HasPropertyTrap(fn, getPropertyId));
    }

    BOOL JavascriptProxy::HasOwnProperty(PropertyId propertyId)
    {
        // should never come here and it will be redirected to GetOwnPropertyDescriptor
        Assert(FALSE);
        PropertyDescriptor propertyDesc;
        return GetOwnPropertyDescriptor(this, propertyId, GetScriptContext(), &propertyDesc);
    }

    BOOL JavascriptProxy::HasOwnPropertyNoHostObject(PropertyId propertyId)
    {
        // the virtual method is for checking if globalobject has local property before we start initializing
        // we shouldn't trap??
        Assert(FALSE);
        return HasProperty(propertyId);
    }

    BOOL JavascriptProxy::HasOwnPropertyCheckNoRedecl(PropertyId propertyId)
    {
        // root object and activation object verification only; not needed.
        Assert(FALSE);
        return false;
    }

    BOOL JavascriptProxy::UseDynamicObjectForNoHostObjectAccess()
    {
        // heapenum check for CEO etc., and we don't want to access external method during enumeration. not applicable here.
        Assert(FALSE);
        return false;
    }

    DescriptorFlags JavascriptProxy::GetSetter(PropertyId propertyId, Var* setterValueOrProxy, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        // This is called when we walk prototype chain looking for setter. It is part of the [[set]] operation, but we don't need to restrict the
        // code to mimic the 'one step prototype chain lookup' spec letter. Current code structure is enough.
        *setterValueOrProxy = this;
        PropertyValueInfo::SetNoCache(info, this);
        PropertyValueInfo::DisablePrototypeCache(info, this); // We can't cache prototype property either
        return DescriptorFlags::Proxy;
    }

    // GetSetter is called for
    DescriptorFlags JavascriptProxy::GetSetter(JavascriptString* propertyNameString, Var* setterValueOrProxy, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        *setterValueOrProxy = this;
        PropertyValueInfo::SetNoCache(info, this);
        PropertyValueInfo::DisablePrototypeCache(info, this); // We can't cache prototype property either
        return DescriptorFlags::Proxy;
    }

    PropertyQueryFlags JavascriptProxy::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        // We can't cache the property at this time. both target and handler can be changed outside of the proxy, so the inline cache needs to be
        // invalidate when target, handler, or handler prototype has changed. We don't have a way to achieve this yet.
        // Also, Get and Has operations share a cache, so a trap on either should prevent caching on both.
        PropertyValueInfo::SetNoCache(info, this);
        PropertyValueInfo::DisablePrototypeCache(info, this); // We can't cache prototype property either
        auto fn = [&](RecyclableObject* object)-> BOOL {
            return JavascriptOperators::GetProperty(originalInstance, object, propertyId, value, requestContext, nullptr);
        };
        auto getPropertyId = [&]()->PropertyId {return propertyId; };
        PropertyDescriptor result;
        BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
        if (!foundProperty)
        {
            *value = requestContext->GetMissingPropertyResult();
        }
        else if (result.IsFromProxy())
        {
            *value = GetValueFromDescriptor(originalInstance, result, requestContext);
        }
        return JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
    }

    PropertyQueryFlags JavascriptProxy::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        // We can't cache the property at this time. both target and handler can be changed outside of the proxy, so the inline cache needs to be
        // invalidate when target, handler, or handler prototype has changed. We don't have a way to achieve this yet.
        PropertyValueInfo::SetNoCache(info, this);
        PropertyValueInfo::DisablePrototypeCache(info, this); // We can't cache prototype property either
        auto fn = [&](RecyclableObject* object)-> BOOL {
            return JavascriptOperators::GetPropertyWPCache<false /* OutputExistence */>(originalInstance, object, propertyNameString, value, requestContext, info);
        };
        auto getPropertyId = [&]()->PropertyId{
            const PropertyRecord* propertyRecord;
            requestContext->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
            return propertyRecord->GetPropertyId();
        };
        PropertyDescriptor result;
        BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
        if (!foundProperty)
        {
            *value = requestContext->GetMissingPropertyResult();
        }
        else if (result.IsFromProxy())
        {
            *value = GetValueFromDescriptor(originalInstance, result, requestContext);
        }
        return JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
    }

    BOOL JavascriptProxy::GetInternalProperty(Var instance, PropertyId internalPropertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (internalPropertyId == InternalPropertyIds::WeakMapKeyMap)
        {
            return __super::GetInternalProperty(instance, internalPropertyId, value, info, requestContext);
        }
        return FALSE;
    }

    _Check_return_ _Success_(return) BOOL JavascriptProxy::GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext* requestContext)
    {
        PropertyDescriptor result;
        if (getter != nullptr)
        {
            *getter = nullptr;
        }

        if (setter != nullptr)
        {
            *setter = nullptr;
        }

        BOOL foundProperty = GetOwnPropertyDescriptor(this, propertyId, requestContext, &result);
        if (foundProperty && result.IsFromProxy())
        {
            if (result.GetterSpecified() && getter != nullptr)
            {
                *getter = result.GetGetter();
            }
            if (result.SetterSpecified() && setter != nullptr)
            {
                *setter = result.GetSetter();
            }
            foundProperty = result.GetterSpecified() || result.SetterSpecified();
        }
        return foundProperty;
    }

    PropertyQueryFlags JavascriptProxy::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        // We can't cache the property at this time. both target and handler can be changed outside of the proxy, so the inline cache needs to be
        // invalidate when target, handler, or handler prototype has changed. We don't have a way to achieve this yet.
        PropertyValueInfo::SetNoCache(info, this);
        PropertyValueInfo::DisablePrototypeCache(info, this); // We can't cache prototype property either
        auto fn = [&](RecyclableObject* object)-> BOOL {
            return JavascriptOperators::GetPropertyReference(originalInstance, object, propertyId, value, requestContext, nullptr);
        };
        auto getPropertyId = [&]() -> PropertyId {return propertyId; };
        PropertyDescriptor result;
        BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
        if (!foundProperty)
        {
            *value = requestContext->GetMissingPropertyResult();
        }
        else if (result.IsFromProxy())
        {
            *value = GetValueFromDescriptor(originalInstance, result, requestContext);
        }
        return JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
    }

    BOOL JavascriptProxy::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

        // This is the second half of [[set]] where when the handler does not specified [[set]] so we forward to [[set]] on target
        // with receiver as the proxy.
        //c.Let existingDescriptor be the result of calling the[[GetOwnProperty]] internal method of Receiver with argument P.
        //d.ReturnIfAbrupt(existingDescriptor).
        //e.If existingDescriptor is not undefined, then
        //    i.Let valueDesc be the PropertyDescriptor{ [[Value]]: V }.
        //    ii.Return the result of calling the[[DefineOwnProperty]] internal method of Receiver with arguments P and valueDesc.
        //f.Else Receiver does not currently have a property P,
        //    i.Return the result of performing CreateDataProperty(Receiver, P, V).
        // We can't cache the property at this time. both target and handler can be changed outside of the proxy, so the inline cache needs to be
        // invalidate when target, handler, or handler prototype has changed. We don't have a way to achieve this yet.
        PropertyValueInfo::SetNoCache(info, this);
        PropertyValueInfo::DisablePrototypeCache(info, this); // We can't cache prototype property either

        PropertyDescriptor proxyPropertyDescriptor;

        ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
        ScriptContext* requestContext =
            threadContext->GetPreviousHostScriptContext()->GetScriptContext();

        // Set implicit call flag so we bailout and not do copy-prop on field

        Js::ImplicitCallFlags saveImplicitCallFlags = threadContext->GetImplicitCallFlags();
        threadContext->SetImplicitCallFlags((Js::ImplicitCallFlags)(saveImplicitCallFlags | ImplicitCall_Accessor));

        if (!JavascriptOperators::GetOwnPropertyDescriptor(this, propertyId, requestContext, &proxyPropertyDescriptor))
        {
            PropertyDescriptor resultDescriptor;
            resultDescriptor.SetConfigurable(true);
            resultDescriptor.SetWritable(true);
            resultDescriptor.SetEnumerable(true);
            resultDescriptor.SetValue(value);
            return Js::JavascriptOperators::DefineOwnPropertyDescriptor(this, propertyId, resultDescriptor, true, requestContext, flags);
        }
        else
        {
            // ES2017 Spec'd (9.1.9.1):
            // If existingDescriptor is not undefined, then
            //    If IsAccessorDescriptor(existingDescriptor) is true, return false.
            //    If existingDescriptor.[[Writable]] is false, return false.

            if (proxyPropertyDescriptor.IsAccessorDescriptor())
            {
                return FALSE;
            }

            if (proxyPropertyDescriptor.WritableSpecified() && !proxyPropertyDescriptor.IsWritable())
            {
                return FALSE;
            }

            proxyPropertyDescriptor.SetValue(value);
            proxyPropertyDescriptor.SetOriginal(nullptr);
            return Js::JavascriptOperators::DefineOwnPropertyDescriptor(this, propertyId, proxyPropertyDescriptor, true, requestContext, flags);
        }
    }

    BOOL JavascriptProxy::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        const PropertyRecord* propertyRecord;
        GetScriptContext()->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        return SetProperty(propertyRecord->GetPropertyId(), value, flags, info);
    }

    BOOL JavascriptProxy::SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        if (internalPropertyId == InternalPropertyIds::WeakMapKeyMap)
        {
            return __super::SetInternalProperty(internalPropertyId, value, flags, info);
        }
        return FALSE;
    }

    BOOL JavascriptProxy::InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        return SetProperty(propertyId, value, flags, info);
    }

    BOOL JavascriptProxy::EnsureProperty(PropertyId propertyId)
    {
        // proxy needs to be explicitly constructed. we don't have Ensure code path.
        Assert(FALSE);
        return false;
    }

    BOOL JavascriptProxy::EnsureNoRedeclProperty(PropertyId propertyId)
    {
        // proxy needs to be explicitly constructed. we don't have Ensure code path.
        Assert(FALSE);
        return false;
    }

    BOOL JavascriptProxy::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        // called from untrapped DefineProperty and from DOM side. I don't see this being used when the object is a proxy.
        Assert(FALSE);
        return false;
    }

    BOOL JavascriptProxy::InitPropertyScoped(PropertyId propertyId, Var value)
    {
        // proxy needs to be explicitly constructed. we don't have Ensure code path.
        Assert(FALSE);
        return false;
    }

    BOOL JavascriptProxy::InitFuncScoped(PropertyId propertyId, Var value)
    {
        // proxy needs to be explicitly constructed. we don't have Ensure code path.
        Assert(FALSE);
        return false;
    }

    BOOL JavascriptProxy::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
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
        ScriptContext* requestContext =
            threadContext->GetPreviousHostScriptContext()->GetScriptContext();

        //1. Assert: IsPropertyKey(P) is true.
        //2. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        RecyclableObject * handlerObj = this->MarshalHandler(requestContext);

        //3. If handler is null, then throw a TypeError exception.
        //6. ReturnIfAbrupt(trap).

        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, _u("deleteProperty"));
        }

        //4. Let target be the value of the[[ProxyTarget]] internal slot of O.
        RecyclableObject * targetObj = this->MarshalTarget(requestContext);

        //5. Let trap be the result of GetMethod(handler, "deleteProperty").
        JavascriptFunction* deleteMethod = GetMethodHelper(PropertyIds::deleteProperty, requestContext);

        //7. If trap is undefined, then
        //a.Return the result of calling the[[Delete]] internal method of target with argument P.
        Assert(!GetScriptContext()->IsHeapEnumInProgress());
        if (nullptr == deleteMethod)
        {
            uint32 indexVal;
            if (requestContext->IsNumericPropertyId(propertyId, &indexVal))
            {
                return targetObj->DeleteItem(indexVal, flags);
            }
            else
            {
                return targetObj->DeleteProperty(propertyId, flags);
            }
        }
        //8. Let trapResult be the result of calling the[[Call]] internal method of trap with handler as the this value and a new List containing target and P.
        //9. Let booleanTrapResult be ToBoolean(trapResult).
        //10. ReturnIfAbrupt(booleanTrapResult).
        //11. If booleanTrapResult is false, then return false.

        Var propertyName = GetName(requestContext, propertyId);

        Var deletePropertyResult = threadContext->ExecuteImplicitCall(deleteMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, deleteMethod, CallInfo(CallFlags_Value, 3), handlerObj, targetObj, propertyName);
        });

        BOOL trapResult = JavascriptConversion::ToBoolean(deletePropertyResult, requestContext);
        if (!trapResult)
        {
            if (flags & PropertyOperation_StrictMode)
            {
                JavascriptError::ThrowTypeErrorVar(
                    requestContext, 
                    JSERR_ProxyHandlerReturnedFalse, 
                    _u("deleteProperty"),
                    threadContext->GetPropertyName(propertyId)->GetBuffer()
                );
            }
            return trapResult;
        }

        //12. Let targetDesc be the result of calling the[[GetOwnProperty]] internal method of target with argument P.
        //13. ReturnIfAbrupt(targetDesc).
        //14. If targetDesc is undefined, then return true.
        //15. If targetDesc.[[Configurable]] is false, then throw a TypeError exception.
        //16. Return true.
        PropertyDescriptor targetPropertyDescriptor;
        if (!Js::JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propertyId, requestContext, &targetPropertyDescriptor))
        {
            return TRUE;
        }
        if (!targetPropertyDescriptor.IsConfigurable())
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("deleteProperty"));
        }
        return TRUE;
    }

    BOOL JavascriptProxy::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags)
    {
        PropertyRecord const *propertyRecord = nullptr;
        if (JavascriptOperators::ShouldTryDeleteProperty(this, propertyNameString, &propertyRecord))
        {
            Assert(propertyRecord);
            return DeleteProperty(propertyRecord->GetPropertyId(), flags);
        }

        return TRUE;
    }

#if ENABLE_FIXED_FIELDS
    BOOL JavascriptProxy::IsFixedProperty(PropertyId propertyId)
    {
        // TODO: can we add support for fixed property? don't see a clear way to invalidate...
        return false;
    }
#endif

    PropertyQueryFlags JavascriptProxy::HasItemQuery(uint32 index)
    {
        const PropertyRecord* propertyRecord;
        auto fn = [&](RecyclableObject* object)-> BOOL {
            return JavascriptOperators::HasItem(object, index);
        };
        auto getPropertyId = [&]() ->PropertyId {
            PropertyIdFromInt(index, &propertyRecord);
            return propertyRecord->GetPropertyId();
        };
        return JavascriptConversion::BooleanToPropertyQueryFlags(HasPropertyTrap(fn, getPropertyId));
    }

    BOOL JavascriptProxy::HasOwnItem(uint32 index)
    {
        const PropertyRecord* propertyRecord;
        auto fn = [&](RecyclableObject* object)-> BOOL {
            return JavascriptOperators::HasOwnItem(object, index);
        };
        auto getPropertyId = [&]() ->PropertyId {
            PropertyIdFromInt(index, &propertyRecord);
            return propertyRecord->GetPropertyId();
        };
        return HasPropertyTrap(fn, getPropertyId);
    }

    PropertyQueryFlags JavascriptProxy::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        const PropertyRecord* propertyRecord;
        auto fn = [&](RecyclableObject* object)-> BOOL {
            return JavascriptOperators::GetItem(originalInstance, object, index, value, requestContext);
        };
        auto getPropertyId = [&]() ->PropertyId {
            PropertyIdFromInt(index, &propertyRecord);
            return propertyRecord->GetPropertyId();
        };
        PropertyDescriptor result;
        BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
        if (!foundProperty)
        {
            *value = requestContext->GetMissingItemResult();
        }
        else if (result.IsFromProxy())
        {
            *value = GetValueFromDescriptor(originalInstance, result, requestContext);
        }
        return JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
    }

    PropertyQueryFlags JavascriptProxy::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        const PropertyRecord* propertyRecord;
        auto fn = [&](RecyclableObject* object)-> BOOL {
            return JavascriptOperators::GetItem(originalInstance, object, index, value, requestContext);
        };
        auto getPropertyId = [&]() ->PropertyId {
            PropertyIdFromInt(index, &propertyRecord);
            return propertyRecord->GetPropertyId();
        };
        PropertyDescriptor result;
        BOOL foundProperty = GetPropertyTrap(originalInstance, &result, fn, getPropertyId, requestContext);
        if (!foundProperty)
        {
            *value = requestContext->GetMissingItemResult();
        }
        else if (result.IsFromProxy())
        {
            *value = GetValueFromDescriptor(originalInstance, result, requestContext);
        }
        return JavascriptConversion::BooleanToPropertyQueryFlags(foundProperty);
    }

    DescriptorFlags JavascriptProxy::GetItemSetter(uint32 index, Var* setterValueOrProxy, ScriptContext* requestContext)
    {
        *setterValueOrProxy = this;
        return DescriptorFlags::Proxy;
    }

    BOOL JavascriptProxy::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        const PropertyRecord* propertyRecord;
        PropertyIdFromInt(index, &propertyRecord);
        return SetProperty(propertyRecord->GetPropertyId(), value, flags, nullptr);
    }

    BOOL JavascriptProxy::DeleteItem(uint32 index, PropertyOperationFlags flags)
    {
        const PropertyRecord* propertyRecord;
        PropertyIdFromInt(index, &propertyRecord);
        return DeleteProperty(propertyRecord->GetPropertyId(), flags);
    }

    // No change to foreign enumerator, just forward
    BOOL JavascriptProxy::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, EnumeratorCache * enumeratorCache)
    {
        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return FALSE;
        }

        // 1. Assert: Either Type(V) is Object or Type(V) is Null.
        // 2. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        // 3. If handler is null, then throw a TypeError exception.
        if (IsRevoked())
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("ownKeys"));
        }

        struct ProxyOwnkeysEnumerator : public JavascriptEnumerator
        {
            typedef JsUtil::BaseHashSet<JsUtil::CharacterBuffer<WCHAR>, Recycler> VisitedNamesHashSet;
            Field(VisitedNamesHashSet*) visited;
            Field(JavascriptArray*) trapResult;
            Field(JavascriptProxy*) proxy;
            FieldNoBarrier(ScriptContext*) scriptContext;
            Field(uint32) index;

            DEFINE_VTABLE_CTOR_ABSTRACT(ProxyOwnkeysEnumerator, JavascriptEnumerator)

            ProxyOwnkeysEnumerator(ScriptContext* scriptContext, JavascriptProxy* proxy, JavascriptArray* trapResult)
                :JavascriptEnumerator(scriptContext), scriptContext(scriptContext), proxy(proxy), trapResult(trapResult)
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
                uint32 len = trapResult->GetLength();
                while (index < len)
                {
                    Var var = trapResult->DirectGetItem(index++) ;
                    if (var)
                    {
                        // if (typeof key === "string") {
                        if (VarIs<JavascriptString>(var))
                        {
                            JavascriptString* propertyName = VarTo<JavascriptString>(var);
                            // let desc = Reflect.getOwnPropertyDescriptor(obj, key);
                            Js::PropertyDescriptor desc;
                            BOOL ret = JavascriptOperators::GetOwnPropertyDescriptor(proxy, propertyName, scriptContext, &desc);
                            const JsUtil::CharacterBuffer<WCHAR> propertyString(propertyName->GetString(), propertyName->GetLength());
                            // if (desc && !visited.has(key)) {
                            if (ret && !visited->Contains(propertyString))
                            {
                                visited->Add(propertyString);
                                // if (desc.enumerable) yield key;
                                if (desc.IsEnumerable())
                                {
                                    return VarTo<JavascriptString>(CrossSite::MarshalVar(
                                      scriptContext, propertyName, propertyName->GetScriptContext()));
                                }
                            }
                        }
                    }
                }
                return nullptr;
            }
        };

        JavascriptArray* trapResult = JavascriptOperators::GetOwnPropertyNames(this, requestContext);
        ProxyOwnkeysEnumerator* ownKeysEnum = RecyclerNew(requestContext->GetRecycler(), ProxyOwnkeysEnumerator, requestContext, this, trapResult);

        return enumerator->Initialize(ownKeysEnum, nullptr, nullptr, flags, requestContext, enumeratorCache);
    }

    BOOL JavascriptProxy::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        // should be for __definegetter style usage. need to wait for clear spec what it means.
        Assert(FALSE);
        return false;
    }

    BOOL JavascriptProxy::Equals(__in Var other, __out BOOL* value, ScriptContext* requestContext)
    {
        //RecyclableObject* targetObj;
        if (IsRevoked())
        {
            // the proxy has been revoked; TypeError.
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, _u("equal"));
        }
        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            *value = FALSE;
            return FALSE;
        }
        *value = (other == this);
        return true;
    }

    BOOL JavascriptProxy::StrictEquals(__in Var other, __out BOOL* value, ScriptContext* requestContext)
    {
        *value = FALSE;
        //RecyclableObject* targetObj;
        if (IsRevoked())
        {
            // the proxy has been revoked; TypeError.
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, _u("strict equal"));
        }
        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return FALSE;
        }
        *value = (other == this);
        return true;
    }

    BOOL JavascriptProxy::IsWritable(PropertyId propertyId)
    {
        PropertyDescriptor propertyDescriptor;
        if (!GetOwnPropertyDescriptor(this, propertyId, GetScriptContext(), &propertyDescriptor))
        {
            return FALSE;
        }

        // If property descriptor has getter/setter we should check if writable is specified before checking IsWritable
        return propertyDescriptor.WritableSpecified() ? propertyDescriptor.IsWritable() : FALSE;
    }

    BOOL JavascriptProxy::IsConfigurable(PropertyId propertyId)
    {
        Assert(FALSE);
        return target->IsConfigurable(propertyId);
    }

    BOOL JavascriptProxy::IsEnumerable(PropertyId propertyId)
    {
        Assert(FALSE);
        return target->IsEnumerable(propertyId);
    }

    BOOL JavascriptProxy::IsExtensible()
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
        ScriptContext* requestContext =
            threadContext->GetPreviousHostScriptContext()->GetScriptContext();

        //1. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        Js::RecyclableObject *handlerObj = this->MarshalHandler(requestContext);

        //2. If handler is null, then throw a TypeError exception.
        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("isExtensible"));
        }

        //3. Let target be the value of the[[ProxyTarget]] internal slot of O.
        Js::RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        //4. Let trap be the result of GetMethod(handler, "isExtensible").
        //5. ReturnIfAbrupt(trap).
        //6. If trap is undefined, then
        //a.Return the result of calling the[[IsExtensible]] internal method of target.
        //7. Let trapResult be the result of calling the[[Call]] internal method of trap with handler as the this value and a new List containing target.
        //8. Let booleanTrapResult be ToBoolean(trapResult).
        //9. ReturnIfAbrupt(booleanTrapResult).
        //10. Let targetResult be the result of calling the[[IsExtensible]] internal method of target.
        //11. ReturnIfAbrupt(targetResult).
        //12. If SameValue(booleanTrapResult, targetResult) is false, then throw a TypeError exception.
        //13. Return booleanTrapResult.
        JavascriptFunction* isExtensibleMethod = GetMethodHelper(PropertyIds::isExtensible, requestContext);
        Assert(!requestContext->IsHeapEnumInProgress());
        if (nullptr == isExtensibleMethod)
        {
            return targetObj->IsExtensible();
        }

        Var isExtensibleResult = threadContext->ExecuteImplicitCall(isExtensibleMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, isExtensibleMethod, CallInfo(CallFlags_Value, 2), handlerObj, targetObj);
        });

        BOOL trapResult = JavascriptConversion::ToBoolean(isExtensibleResult, requestContext);
        BOOL targetIsExtensible = targetObj->IsExtensible();
        if (trapResult != targetIsExtensible)
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("isExtensible"));
        }
        return trapResult;
    }

    BOOL JavascriptProxy::PreventExtensions()
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

        // Reject implicit call
        ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return  FALSE;
        }

        // Caller does not pass requestContext. Retrieve from host scriptContext stack.
        ScriptContext* requestContext =
            threadContext->GetPreviousHostScriptContext()->GetScriptContext();

        //1. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        Js::RecyclableObject *handlerObj = this->MarshalHandler(requestContext);

        //2. If handler is null, then throw a TypeError exception.
        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("preventExtensions"));
        }
        //3. Let target be the value of the[[ProxyTarget]] internal slot of O.
        Js::RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        //4. Let trap be the result of GetMethod(handler, "preventExtensions").
        //5. ReturnIfAbrupt(trap).
        //6. If trap is undefined, then
        //a.Return the result of calling the[[PreventExtensions]] internal method of target.
        //7. Let trapResult be the result of calling the[[Call]] internal method of trap with handler as the this value and a new List containing target.
        JavascriptFunction* preventExtensionsMethod = GetMethodHelper(PropertyIds::preventExtensions, requestContext);
        Assert(!GetScriptContext()->IsHeapEnumInProgress());
        if (nullptr == preventExtensionsMethod)
        {
            return targetObj->PreventExtensions();
        }

        //8. Let booleanTrapResult be ToBoolean(trapResult)
        //9. ReturnIfAbrupt(booleanTrapResult).
        //10. Let targetIsExtensible be the result of calling the[[IsExtensible]] internal method of target.
        //11. ReturnIfAbrupt(targetIsExtensible).
        //12. If booleanTrapResult is true and targetIsExtensible is true, then throw a TypeError exception.
        //13. Return booleanTrapResult.
        Var preventExtensionsResult = threadContext->ExecuteImplicitCall(preventExtensionsMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, preventExtensionsMethod, CallInfo(CallFlags_Value, 2), handlerObj, targetObj);
        });

        BOOL trapResult = JavascriptConversion::ToBoolean(preventExtensionsResult, requestContext);
        if (trapResult)
        {
            BOOL targetIsExtensible = targetObj->IsExtensible();
            if (targetIsExtensible)
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("preventExtensions"));
            }
        }
        return trapResult;
    }

    // 7.3.12 in ES 2015. While this should have been no observable behavior change. Till there is obvious change warrant this
    // to be moved to JavascriptOperators, let's keep it in proxy only first.
    BOOL JavascriptProxy::TestIntegrityLevel(IntegrityLevel integrityLevel, RecyclableObject* obj, ScriptContext* scriptContext)
    {
        //1. Assert: Type(O) is Object.
        //2. Assert: level is either "sealed" or "frozen".

        //3. Let status be IsExtensible(O).
        //4. ReturnIfAbrupt(status).
        //5. If status is true, then return false
        //6. NOTE If the object is extensible, none of its properties are examined.
        BOOL isExtensible = obj->IsExtensible();
        if (isExtensible)
        {
            return FALSE;
        }

        // at this time this is called from proxy only; when we extend this to other objects, we need to handle the other codepath.
        //7. Let keys be O.[[OwnPropertyKeys]]().
        //8. ReturnIfAbrupt(keys).
        Assert(VarIs<JavascriptProxy>(obj));
        JavascriptArray* resultArray = JavascriptOperators::GetOwnPropertyKeys(obj, scriptContext);

        //9. Repeat for each element k of keys,
        //      a. Let currentDesc be O.[[GetOwnProperty]](k).
        //      b. ReturnIfAbrupt(currentDesc).
        //      c. If currentDesc is not undefined, then
        //          i. If currentDesc.[[Configurable]] is true, return false.
        //          ii. If level is "frozen" and IsDataDescriptor(currentDesc) is true, then
        //              1. If currentDesc.[[Writable]] is true, return false.
        Var itemVar;
        bool writable = false;
        bool configurable = false;
        const PropertyRecord* propertyRecord;
        PropertyDescriptor propertyDescriptor;
        for (uint i = 0; i < resultArray->GetLength(); i++)
        {
            itemVar = resultArray->DirectGetItem(i);
            AssertMsg(VarIs<JavascriptSymbol>(itemVar) || VarIs<JavascriptString>(itemVar), "Invariant check during ownKeys proxy trap should make sure we only get property key here. (symbol or string primitives)");
            JavascriptConversion::ToPropertyKey(itemVar, scriptContext, &propertyRecord, nullptr);
            PropertyId propertyId = propertyRecord->GetPropertyId();
            if (JavascriptObject::GetOwnPropertyDescriptorHelper(obj, propertyId, scriptContext, propertyDescriptor))
            {
                configurable |= propertyDescriptor.IsConfigurable();
                if (propertyDescriptor.IsDataDescriptor())
                {
                    writable |= propertyDescriptor.IsWritable();
                }
            }
        }
        if (integrityLevel == IntegrityLevel::IntegrityLevel_frozen && writable)
        {
            return FALSE;
        }
        if (configurable)
        {
            return FALSE;
        }
        return TRUE;
    }

    BOOL JavascriptProxy::SetIntegrityLevel(IntegrityLevel integrityLevel, RecyclableObject* obj, ScriptContext* scriptContext)
    {
        //1. Assert: Type(O) is Object.
        //2. Assert : level is either "sealed" or "frozen".
        //3. Let status be O.[[PreventExtensions]]().
        //4. ReturnIfAbrupt(status).
        //5. If status is false, return false.

        // at this time this is called from proxy only; when we extend this to other objects, we need to handle the other codepath.
        Assert(VarIs<JavascriptProxy>(obj));
        if (obj->PreventExtensions() == FALSE)
            return FALSE;

        //6. Let keys be O.[[OwnPropertyKeys]]().
        //7. ReturnIfAbrupt(keys).
        JavascriptArray* resultArray = JavascriptOperators::GetOwnPropertyKeys(obj, scriptContext);

        const PropertyRecord* propertyRecord;
        if (integrityLevel == IntegrityLevel::IntegrityLevel_sealed)
        {
            //8. If level is "sealed", then
                //a. Repeat for each element k of keys,
                    //i. Let status be DefinePropertyOrThrow(O, k, PropertyDescriptor{ [[Configurable]]: false }).
                    //ii. ReturnIfAbrupt(status).
            PropertyDescriptor propertyDescriptor;
            propertyDescriptor.SetConfigurable(false);
            Var itemVar;
            for (uint i = 0; i < resultArray->GetLength(); i++)
            {
                itemVar = resultArray->DirectGetItem(i);
                AssertMsg(VarIs<JavascriptSymbol>(itemVar) || VarIs<JavascriptString>(itemVar), "Invariant check during ownKeys proxy trap should make sure we only get property key here. (symbol or string primitives)");
                JavascriptConversion::ToPropertyKey(itemVar, scriptContext, &propertyRecord, nullptr);
                PropertyId propertyId = propertyRecord->GetPropertyId();
                JavascriptObject::DefineOwnPropertyHelper(obj, propertyId, propertyDescriptor, scriptContext);
            }
        }
        else
        {
            //9.Else level is "frozen",
            //  a.Repeat for each element k of keys,
            //      i. Let currentDesc be O.[[GetOwnProperty]](k).
            //      ii. ReturnIfAbrupt(currentDesc).
            //      iii. If currentDesc is not undefined, then
            //          1. If IsAccessorDescriptor(currentDesc) is true, then
            //              a. Let desc be the PropertyDescriptor{[[Configurable]]: false}.
            //          2.Else,
            //              a. Let desc be the PropertyDescriptor { [[Configurable]]: false, [[Writable]]: false }.
            //          3. Let status be DefinePropertyOrThrow(O, k, desc).
            //          4. ReturnIfAbrupt(status).
            Assert(integrityLevel == IntegrityLevel::IntegrityLevel_frozen);
            PropertyDescriptor current, dataDescriptor, accessorDescriptor;
            dataDescriptor.SetConfigurable(false);
            dataDescriptor.SetWritable(false);
            accessorDescriptor.SetConfigurable(false);
            Var itemVar;
            for (uint i = 0; i < resultArray->GetLength(); i++)
            {
                itemVar = resultArray->DirectGetItem(i);
                AssertMsg(VarIs<JavascriptSymbol>(itemVar) || VarIs<JavascriptString>(itemVar), "Invariant check during ownKeys proxy trap should make sure we only get property key here. (symbol or string primitives)");
                JavascriptConversion::ToPropertyKey(itemVar, scriptContext, &propertyRecord, nullptr);
                PropertyId propertyId = propertyRecord->GetPropertyId();
                PropertyDescriptor propertyDescriptor;
                if (JavascriptObject::GetOwnPropertyDescriptorHelper(obj, propertyId, scriptContext, propertyDescriptor))
                {
                    if (propertyDescriptor.IsDataDescriptor())
                    {
                        JavascriptObject::DefineOwnPropertyHelper(obj, propertyRecord->GetPropertyId(), dataDescriptor, scriptContext);
                    }
                    else if (propertyDescriptor.IsAccessorDescriptor())
                    {
                        JavascriptObject::DefineOwnPropertyHelper(obj, propertyRecord->GetPropertyId(), accessorDescriptor, scriptContext);
                    }
                }
            }
        }

        // 10. Return true
        return TRUE;
    }

    BOOL JavascriptProxy::Seal()
    {
        return SetIntegrityLevel(IntegrityLevel::IntegrityLevel_sealed, this, this->GetScriptContext());
    }

    BOOL JavascriptProxy::Freeze()
    {
        return SetIntegrityLevel(IntegrityLevel::IntegrityLevel_frozen, this, this->GetScriptContext());
    }

    BOOL JavascriptProxy::IsSealed()
    {
        return TestIntegrityLevel(IntegrityLevel::IntegrityLevel_sealed, this, this->GetScriptContext());
    }

    BOOL JavascriptProxy::IsFrozen()
    {
        return TestIntegrityLevel(IntegrityLevel::IntegrityLevel_frozen, this, this->GetScriptContext());
    }

    BOOL JavascriptProxy::SetWritable(PropertyId propertyId, BOOL value)
    {
        Assert(FALSE);
        return FALSE;
    }

    BOOL JavascriptProxy::SetConfigurable(PropertyId propertyId, BOOL value)
    {
        Assert(FALSE);
        return FALSE;
    }

    BOOL JavascriptProxy::SetEnumerable(PropertyId propertyId, BOOL value)
    {
        Assert(FALSE);
        return FALSE;
    }

    BOOL JavascriptProxy::SetAttributes(PropertyId propertyId, PropertyAttributes attributes)
    {
        Assert(FALSE);
        return FALSE;
    }

    BOOL JavascriptProxy::HasInstance(Var instance, ScriptContext* scriptContext, IsInstInlineCache* inlineCache)
    {
        Var funcPrototype = JavascriptOperators::GetProperty(this, PropertyIds::prototype, scriptContext);
        return JavascriptFunction::HasInstance(funcPrototype, instance, scriptContext, NULL, NULL);
    }

    JavascriptString* JavascriptProxy::GetClassName(ScriptContext * requestContext)
    {
        Assert(FALSE);
        return nullptr;
    }

    RecyclableObject* JavascriptProxy::GetPrototypeSpecial()
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

        // Reject implicit call
        ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return nullptr;
        }

        // Caller does not pass requestContext. Retrieve from host scriptContext stack.
        ScriptContext* requestContext =
            threadContext->GetPreviousHostScriptContext()->GetScriptContext();

        Js::RecyclableObject *handlerObj = this->MarshalHandler(requestContext);
        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return nullptr;
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("getPrototypeOf"));
        }

        Js::RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        JavascriptFunction* getPrototypeOfMethod = GetMethodHelper(PropertyIds::getPrototypeOf, requestContext);

        if (nullptr == getPrototypeOfMethod || GetScriptContext()->IsHeapEnumInProgress())
        {
            return VarTo<RecyclableObject>(JavascriptObject::GetPrototypeOf(targetObj, requestContext));
        }

        Var getPrototypeOfResult = threadContext->ExecuteImplicitCall(getPrototypeOfMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, getPrototypeOfMethod, CallInfo(CallFlags_Value, 2), handlerObj, targetObj);
        });

        TypeId prototypeTypeId = JavascriptOperators::GetTypeId(getPrototypeOfResult);
        if (!JavascriptOperators::IsObjectType(prototypeTypeId) && prototypeTypeId != TypeIds_Null)
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("getPrototypeOf"));
        }
        if (!targetObj->IsExtensible() && !JavascriptConversion::SameValue(getPrototypeOfResult, targetObj->GetPrototype()))
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("getPrototypeOf"));
        }
        return VarTo<RecyclableObject>(getPrototypeOfResult);
    }

    RecyclableObject* JavascriptProxy::GetConfigurablePrototype(ScriptContext * requestContext)
    {
        // We should be using GetPrototypeSpecial for proxy object; never should come over here.
        Assert(FALSE);
        return nullptr;
    }

    void JavascriptProxy::RemoveFromPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated)
    {
        Assert(FALSE);
    }

    void JavascriptProxy::AddToPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated)
    {
        Assert(FALSE);
    }

    void JavascriptProxy::SetPrototype(RecyclableObject* newPrototype)
    {
        Assert(FALSE);
    }

    BOOL JavascriptProxy::SetPrototypeTrap(RecyclableObject* newPrototype, bool shouldThrow,
        ScriptContext * requestContext)
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);
        Assert(JavascriptOperators::IsObjectOrNull(newPrototype));

        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return FALSE;
        }

        //1. Assert: Either Type(V) is Object or Type(V) is Null.
        //2. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        Js::RecyclableObject *handlerObj = this->MarshalHandler(requestContext);

        //3. If handler is null, then throw a TypeError exception.
        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (shouldThrow)
            {
                if (!threadContext->RecordImplicitException())
                    return FALSE;
                JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("setPrototypeOf"));
            }
        }

        //4. Let target be the value of the[[ProxyTarget]] internal slot of O.
        Js::RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        //5. Let trap be the result of GetMethod(handler, "setPrototypeOf").
        //6. ReturnIfAbrupt(trap).
        //7. If trap is undefined, then
        //a.Return the result of calling the[[SetPrototypeOf]] internal method of target with argument V.
        JavascriptFunction* setPrototypeOfMethod = GetMethodHelper(PropertyIds::setPrototypeOf, requestContext);
        Assert(!GetScriptContext()->IsHeapEnumInProgress());
        if (nullptr == setPrototypeOfMethod)
        {
            JavascriptObject::ChangePrototype(targetObj, newPrototype, shouldThrow, requestContext);
            return TRUE;
        }
        //8. Let trapResult be the result of calling the[[Call]] internal method of trap with handler as the this value and a new List containing target and V.

        Var setPrototypeResult = threadContext->ExecuteImplicitCall(setPrototypeOfMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, setPrototypeOfMethod, CallInfo(CallFlags_Value, 3), handlerObj, targetObj, newPrototype);
        });

        //9. Let booleanTrapResult be ToBoolean(trapResult).
        //10. ReturnIfAbrupt(booleanTrapResult).
        //11. Let extensibleTarget be the result of IsExtensible(target).
        //12. ReturnIfAbrupt(extensibleTarget).
        //13. If extensibleTarget is true, then return booleanTrapResult.
        //14. Let targetProto be the result of calling the[[GetPrototypeOf]] internal method of target.
        //15. ReturnIfAbrupt(targetProto).
        //16. If booleanTrapResult is true and SameValue(V, targetProto) is false, then throw a TypeError exception.
        //17. Return booleanTrapResult.
        BOOL prototypeSetted = JavascriptConversion::ToBoolean(setPrototypeResult, requestContext);
        BOOL isExtensible = targetObj->IsExtensible();
        if (isExtensible)
        {
            if (!prototypeSetted && shouldThrow)
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_ProxyTrapReturnedFalse, _u("setPrototypeOf"));
            }
            return prototypeSetted;
        }
        Var targetProto = targetObj->GetPrototype();
        if (!JavascriptConversion::SameValue(targetProto, newPrototype))
        {
            if (shouldThrow)
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("setPrototypeOf"));
            }
            return FALSE;
        }
        return TRUE;
    }


    Var JavascriptProxy::ToString(ScriptContext* scriptContext)
    {
        //RecyclableObject* targetObj;
        if (IsRevoked())
        {
            ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return nullptr;
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("toString"));
        }
        return JavascriptObject::ToStringHelper(target, scriptContext);
    }

    // before recursively calling something on 'target' use this helper in case there is nesting of proxies.
    // the proxies could be deep nested and cause SO when processed recursively.
    const JavascriptProxy* JavascriptProxy::UnwrapNestedProxies(const JavascriptProxy* proxy)
    {
        // continue while we have a proxy that is not revoked
        while (!proxy->IsRevoked())
        {
            JavascriptProxy* nestedProxy = JavascriptOperators::TryFromVar<JavascriptProxy>(proxy->target);
            if (nestedProxy == nullptr)
            {
                break;
            }

            proxy = nestedProxy;
        }

        return proxy;
    }

    BOOL JavascriptProxy::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        const JavascriptProxy* proxy = UnwrapNestedProxies(this);

        //RecyclableObject* targetObj;
        if (proxy->IsRevoked())
        {
            ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("getTypeString"));
        }
        return proxy->target->GetDiagTypeString(stringBuilder, requestContext);
    }

    RecyclableObject* JavascriptProxy::ToObject(ScriptContext * requestContext)
    {
        //RecyclableObject* targetObj;
        if (IsRevoked())
        {
            ThreadContext* threadContext = GetScriptContext()->GetThreadContext();
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return nullptr;
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("toObject"));
        }
        return __super::ToObject(requestContext);
    }

    Var JavascriptProxy::GetTypeOfString(ScriptContext* requestContext)
    {
        const JavascriptProxy* proxy = UnwrapNestedProxies(this);

        if (proxy->handler == nullptr)
        {
            // even if handler is nullptr, return typeof as "object"
            return requestContext->GetLibrary()->GetObjectTypeDisplayString();
        }
        // if exotic object has [[Call]] we should return "function", otherwise return "object"
        if (VarIs<JavascriptFunction>(this->target))
        {
            return requestContext->GetLibrary()->GetFunctionTypeDisplayString();
        }
        else
        {
            // handle nested cases recursively
            return proxy->target->GetTypeOfString(requestContext);
        }
    }

    BOOL JavascriptProxy::GetOwnPropertyDescriptor(RecyclableObject* obj, PropertyId propertyId, ScriptContext* requestContext, PropertyDescriptor* propertyDescriptor)
    {
        JavascriptProxy* proxy = VarTo<JavascriptProxy>(obj);
        return proxy->GetPropertyDescriptorTrap(propertyId, propertyDescriptor, requestContext);
    }


    BOOL JavascriptProxy::DefineOwnPropertyDescriptor(RecyclableObject* obj, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* requestContext, PropertyOperationFlags flags)
    {
        // #sec-proxy-object-internal-methods-and-internal-slots-defineownproperty-p-desc
        PROBE_STACK(requestContext, Js::Constants::MinStackDefault);

        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return FALSE;
        }

        JavascriptProxy* proxy = VarTo<JavascriptProxy>(obj);

        //1. Assert: IsPropertyKey(P) is true.
        //2. Let handler be O.[[ProxyHandler]].
        RecyclableObject *handlerObj = proxy->MarshalHandler(requestContext);

        //3. If handler is null, then throw a TypeError exception.
        //4. Assert: Type(handler) is Object.
        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, _u("definePropertyDescriptor"));
        }

        //5. Let target be O.[[ProxyTarget]].
        RecyclableObject *targetObj = proxy->MarshalTarget(requestContext);

        //6. Let trap be ? GetMethod(handler, "defineProperty").
        //7. If trap is undefined, then
        //a. Return ? target.[[DefineOwnProperty]](P, Desc).
        JavascriptFunction* defineOwnPropertyMethod = proxy->GetMethodHelper(PropertyIds::defineProperty, requestContext);

        Assert(!requestContext->IsHeapEnumInProgress());
        if (nullptr == defineOwnPropertyMethod)
        {
            return JavascriptOperators::DefineOwnPropertyDescriptor(targetObj, propId, descriptor, throwOnError, requestContext, flags);
        }

        //8. Let descObj be FromPropertyDescriptor(Desc).
        //9. Let booleanTrapResult be ToBoolean(? Call(trap, handler, << target, P, descObj >> )).
        //10. If booleanTrapResult is false, then return false.
        //11. Let targetDesc be ? target.[[GetOwnProperty]](P).
        Var descVar = JavascriptOperators::FromPropertyDescriptor(descriptor, requestContext);

        Var propertyName = GetName(requestContext, propId);

        Var definePropertyResult = threadContext->ExecuteImplicitCall(defineOwnPropertyMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, defineOwnPropertyMethod, CallInfo(CallFlags_Value, 4), handlerObj, targetObj, propertyName, descVar);
        });

        BOOL defineResult = JavascriptConversion::ToBoolean(definePropertyResult, requestContext);
        if (!defineResult)
        {
            if (throwOnError && flags & PropertyOperation_StrictMode)
            {
                JavascriptError::ThrowTypeErrorVar(
                    requestContext,
                    JSERR_ProxyHandlerReturnedFalse,
                    _u("defineProperty"),
                    requestContext->GetPropertyName(propId)->GetBuffer()
                );
            }
            return defineResult;
        }

        //12. Let extensibleTarget be ? IsExtensible(target).
        //13. If Desc has a[[Configurable]] field and if Desc.[[Configurable]] is false, then
        //    a.Let settingConfigFalse be true.
        //14. Else let settingConfigFalse be false.
        //15. If targetDesc is undefined, then
        //    a.If extensibleTarget is false, then throw a TypeError exception.
        //    b.If settingConfigFalse is true, then throw a TypeError exception.
        //16. Else targetDesc is not undefined,
        //    a.If IsCompatiblePropertyDescriptor(extensibleTarget, Desc, targetDesc) is false, then throw a TypeError exception.
        //    b.If settingConfigFalse is true and targetDesc.[[Configurable]] is true, then throw a TypeError exception.
        //17. Return true.
        PropertyDescriptor targetDescriptor;
        BOOL hasProperty = JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propId, requestContext, &targetDescriptor);
        BOOL isExtensible = targetObj->IsExtensible();
        BOOL settingConfigFalse = (descriptor.ConfigurableSpecified() && !descriptor.IsConfigurable());
        if (!hasProperty)
        {
            if (!isExtensible || settingConfigFalse)
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("defineProperty"));
            }
        }
        else
        {
            if (!JavascriptOperators::IsCompatiblePropertyDescriptor(descriptor, hasProperty? &targetDescriptor : nullptr, !!isExtensible, true, requestContext))
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("defineProperty"));
            }
            if (settingConfigFalse && targetDescriptor.IsConfigurable())
            {
                JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("defineProperty"));
            }
        }
        return TRUE;
    }


    BOOL JavascriptProxy::SetPropertyTrap(Var receiver, SetPropertyTrapKind setPropertyTrapKind, Js::JavascriptString * propertyNameString, Var newValue, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags)
    {
        const PropertyRecord* propertyRecord;
        requestContext->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        return SetPropertyTrap(receiver, setPropertyTrapKind, propertyRecord->GetPropertyId(), newValue, requestContext, propertyOperationFlags);

    }

    BOOL JavascriptProxy::SetPropertyTrap(Var receiver, SetPropertyTrapKind setPropertyTrapKind, PropertyId propertyId, Var newValue, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags, BOOL skipPrototypeCheck)
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return FALSE;
        }

        //1. Assert: IsPropertyKey(P) is true.
        //2. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        Js::RecyclableObject *handlerObj = this->MarshalHandler(requestContext);

        //3. If handler is null, then throw a TypeError exception.
        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return FALSE;
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, _u("set"));
        }

        //4. Let target be the value of the[[ProxyTarget]] internal slot of O.
        Js::RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        //5. Let trap be the result of GetMethod(handler, "set").
        //6. ReturnIfAbrupt(trap).
        //7. If trap is undefined, then
        //a.Return the result of calling the[[Set]] internal method of target with arguments P, V, and Receiver.
        JavascriptFunction* setMethod = GetMethodHelper(PropertyIds::set, requestContext);

        Assert(!GetScriptContext()->IsHeapEnumInProgress());
        if (nullptr == setMethod)
        {
            PropertyValueInfo info;
            switch (setPropertyTrapKind)
            {
            case SetPropertyTrapKind::SetItemOnTaggedNumberKind:
            {
                uint32 indexVal;
                BOOL isNumericPropertyId = requestContext->IsNumericPropertyId(propertyId, &indexVal);
                Assert(isNumericPropertyId);
                return JavascriptOperators::SetItemOnTaggedNumber(receiver, targetObj, indexVal, newValue, requestContext, propertyOperationFlags);
            }
            case SetPropertyTrapKind::SetPropertyOnTaggedNumberKind:
                return JavascriptOperators::SetPropertyOnTaggedNumber(receiver, targetObj, propertyId, newValue, requestContext, propertyOperationFlags);
            case SetPropertyTrapKind::SetPropertyKind:
                return JavascriptOperators::SetProperty(receiver, targetObj, propertyId, newValue, requestContext, propertyOperationFlags);
            case SetPropertyTrapKind::SetItemKind:
            {
                uint32 indexVal;
                BOOL isNumericPropertyId = requestContext->IsNumericPropertyId(propertyId, &indexVal);
                Assert(isNumericPropertyId);
                return  JavascriptOperators::SetItem(receiver, targetObj, indexVal, newValue, requestContext, propertyOperationFlags, skipPrototypeCheck);
            }
            case SetPropertyTrapKind::SetPropertyWPCacheKind:
            {
                PropertyValueInfo propertyValueInfo;
                return JavascriptOperators::SetPropertyWPCache(receiver, targetObj, propertyId, newValue, requestContext, propertyOperationFlags, &propertyValueInfo);
            }
            default:
                AnalysisAssert(FALSE);
            }
        }
        //8. Let trapResult be the result of calling the[[Call]] internal method of trap with handler as the this value and a new List containing target, P, V, and Receiver.
        //9. Let booleanTrapResult be ToBoolean(trapResult).
        //10. ReturnIfAbrupt(booleanTrapResult).
        //11. If booleanTrapResult is false, then return false.

        Var propertyName = GetName(requestContext, propertyId);

        Var setPropertyResult = threadContext->ExecuteImplicitCall(setMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, setMethod, CallInfo(CallFlags_Value, 5), handlerObj, targetObj, propertyName, newValue, receiver);
        });

        BOOL setResult = JavascriptConversion::ToBoolean(setPropertyResult, requestContext);
        if (!setResult)
        {
            if (propertyOperationFlags & PropertyOperation_StrictMode)
            {
                JavascriptError::ThrowTypeErrorVar(
                    requestContext,
                    JSERR_ProxyHandlerReturnedFalse,
                    _u("set"),
                    requestContext->GetPropertyName(propertyId)->GetBuffer()
                );
            }
            return setResult;
        }

        //12. Let targetDesc be the result of calling the[[GetOwnProperty]] internal method of target with argument P.
        //13. ReturnIfAbrupt(targetDesc).
        //14. If targetDesc is not undefined, then
        //a.If IsDataDescriptor(targetDesc) and targetDesc.[[Configurable]] is false and targetDesc.[[Writable]] is false, then
        //i.If SameValue(V, targetDesc.[[Value]]) is false, then throw a TypeError exception.
        //b.If IsAccessorDescriptor(targetDesc) and targetDesc.[[Configurable]] is false, then
        //i.If targetDesc.[[Set]] is undefined, then throw a TypeError exception.
        //15. Return true
        PropertyDescriptor targetDescriptor;
        BOOL hasProperty;

        hasProperty = JavascriptOperators::GetOwnPropertyDescriptor(targetObj, propertyId, requestContext, &targetDescriptor);
        if (hasProperty)
        {
            if (targetDescriptor.ValueSpecified())
            {
                if (!targetDescriptor.IsConfigurable() && !targetDescriptor.IsWritable() &&
                    !JavascriptConversion::SameValue(newValue, targetDescriptor.GetValue()))
                {
                    JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("set"));
                }
            }
            else
            {
                if (!targetDescriptor.IsConfigurable() && targetDescriptor.GetSetter() == requestContext->GetLibrary()->GetDefaultAccessorFunction())
                {
                    JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("set"));
                }
            }
        }

        threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);

        return TRUE;

    }

    JavascriptFunction* JavascriptProxy::GetMethodHelper(PropertyId methodId, ScriptContext* requestContext)
    {
        //2. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        //3. If handler is null, then throw a TypeError exception.
        if (IsRevoked())
        {
            // the proxy has been revoked; TypeError.
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnRevokedProxy, requestContext->GetPropertyName(methodId)->GetBuffer());
        }
        Var varMethod;
        //5. Let trap be the result of GetMethod(handler, "getOwnPropertyDescriptor").
        //6. ReturnIfAbrupt(trap).

        //7.3.9 GetMethod(V, P)
        //  The abstract operation GetMethod is used to get the value of a specific property of an ECMAScript language value when the value of the
        //  property is expected to be a function. The operation is called with arguments V and P where V is the ECMAScript language value, P is the
        //  property key. This abstract operation performs the following steps:
        //  1. Assert: IsPropertyKey(P) is true.
        //  2. Let func be ? GetV(V, P).
        //  3. If func is either undefined or null, return undefined.
        //  4. If IsCallable(func) is false, throw a TypeError exception.
        //  5. Return func.
        BOOL result = JavascriptOperators::GetPropertyReference(handler, methodId, &varMethod, requestContext);
        if (!result || JavascriptOperators::IsUndefinedOrNull(varMethod))
        {
            return nullptr;
        }
        if (!VarIs<JavascriptFunction>(varMethod))
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_NeedFunction, requestContext->GetPropertyName(methodId)->GetBuffer());
        }

        JavascriptFunction* function = VarTo<JavascriptFunction>(varMethod);

        return VarTo<JavascriptFunction>(CrossSite::MarshalVar(requestContext,
          function, function->GetScriptContext()));
    }

    Var JavascriptProxy::GetValueFromDescriptor(Var instance, PropertyDescriptor propertyDescriptor, ScriptContext* requestContext)
    {
        if (propertyDescriptor.ValueSpecified())
        {
            return CrossSite::MarshalVar(requestContext, propertyDescriptor.GetValue());
        }
        if (propertyDescriptor.GetterSpecified())
        {
            return JavascriptOperators::CallGetter(VarTo<RecyclableObject>(propertyDescriptor.GetGetter()), instance, requestContext);
        }
        Assert(FALSE);
        return requestContext->GetLibrary()->GetUndefined();
    }

    void JavascriptProxy::PropertyIdFromInt(uint32 index, PropertyRecord const** propertyRecord)
    {
        char16 buffer[22];
        int pos = TaggedInt::ToBuffer(index, buffer, _countof(buffer));

        GetScriptContext()->GetOrAddPropertyRecord((LPCWSTR)buffer + pos, (_countof(buffer) - 1) - pos, propertyRecord);
    }

    Var JavascriptProxy::GetName(ScriptContext* requestContext, PropertyId propertyId)
    {
        const PropertyRecord* propertyRecord = requestContext->GetThreadContext()->GetPropertyName(propertyId);
        Var name;
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

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    PropertyId JavascriptProxy::EnsureHandlerPropertyId(ScriptContext* scriptContext)
    {
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if (threadContext->handlerPropertyId == Js::Constants::NoProperty)
        {
            LPCWSTR autoProxyName;

            if (threadContext->GetAutoProxyName() != nullptr)
            {
                autoProxyName = threadContext->GetAutoProxyName();
            }
            else
            {
                autoProxyName = Js::Configuration::Global.flags.autoProxy;
            }

            threadContext->handlerPropertyId = threadContext->GetOrAddPropertyRecordBind(
                JsUtil::CharacterBuffer<WCHAR>(autoProxyName, static_cast<charcount_t>(wcslen(autoProxyName))))->GetPropertyId();
        }
        return threadContext->handlerPropertyId;
    }

    RecyclableObject* JavascriptProxy::AutoProxyWrapper(Var obj)
    {
        RecyclableObject* object = VarTo<RecyclableObject>(obj);
        if (!JavascriptOperators::IsObject(object) || VarIs<JavascriptProxy>(object))
        {
            return object;
        }
        ScriptContext* scriptContext = object->GetScriptContext();
        if (!scriptContext->GetThreadContext()->IsScriptActive())
        {
            return object;
        }
        if (!scriptContext->GetConfig()->IsES6ProxyEnabled())
        {
            return object;
        }
        Assert(Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag));
        PropertyId handlerId = EnsureHandlerPropertyId(scriptContext);
        GlobalObject* globalObject = scriptContext->GetLibrary()->GetGlobalObject();
        Var handler = nullptr;
        if (!JavascriptOperators::GetProperty(globalObject, handlerId, &handler, scriptContext))
        {
            handler = scriptContext->GetLibrary()->CreateObject();
            JavascriptOperators::SetProperty(globalObject, globalObject, handlerId, handler, scriptContext);
        }
        CallInfo callInfo(CallFlags_Value, 3);
        Var varArgs[3];
        Js::Arguments arguments(callInfo, varArgs);
        varArgs[0] = scriptContext->GetLibrary()->GetProxyConstructor();
        varArgs[1] = object;
        varArgs[2] = handler;
        return Create(scriptContext, arguments);
    }
#endif

    Var JavascriptProxy::ConstructorTrap(Arguments args, ScriptContext* scriptContext, const Js::AuxArray<uint32> *spreadIndices)
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

        Var functionResult;
        if (spreadIndices != nullptr)
        {
            functionResult = JavascriptFunction::CallSpreadFunction(this, args, spreadIndices);
        }
        else
        {
            functionResult = JavascriptFunction::CallFunction<true>(this, this->GetEntryPoint(), args);
        }
        return functionResult;
    }

    Var JavascriptProxy::FunctionCallTrap(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        BOOL hasOverridingNewTarget = args.HasNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);
        bool isNewCall = args.IsNewCall() || hasOverridingNewTarget;

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        if (!VarIs<JavascriptProxy>(function))
        {
            if (args.Info.Flags & CallFlags_New)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction, _u("construct"));
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction, _u("call"));
            }
        }

        Var newTarget = nullptr;
        JavascriptProxy* proxy = VarTo<JavascriptProxy>(function);
        Js::RecyclableObject *handlerObj = proxy->handler;
        Js::RecyclableObject *targetObj = proxy->target;

        JavascriptFunction* callMethod;
        Assert(!scriptContext->IsHeapEnumInProgress());

        // To conform with ES6 spec 7.3.13
        if (hasOverridingNewTarget)
        {
            newTarget = args.Values[callInfo.Count];
        }
        else
        {
            newTarget = proxy;
        }

        if (args.Info.Flags & CallFlags_New)
        {
            callMethod = proxy->GetMethodHelper(PropertyIds::construct, scriptContext);
        }
        else
        {
            callMethod = proxy->GetMethodHelper(PropertyIds::apply, scriptContext);
        }
        if (!JavascriptConversion::IsCallable(targetObj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction, _u("call"));
        }

        if (nullptr == callMethod)
        {
            // newCount is ushort. If args count is greater than or equal to 65535, an integer
            // too many arguments
            if (args.Info.Count >= USHORT_MAX) //check against CallInfo::kMaxCountArgs if newCount is ever made int
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgListTooLarge);
            }

            // in [[construct]] case, we don't need to check if the function is a constructor: the function should throw there.
            Var newThisObject = nullptr;
            if (args.Info.Flags & CallFlags_New)
            {
                if (!JavascriptOperators::IsConstructor(targetObj))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedFunction, _u("construct"));
                }

                // args.Values[0] will be null in the case where NewTarget is initially provided by proxy.
                if (!isCtorSuperCall || !args.Values[0])
                {
                    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                    {
                        newThisObject = JavascriptOperators::NewScObjectNoCtorCommon(proxy, scriptContext, false);
                    }
                    END_SAFE_REENTRANT_CALL
                    args.Values[0] = newThisObject;
                }
                else
                {
                    newThisObject = args.Values[0];
                }
            }

            ushort newCount = (ushort)args.Info.Count;
            if (isNewCall)
            {
                newCount++;
                if (!newCount)
                {
                    ::Math::DefaultOverflowPolicy();
                }
            }
            AnalysisAssert(newCount >= (ushort)args.Info.Count);

            Var* newValues;
            const unsigned STACK_ARGS_ALLOCA_THRESHOLD = 8; // Number of stack args we allow before using _alloca
            Var stackArgs[STACK_ARGS_ALLOCA_THRESHOLD];
            if (newCount > STACK_ARGS_ALLOCA_THRESHOLD)
            {
                PROBE_STACK(scriptContext, newCount * sizeof(Var) + Js::Constants::MinStackDefault); // args + function call
                newValues = (Var*)_alloca(newCount * sizeof(Var));
            }
            else
            {
                newValues = stackArgs;
            }

            CallInfo calleeInfo((CallFlags)(args.Info.Flags), args.Info.Count);
            if (isNewCall)
            {
                calleeInfo.Flags = (CallFlags)(calleeInfo.Flags | CallFlags_ExtraArg | CallFlags_NewTarget);
            }

            for (ushort argCount = 0; argCount < (ushort)args.Info.Count; argCount++)
            {
                AnalysisAssert(newCount >= ((ushort)args.Info.Count));
                AnalysisAssert(argCount < newCount);
                AnalysisAssert(argCount < (ushort)args.Info.Count);
                AnalysisAssert(sizeof(Var*) == sizeof(void*));
                AnalysisAssert(sizeof(Var*) * argCount < sizeof(void*) * newCount);
#pragma prefast(suppress:__WARNING_WRITE_OVERRUN, "This is a false positive, and all of the above analysis asserts still didn't convince prefast of that.")
                newValues[argCount] = args.Values[argCount];
            }
            if (isNewCall)
            {
                AnalysisAssert(newCount == ((ushort)args.Info.Count) + 1);
                newValues[args.Info.Count] = newTarget;

                // If the function we're calling is a class constructor, it expects newTarget
                // as the first arg so it knows how to construct "this". (We can leave the
                // extra copy of newTarget at the end of the arguments; it's harmless so
                // there's no need to introduce additional complexity here.)
                FunctionInfo* functionInfo = JavascriptOperators::GetConstructorFunctionInfo(targetObj, scriptContext);
                if (functionInfo && functionInfo->IsClassConstructor())
                {
                    newValues[0] = newTarget;
                }
            }

            Js::Arguments arguments(calleeInfo, newValues);
            Var aReturnValue = nullptr;
            BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
            {
                aReturnValue = JavascriptFunction::CallFunction<true>(targetObj, targetObj->GetEntryPoint(), arguments);
            }
            END_SAFE_REENTRANT_CALL
            // If this is constructor call, return the actual object instead of function result
            if ((callInfo.Flags & CallFlags_New) && !JavascriptOperators::IsObject(aReturnValue))
            {
                aReturnValue = newThisObject;
            }
            return aReturnValue;
        }

        JavascriptArray* argList = scriptContext->GetLibrary()->CreateArray(callInfo.Count - 1);
        for (uint i = 1; i < callInfo.Count; i++)
        {
            argList->DirectSetItemAt(i - 1, args[i]);
        }

        Var varArgs[4];
        CallInfo calleeInfo(CallFlags_Value, 4);
        Js::Arguments arguments(calleeInfo, varArgs);

        varArgs[0] = handlerObj;
        varArgs[1] = targetObj;
        if (args.Info.Flags & CallFlags_New)
        {
            if (!JavascriptOperators::IsConstructor(targetObj))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NotAConstructor);
            }

            varArgs[2] = argList;
            // 1st preference - overridden newTarget
            // 2nd preference - 'this' in case of super() call
            // 3rd preference - newTarget ( which is same as F)
            varArgs[3] = hasOverridingNewTarget ? newTarget :
                isCtorSuperCall ? args[0] : newTarget;
         }
        else
        {
            varArgs[2] = args[0];
            varArgs[3] = argList;
        }

        Var trapResult = nullptr;
        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            trapResult = callMethod->CallFunction(arguments);
        }
        END_SAFE_REENTRANT_CALL
        if (args.Info.Flags & CallFlags_New)
        {
            if (!Js::JavascriptOperators::IsObject(trapResult))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_InconsistentTrapResult, _u("construct"));
            }
        }
        return trapResult;
    }

    JavascriptArray* JavascriptProxy::PropertyKeysTrap(KeysTrapKind keysTrapKind, ScriptContext* requestContext)
    {
        PROBE_STACK(GetScriptContext(), Js::Constants::MinStackDefault);

        // Reject implicit call
        ThreadContext* threadContext = requestContext->GetThreadContext();
        if (threadContext->IsDisableImplicitCall())
        {
            threadContext->AddImplicitCallFlags(Js::ImplicitCall_External);
            return nullptr;
        }

        //1. Let handler be the value of the[[ProxyHandler]] internal slot of O.
        RecyclableObject *handlerObj = this->MarshalHandler(requestContext);

        //2. If handler is null, throw a TypeError exception.
        //3. Assert: Type(handler) is Object.
        if (handlerObj == nullptr)
        {
            // the proxy has been revoked; TypeError.
            if (!threadContext->RecordImplicitException())
                return nullptr;
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_ErrorOnRevokedProxy, _u("ownKeys"));
        }
        AssertMsg(JavascriptOperators::IsObject(handlerObj), "Handler should be object.");

        //4. Let target be the value of the[[ProxyTarget]] internal slot of O.
        RecyclableObject *targetObj = this->MarshalTarget(requestContext);

        //5. Let trap be GetMethod(handler, "ownKeys").
        //6. ReturnIfAbrupt(trap).
        //7. If trap is undefined, then
        //      a. Return target.[[OwnPropertyKeys]]().
        JavascriptFunction* ownKeysMethod = GetMethodHelper(PropertyIds::ownKeys, requestContext);
        Assert(!GetScriptContext()->IsHeapEnumInProgress());

        JavascriptArray *targetKeys;

        if (nullptr == ownKeysMethod)
        {
            switch (keysTrapKind)
            {
                case GetOwnPropertyNamesKind:
                    targetKeys = JavascriptOperators::GetOwnPropertyNames(targetObj, requestContext);
                    break;
                case GetOwnPropertySymbolKind:
                    targetKeys = JavascriptOperators::GetOwnPropertySymbols(targetObj, requestContext);
                    break;
                case KeysKind:
                    targetKeys = JavascriptOperators::GetOwnPropertyKeys(targetObj, requestContext);
                    break;
                default:
                    AssertMsg(false, "Invalid KeysTrapKind.");
                    return requestContext->GetLibrary()->CreateArray(0);
            }
            return targetKeys;
        }

        //8. Let trapResultArray be Call(trap, handler, <<target>>).
        //9. Let trapResult be CreateListFromArrayLike(trapResultArray, <<String, Symbol>>).
        //10. ReturnIfAbrupt(trapResult).
        //11. Let extensibleTarget be IsExtensible(target).
        //12. ReturnIfAbrupt(extensibleTarget).
        //13. Let targetKeys be target.[[OwnPropertyKeys]]().
        //14. ReturnIfAbrupt(targetKeys).

        Var ownKeysResult = threadContext->ExecuteImplicitCall(ownKeysMethod, ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, ownKeysMethod, CallInfo(CallFlags_Value, 2), handlerObj, targetObj);
        });

        if (!JavascriptOperators::IsObject(ownKeysResult))
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
        }
        RecyclableObject* trapResultArray = VarTo<RecyclableObject>(ownKeysResult);

        //15. Assert: targetKeys is a List containing only String and Symbol values.
        //16. Let targetConfigurableKeys be an empty List.
        //17. Let targetNonconfigurableKeys be an empty List.
        //18. Repeat, for each element key of targetKeys,
        //    a.Let desc be target.[[GetOwnProperty]](key).
        //    b.ReturnIfAbrupt(desc).
        //    c.If desc is not undefined and desc.[[Configurable]] is false, then
        //         i.Append key as an element of targetNonconfigurableKeys.
        //    d.Else,
        //         i.Append key as an element of targetConfigurableKeys.
        //19. If extensibleTarget is true and targetNonconfigurableKeys is empty, then
        //    a. Return trapResult.
        //20. Let uncheckedResultKeys be a new List which is a copy of trapResult.
        //21. Repeat, for each key that is an element of targetNonconfigurableKeys,
        //      a. If key is not an element of uncheckedResultKeys, throw a TypeError exception.
        //      b. Remove key from uncheckedResultKeys
        //22. If extensibleTarget is true, return trapResult.

        /*
        To avoid creating targetConfigurableKeys, targetNonconfigurableKeys and uncheckedResultKeys list in above steps,
        use below algorithm to accomplish same behavior

        // Track if there are any properties that are present in target but not present in trap result
        for(var i = 0; i < trapResult.length; i++)
        {
            PropertyId propId = GetPropertyId(trapResult[i]);
            if(propId != NoProperty) { targetToTrapResultMap[propId] = 1; }
            else { isTrapResultMissingFromTargetKeys = true; }
        }

        isConfigurableKeyMissingFromTrapResult = false;
        isNonconfigurableKeyMissingFromTrapResult = false;
        for(var i = 0; i < targetKeys.length; i++)
        {
            PropertyId propId = GetPropertyId(targetKeys[i]);
            Var desc = GetPropertyDescriptor(propId);
            if(targetToTrapResultMap[propId]) {
                delete targetToTrapResultMap[propId];
                isMissingFromTrapResult = false;
            } else {
                isMissingFromTrapResult = true;
            }
            if(desc->IsConfigurable()) {
                if(isMissingFromTrapResult) {
                    isConfigurableKeyMissingFromTrapResult = true;
                }
            } else {
                isAnyNonconfigurableKeyPresent = true
                if(isMissingFromTrapResult) {
                    isNonconfigurableKeyMissingFromTrapResult = true;
                }
            }
        }

        // 19.
        if(isExtensible && !isAnyNonconfigurableKeyPresent) { return trapResult; }

        // 21.
        if(isNonconfigurableKeyMissingFromTrapResult) { throw TypeError; }

        // 22.
        if(isExtensible) { return trapResult; }

        // 23.
        if(isConfigurableKeyMissingFromTrapResult)  { throw TypeError; }

        // 24.
        if(!targetToTrapResultMap.Empty()) { throw TypeError; }

        return trapResult;
        */

        JavascriptArray* trapResult = requestContext->GetLibrary()->CreateArray(0);
        bool isConfigurableKeyMissingFromTrapResult = false;
        bool isNonconfigurableKeyMissingFromTrapResult = false;
        bool isKeyMissingFromTrapResult = false;
        bool isKeyMissingFromTargetResult = false;
        bool isAnyNonconfigurableKeyPresent = false;
        Var element;
        PropertyId propertyId;
        const PropertyRecord* propertyRecord = nullptr;
        BOOL isTargetExtensible = FALSE;

        BEGIN_TEMP_ALLOCATOR(tempAllocator, requestContext, _u("Runtime"))
        {
            // Dictionary containing intersection of keys present in targetKeys and trapResult
            Var lenValue = JavascriptOperators::OP_GetLength(trapResultArray, requestContext);
            uint32 len = (uint32)JavascriptConversion::ToLength(lenValue, requestContext);

            JsUtil::BaseDictionary<Js::PropertyId, bool, ArenaAllocator> targetToTrapResultMap(tempAllocator, len);

            // Trap result to return.
            // Note : This will not necessarily have all elements present in trapResultArray. E.g. If trap was called from GetOwnPropertySymbols()
            // trapResult will only contain symbol elements from trapResultArray.

            switch (keysTrapKind)
            {
            case GetOwnPropertyNamesKind:
                GetOwnPropertyKeysHelper(requestContext, trapResultArray, len, trapResult, targetToTrapResultMap,
                    [&](const PropertyRecord *propertyRecord)->bool
                {
                    return !propertyRecord->IsSymbol();
                });
                break;
            case GetOwnPropertySymbolKind:
                GetOwnPropertyKeysHelper(requestContext, trapResultArray, len, trapResult, targetToTrapResultMap,
                    [&](const PropertyRecord *propertyRecord)->bool
                {
                    return propertyRecord->IsSymbol();
                });
                break;
            case KeysKind:
                GetOwnPropertyKeysHelper(requestContext, trapResultArray, len, trapResult, targetToTrapResultMap,
                    [&](const PropertyRecord *propertyRecord)->bool
                {
                    return true;
                });
                break;
            }

            isTargetExtensible = targetObj->IsExtensible();
            targetKeys = JavascriptOperators::GetOwnPropertyKeys(targetObj, requestContext);

            for (uint32 i = 0; i < targetKeys->GetLength(); i++)
            {
                element = targetKeys->DirectGetItem(i);
                AssertMsg(VarIs<JavascriptSymbol>(element) || VarIs<JavascriptString>(element), "Invariant check during ownKeys proxy trap should make sure we only get property key here. (symbol or string primitives)");
                JavascriptConversion::ToPropertyKey(element, requestContext, &propertyRecord, nullptr);
                propertyId = propertyRecord->GetPropertyId();

                if (propertyId == Constants::NoProperty)
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

                PropertyDescriptor targetKeyPropertyDescriptor;
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
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
        }

        // 22.
        if (isTargetExtensible)
        {
            return trapResult;
        }

        // 23.
        if (isConfigurableKeyMissingFromTrapResult)
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
        }

        // 24.
        if (isKeyMissingFromTargetResult)
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
        }

        return trapResult;
    }

#if ENABLE_TTD
    void JavascriptProxy::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        if(this->handler != nullptr)
        {
            extractor->MarkVisitVar(this->handler);
        }

        if(this->target != nullptr)
        {
            extractor->MarkVisitVar(this->target);
        }
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptProxy::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapProxyObject;
    }

    void JavascriptProxy::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapProxyInfo* spi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapProxyInfo>();

        const uint32 reserveSize = 2;
        uint32 depOnCount = 0;
        TTD_PTR_ID* depOnArray = alloc.SlabReserveArraySpace<TTD_PTR_ID>(reserveSize);

        spi->HandlerId = TTD_INVALID_PTR_ID;
        if(this->handler != nullptr)
        {
            spi->HandlerId = TTD_CONVERT_VAR_TO_PTR_ID(this->handler);

            if(TTD::JsSupport::IsVarComplexKind(this->handler))
            {
                depOnArray[depOnCount] = TTD_CONVERT_VAR_TO_PTR_ID(this->handler);
                depOnCount++;
            }
        }

        spi->TargetId = TTD_INVALID_PTR_ID;
        if(this->target != nullptr)
        {
            spi->TargetId = TTD_CONVERT_VAR_TO_PTR_ID(this->target);

            if(TTD::JsSupport::IsVarComplexKind(this->handler))
            {
                depOnArray[depOnCount] = TTD_CONVERT_VAR_TO_PTR_ID(this->target);
                depOnCount++;
            }
        }

        if(depOnCount == 0)
        {
            alloc.SlabAbortArraySpace<TTD_PTR_ID>(reserveSize);

            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapProxyInfo*, TTD::NSSnapObjects::SnapObjectType::SnapProxyObject>(objData, spi);
        }
        else
        {
            alloc.SlabCommitArraySpace<TTD_PTR_ID>(depOnCount, reserveSize);

            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapProxyInfo*, TTD::NSSnapObjects::SnapObjectType::SnapProxyObject>(objData, spi, alloc, depOnCount, depOnArray);
        }
    }
#endif
}

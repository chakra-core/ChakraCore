//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "Types/NullTypeHandler.h"

namespace Js
{
    Var JavascriptObject::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch.
        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr
            || JavascriptOperators::GetTypeId(args[0]) == TypeIds_HostDispatch);

        if (args.Info.Count > 1)
        {
            switch (JavascriptOperators::GetTypeId(args[1]))
            {
            case TypeIds_Undefined:
            case TypeIds_Null:
                // Break to return a new object
                break;

            case TypeIds_StringObject:
            case TypeIds_Function:
            case TypeIds_Array:
            case TypeIds_ES5Array:
            case TypeIds_RegEx:
            case TypeIds_NumberObject:
            case TypeIds_SIMDObject:
            case TypeIds_Date:
            case TypeIds_BooleanObject:
            case TypeIds_Error:
            case TypeIds_Object:
            case TypeIds_Arguments:
            case TypeIds_ActivationObject:
            case TypeIds_SymbolObject:
                return isCtorSuperCall ?
                    JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), RecyclableObject::FromVar(args[1]), nullptr, scriptContext) :
                    args[1];

            default:
                RecyclableObject* result = nullptr;
                if (FALSE == JavascriptConversion::ToObject(args[1], scriptContext, &result))
                {
                    // JavascriptConversion::ToObject should only return FALSE for null and undefined.
                    Assert(false);
                }

                return isCtorSuperCall ?
                    JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), result, nullptr, scriptContext) :
                    result;
            }
        }

        if (callInfo.Flags & CallFlags_NotUsed)
        {
            return args[0];
        }
        Var newObj = scriptContext->GetLibrary()->CreateObject(true);
        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), RecyclableObject::FromVar(newObj), nullptr, scriptContext) :
            newObj;
    }

    Var JavascriptObject::EntryHasOwnProperty(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        RecyclableObject* dynamicObject = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &dynamicObject))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.hasOwnProperty"));
        }

        // no property specified
        if (args.Info.Count == 1)
        {
            return scriptContext->GetLibrary()->GetFalse();
        }

        const PropertyRecord* propertyRecord;
        JavascriptConversion::ToPropertyKey(args[1], scriptContext, &propertyRecord);

        if (JavascriptOperators::HasOwnProperty(dynamicObject, propertyRecord->GetPropertyId(), scriptContext))
        {
            return scriptContext->GetLibrary()->GetTrue();
        }

        return scriptContext->GetLibrary()->GetFalse();
    }

    Var JavascriptObject::EntryPropertyIsEnumerable(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        RecyclableObject* dynamicObject = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &dynamicObject))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.propertyIsEnumerable"));
        }

        if (args.Info.Count >= 2)
        {
            const PropertyRecord* propertyRecord;
            JavascriptConversion::ToPropertyKey(args[1], scriptContext, &propertyRecord);
            PropertyId propertyId = propertyRecord->GetPropertyId();

            PropertyDescriptor currentDescriptor;
            BOOL isCurrentDescriptorDefined = JavascriptOperators::GetOwnPropertyDescriptor(dynamicObject, propertyId, scriptContext, &currentDescriptor);
            if (isCurrentDescriptorDefined == TRUE)
            {
                if (currentDescriptor.IsEnumerable())
                {
                    return scriptContext->GetLibrary()->GetTrue();
                }
            }
        }
        return scriptContext->GetLibrary()->GetFalse();
    }

    BOOL JavascriptObject::ChangePrototype(RecyclableObject* object, RecyclableObject* newPrototype, bool shouldThrow, ScriptContext* scriptContext)
    {
        // 8.3.2    [[SetInheritance]] (V)
        // When the [[SetInheritance]] internal method of O is called with argument V the following steps are taken:
        // 1.   Assert: Either Type(V) is Object or Type(V) is Null.
        Assert(JavascriptOperators::IsObject(object));
        Assert(JavascriptOperators::IsObjectOrNull(newPrototype));

        if (JavascriptProxy::Is(object))
        {
            JavascriptProxy* proxy = JavascriptProxy::FromVar(object);
            CrossSite::ForceCrossSiteThunkOnPrototypeChain(newPrototype);
            return proxy->SetPrototypeTrap(newPrototype, shouldThrow, scriptContext);
        }

        // 2.   Let extensible be the value of the [[Extensible]] internal data property of O.
        // 3.   Let current be the value of the [[Prototype]] internal data property of O.
        // 4.   If SameValue(V, current), then return true.
        if (newPrototype == JavascriptObject::GetPrototypeOf(object, scriptContext))
        {
            return TRUE;
        }

        // 5.   If extensible is false, then return false.
        if (!object->IsExtensible())
        {
            if (shouldThrow)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NonExtensibleObject);
            }
            return FALSE;
        }

        if (object->IsProtoImmutable())
        {
            // ES2016 19.1.3:
            // The Object prototype object is the intrinsic object %ObjectPrototype%.
            // The Object prototype object is an immutable prototype exotic object.
            // ES2016 9.4.7:
            // An immutable prototype exotic object is an exotic object that has an immutable [[Prototype]] internal slot.
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ImmutablePrototypeSlot);
        }

        // 6.   If V is not null, then
        //  a.  Let p be V.
        //  b.  Repeat, while p is not null
        //      i.  If SameValue(p, O) is true, then return false.
        //      ii. Let nextp be the result of calling the [[GetInheritance]] internal method of p with no arguments.
        //      iii.    ReturnIfAbrupt(nextp).
        //      iv. Let  p be nextp.
        if (IsPrototypeOf(object, newPrototype, scriptContext)) // Reject cycle
        {
            if (shouldThrow)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_CyclicProtoValue);
            }
            return FALSE;
        }

        // 7.   Set the value of the [[Prototype]] internal data property of O to V.
        // 8.   Return true.

        bool isInvalidationOfInlineCacheNeeded = true;
        DynamicObject * obj = DynamicObject::FromVar(object);

        // If this object was not prototype object, then no need to invalidate inline caches.
        // Simply assign it a new type so if this object used protoInlineCache in past, it will
        // be invalidated because of type mismatch and subsequently we will update its protoInlineCache
        if (!(obj->GetDynamicType()->GetTypeHandler()->GetFlags() & DynamicTypeHandler::IsPrototypeFlag))
        {
            // If object has locked type, skip changing its type here as it will be changed anyway below
            // when object gets newPrototype object.
            if (!obj->HasLockedType())
            {
                obj->ChangeType();
            }
            Assert(!obj->GetScriptContext()->GetThreadContext()->IsObjectRegisteredInProtoInlineCaches(obj));
            Assert(!obj->GetScriptContext()->GetThreadContext()->IsObjectRegisteredInStoreFieldInlineCaches(obj));
            isInvalidationOfInlineCacheNeeded = false;
        }

        if (isInvalidationOfInlineCacheNeeded)
        {
            // Notify old prototypes that they are being removed from a prototype chain. This triggers invalidating protocache, etc.
            JavascriptOperators::MapObjectAndPrototypes<true>(object->GetPrototype(), [=](RecyclableObject* obj)
            {
                obj->RemoveFromPrototype(scriptContext);
            });

            // Examine new prototype chain. If it brings in any non-WritableData property, we need to invalidate related caches.
            bool objectAndPrototypeChainHasOnlyWritableDataProperties =
                JavascriptOperators::CheckIfObjectAndPrototypeChainHasOnlyWritableDataProperties(newPrototype);

            if (!objectAndPrototypeChainHasOnlyWritableDataProperties
                || object->GetScriptContext() != newPrototype->GetScriptContext())
            {
                // The HaveOnlyWritableDataProperties cache is cleared when a property is added or changed,
                // but only for types in the same script context. Therefore, if the prototype is in another
                // context, the object's cache won't be cleared when a property is added or changed on the prototype.
                // Moreover, an object is added to the cache only when its whole prototype chain is in the same
                // context.
                //
                // Since we don't have a way to find out which objects have a certain object as their prototype,
                // we clear the cache here instead.

                // Invalidate fast prototype chain writable data test flag
                object->GetLibrary()->NoPrototypeChainsAreEnsuredToHaveOnlyWritableDataProperties();
            }

            if (!objectAndPrototypeChainHasOnlyWritableDataProperties)
            {
                // Invalidate StoreField/PropertyGuards for any non-WritableData property in the new chain
                JavascriptOperators::MapObjectAndPrototypes<true>(newPrototype, [=](RecyclableObject* obj)
                {
                    if (!obj->HasOnlyWritableDataProperties())
                    {
                        obj->AddToPrototype(scriptContext);
                    }
                });
            }
        }

        // Set to new prototype
        if (object->IsExternal() || (DynamicType::Is(object->GetTypeId()) && (DynamicObject::FromVar(object))->IsCrossSiteObject()))
        {
            CrossSite::ForceCrossSiteThunkOnPrototypeChain(newPrototype);
        }
        object->SetPrototype(newPrototype);
        return TRUE;
    }

    Var JavascriptObject::EntryIsPrototypeOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        // no property specified
        if (args.Info.Count == 1 || !JavascriptOperators::IsObject(args[1]))
        {
            return scriptContext->GetLibrary()->GetFalse();
        }

        RecyclableObject* dynamicObject = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &dynamicObject))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.isPrototypeOf"));
        }
        RecyclableObject* value = RecyclableObject::FromVar(args[1]);

        if (dynamicObject->GetTypeId() == TypeIds_GlobalObject)
        {
            dynamicObject = RecyclableObject::FromVar(static_cast<Js::GlobalObject*>(dynamicObject)->ToThis());
        }

        while (JavascriptOperators::GetTypeId(value) != TypeIds_Null)
        {
            value = JavascriptOperators::GetPrototype(value);
            if (dynamicObject == value)
            {
                return scriptContext->GetLibrary()->GetTrue();
            }
        }

        return scriptContext->GetLibrary()->GetFalse();
    }

    // 19.1.3.5 - Object.prototype.toLocaleString as of ES6 (6.0)
    Var JavascriptObject::EntryToLocaleString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        Assert(!(callInfo.Flags & CallFlags_New));
        AssertMsg(args.Info.Count, "Should always have implicit 'this'");

        Var thisValue = args[0];
        RecyclableObject* dynamicObject = nullptr;

        if (FALSE == JavascriptConversion::ToObject(thisValue, scriptContext, &dynamicObject))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.toLocaleString"));
        }

        Var toStringVar = nullptr;
        if (!JavascriptOperators::GetProperty(thisValue, dynamicObject, Js::PropertyIds::toString, &toStringVar, scriptContext) || !JavascriptConversion::IsCallable(toStringVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Object.prototype.toLocaleString"));
        }

        RecyclableObject* toStringFunc = RecyclableObject::FromVar(toStringVar);
        return CALL_FUNCTION(scriptContext->GetThreadContext(), toStringFunc, CallInfo(CallFlags_Value, 1), thisValue);
    }

    Var JavascriptObject::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        AssertMsg(args.Info.Count, "Should always have implicit 'this'");
        return ToStringHelper(args[0], scriptContext);
    }

    // ES2017 19.1.3.6 Object.prototype.toString()
    JavascriptString* JavascriptObject::ToStringTagHelper(Var thisArg, ScriptContext *scriptContext, TypeId type)
    {
        JavascriptLibrary *library = scriptContext->GetLibrary();

        // 1. If the this value is undefined, return "[object Undefined]".
        if (type == TypeIds_Undefined)
        {
            return library->CreateStringFromCppLiteral(_u("[object Undefined]"));
        }
        // 2. If the this value is null, return "[object Null]".
        if (type == TypeIds_Null)
        {
            return library->CreateStringFromCppLiteral(_u("[object Null]"));
        }

        // 3. Let O be ToObject(this value).
        RecyclableObject *thisArgAsObject = RecyclableObject::FromVar(JavascriptOperators::ToObject(thisArg, scriptContext));

        // 4. Let isArray be ? IsArray(O).
        // There is an implicit check for a null proxy handler in IsArray, so use the operator.
        BOOL isArray = JavascriptOperators::IsArray(thisArgAsObject);

        // 15. Let tag be ? Get(O, @@toStringTag).
        Var tag = JavascriptOperators::GetProperty(thisArgAsObject, PropertyIds::_symbolToStringTag, scriptContext); // Let tag be the result of Get(O, @@toStringTag).

        // 17. Return the String that is the result of concatenating "[object ", tag, and "]".
        auto buildToString = [&scriptContext](Var tag) {
            JavascriptString *tagStr = JavascriptString::FromVar(tag);
            const WCHAR objectStartString[9] = _u("[object ");
            const WCHAR objectEndString[1] = { _u(']') };
            CompoundString *const cs = CompoundString::NewWithCharCapacity(_countof(objectStartString)
              + _countof(objectEndString) + tagStr->GetLength(), scriptContext->GetLibrary());

            cs->AppendChars(objectStartString, _countof(objectStartString) - 1 /* ditch \0 */);
            cs->AppendChars(tagStr);
            cs->AppendChars(objectEndString, _countof(objectEndString));

            return cs;
        };
        if (tag != nullptr && JavascriptString::Is(tag))
        {
            return buildToString(tag);
        }

        // If we don't have a tag or it's not a string, use the 'built in tag'.
        if (isArray)
        {
            // 5. If isArray is true, let builtinTag be "Array".
            return library->GetObjectArrayDisplayString();
        }

        JavascriptString* builtInTag = nullptr;
        switch (type)
        {
            // 6. Else if O is an exotic String object, let builtinTag be "String".
        case TypeIds_String:
        case TypeIds_StringObject:
            builtInTag = library->GetObjectStringDisplayString();
            break;

            // 7. Else if O has an[[ParameterMap]] internal slot, let builtinTag be "Arguments".
        case TypeIds_Arguments:
            builtInTag = library->GetObjectArgumentsDisplayString();
            break;

            // 8. Else if O has a [[Call]] internal method, let builtinTag be "Function".
        case TypeIds_Function:
            builtInTag = library->GetObjectFunctionDisplayString();
            break;

            // 9. Else if O has an [[ErrorData]] internal slot, let builtinTag be "Error".
        case TypeIds_Error:
            builtInTag = library->GetObjectErrorDisplayString();
            break;

            // 10. Else if O has a [[BooleanData]] internal slot, let builtinTag be "Boolean".
        case TypeIds_Boolean:
        case TypeIds_BooleanObject:
            builtInTag = library->GetObjectBooleanDisplayString();
            break;

            // 11. Else if O has a [[NumberData]] internal slot, let builtinTag be "Number".
        case TypeIds_Number:
        case TypeIds_Int64Number:
        case TypeIds_UInt64Number:
        case TypeIds_Integer:
        case TypeIds_NumberObject:
            builtInTag = library->GetObjectNumberDisplayString();
            break;

            // 12. Else if O has a [[DateValue]] internal slot, let builtinTag be "Date".
        case TypeIds_Date:
        case TypeIds_WinRTDate:
            builtInTag = library->GetObjectDateDisplayString();
            break;

            // 13. Else if O has a [[RegExpMatcher]] internal slot, let builtinTag be "RegExp".
        case TypeIds_RegEx:
            builtInTag = library->GetObjectRegExpDisplayString();
            break;

            // 14. Else, let builtinTag be "Object".
        default:
        {
            if (thisArgAsObject->IsExternal())
            {
                builtInTag = buildToString(thisArgAsObject->GetClassName(scriptContext));
            }
            else
            {
                builtInTag = library->GetObjectDisplayString(); // [object Object]
            }
            break;
        }
        }

        Assert(builtInTag != nullptr);

        return builtInTag;
    }

    Var JavascriptObject::ToStringHelper(Var thisArg, ScriptContext* scriptContext)
    {
        TypeId type = JavascriptOperators::GetTypeId(thisArg);

        // We first need to make sure we are in the right context.
        if (type == TypeIds_HostDispatch)
        {
            RecyclableObject* hostDispatchObject = RecyclableObject::FromVar(thisArg);
            const DynamicObject* remoteObject = hostDispatchObject->GetRemoteObject();
            if (!remoteObject)
            {
                Var result = nullptr;
                Js::Var values[1];
                Js::CallInfo info(Js::CallFlags_Value, 1);
                Js::Arguments args(info, values);
                values[0] = thisArg;
                if (hostDispatchObject->InvokeBuiltInOperationRemotely(EntryToString, args, &result))
                {
                    return result;
                }
            }
        }

        // Dispatch to @@toStringTag implementation.
        if (type >= TypeIds_TypedArrayMin && type <= TypeIds_TypedArrayMax && !scriptContext->GetThreadContext()->IsScriptActive())
        {
            // Use external call for typedarray in the debugger.
            Var toStringValue = nullptr;
            BEGIN_JS_RUNTIME_CALL_EX(scriptContext, false);
            toStringValue = ToStringTagHelper(thisArg, scriptContext, type);
            END_JS_RUNTIME_CALL(scriptContext);
            return toStringValue;
        }

        // By this point, we should be in the correct context, but the thisArg may still need to be marshalled (for to the implicit ToObject conversion call.)
        return ToStringTagHelper(CrossSite::MarshalVar(scriptContext, thisArg), scriptContext, type);
    }

    // -----------------------------------------------------------
    // Object.prototype.valueOf
    //    1.    Let O be the result of calling ToObject passing the this value as the argument.
    //    2.    If O is the result of calling the Object constructor with a host object (15.2.2.1), then
    //    a.    Return either O or another value such as the host object originally passed to the constructor. The specific result that is returned is implementation-defined.
    //    3.    Return O.
    // -----------------------------------------------------------

    Var JavascriptObject::EntryValueOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        TypeId argType = JavascriptOperators::GetTypeId(args[0]);

        // throw a TypeError if TypeId is null or undefined, and apply ToObject to the 'this' value otherwise.

        if ((argType == TypeIds_Null) || (argType == TypeIds_Undefined))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.valueOf"));
        }
        else
        {
            return JavascriptOperators::ToObject(args[0], scriptContext);
        }
    }

    Var JavascriptObject::EntryGetOwnPropertyDescriptor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        RecyclableObject* obj = nullptr;
        if (args.Info.Count < 2)
        {
            obj = RecyclableObject::FromVar(JavascriptOperators::ToObject(scriptContext->GetLibrary()->GetUndefined(), scriptContext));
        }
        else
        {
            // Convert the argument to object first
            obj = RecyclableObject::FromVar(JavascriptOperators::ToObject(args[1], scriptContext));
        }

        // If the object is HostDispatch try to invoke the operation remotely
        if (obj->GetTypeId() == TypeIds_HostDispatch)
        {
            Var result;
            if (obj->InvokeBuiltInOperationRemotely(EntryGetOwnPropertyDescriptor, args, &result))
            {
                return result;
            }
        }

        Var propertyKey = args.Info.Count > 2 ? args[2] : obj->GetLibrary()->GetUndefined();

        return JavascriptObject::GetOwnPropertyDescriptorHelper(obj, propertyKey, scriptContext);
    }

    Var JavascriptObject::GetOwnPropertyDescriptorHelper(RecyclableObject* obj, Var propertyKey, ScriptContext* scriptContext)
    {
        const PropertyRecord* propertyRecord;
        JavascriptConversion::ToPropertyKey(propertyKey, scriptContext, &propertyRecord);
        PropertyId propertyId = propertyRecord->GetPropertyId();

        obj->ThrowIfCannotGetOwnPropertyDescriptor(propertyId);

        PropertyDescriptor propertyDescriptor;
        BOOL isPropertyDescriptorDefined;
        isPropertyDescriptorDefined = JavascriptObject::GetOwnPropertyDescriptorHelper(obj, propertyId, scriptContext, propertyDescriptor);
        if (!isPropertyDescriptorDefined)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        return JavascriptOperators::FromPropertyDescriptor(propertyDescriptor, scriptContext);
    }

    BOOL JavascriptObject::GetOwnPropertyDescriptorHelper(RecyclableObject* obj, PropertyId propertyId, ScriptContext* scriptContext, PropertyDescriptor& propertyDescriptor)
    {
        BOOL isPropertyDescriptorDefined;
        if (obj->CanHaveInterceptors())
        {
            isPropertyDescriptorDefined = obj->HasOwnProperty(propertyId) ?
                JavascriptOperators::GetOwnPropertyDescriptor(obj, propertyId, scriptContext, &propertyDescriptor) : obj->GetDefaultPropertyDescriptor(propertyDescriptor);
        }
        else
        {
            isPropertyDescriptorDefined = JavascriptOperators::GetOwnPropertyDescriptor(obj, propertyId, scriptContext, &propertyDescriptor) ||
                obj->GetDefaultPropertyDescriptor(propertyDescriptor);
        }
        return isPropertyDescriptorDefined;
    }

    Var JavascriptObject::EntryGetOwnPropertyDescriptors(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        RecyclableObject* obj = nullptr;

        if (args.Info.Count < 2)
        {
            obj = RecyclableObject::FromVar(JavascriptOperators::ToObject(scriptContext->GetLibrary()->GetUndefined(), scriptContext));
        }
        else
        {
            // Convert the argument to object first
            obj = RecyclableObject::FromVar(JavascriptOperators::ToObject(args[1], scriptContext));
        }

        // If the object is HostDispatch try to invoke the operation remotely
        if (obj->GetTypeId() == TypeIds_HostDispatch)
        {
            Var result;
            if (obj->InvokeBuiltInOperationRemotely(EntryGetOwnPropertyDescriptors, args, &result))
            {
                return result;
            }
        }

        JavascriptArray* ownPropertyKeys = JavascriptOperators::GetOwnPropertyKeys(obj, scriptContext);
        RecyclableObject* resultObj = scriptContext->GetLibrary()->CreateObject(true, (Js::PropertyIndex) ownPropertyKeys->GetLength());

        PropertyDescriptor propDesc;
        Var propKey = nullptr;

        for (uint i = 0; i < ownPropertyKeys->GetLength(); i++)
        {
            BOOL getPropResult = ownPropertyKeys->DirectGetItemAt(i, &propKey);
            Assert(getPropResult);

            if (!getPropResult)
            {
                continue;
            }

            PropertyRecord const * propertyRecord;
            JavascriptConversion::ToPropertyKey(propKey, scriptContext, &propertyRecord);

            Var newDescriptor = JavascriptObject::GetOwnPropertyDescriptorHelper(obj, propKey, scriptContext);
            if (!JavascriptOperators::IsUndefined(newDescriptor))
            {
                resultObj->SetProperty(propertyRecord->GetPropertyId(), newDescriptor, PropertyOperation_None, nullptr);
            }
        }

        return resultObj;
    }

    Var JavascriptObject::EntryGetPrototypeOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_getPrototypeOf);

        // 19.1.2.9
        // Object.getPrototypeOf ( O )
        // When the getPrototypeOf function is called with argument O, the following steps are taken:
        RecyclableObject *object = nullptr;

        // 1. Let obj be ToObject(O).
        // 2. ReturnIfAbrupt(obj).
        if (args.Info.Count < 2 || !JavascriptConversion::ToObject(args[1], scriptContext, &object))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Object.getPrototypeOf"));
        }

        // 3. Return obj.[[GetPrototypeOf]]().
        return CrossSite::MarshalVar(scriptContext, GetPrototypeOf(object, scriptContext));
    }

    Var JavascriptObject::EntrySetPrototypeOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));
        ScriptContext* scriptContext = function->GetScriptContext();

        // 19.1.2.18
        // Object.setPrototypeOf ( O, proto )
        // When the setPrototypeOf function is called with arguments O and proto, the following steps are taken:
        // 1. Let O be RequireObjectCoercible(O).
        // 2. ReturnIfAbrupt(O).
        // 3. If Type(proto) is neither Object or Null, then throw a TypeError exception.
        int32 errCode = NOERROR;

        if (args.Info.Count < 2 || !JavascriptConversion::CheckObjectCoercible(args[1], scriptContext))
        {
            errCode = JSERR_FunctionArgument_NeedObject;
        }
        else if (args.Info.Count < 3 || !JavascriptOperators::IsObjectOrNull(args[2]))
        {
            errCode = JSERR_FunctionArgument_NotObjectOrNull;
        }

        if (errCode != NOERROR)
        {
            JavascriptError::ThrowTypeError(scriptContext, errCode, _u("Object.setPrototypeOf"));
        }

        // 4. If Type(O) is not Object, return O.
        if (!JavascriptOperators::IsObject(args[1]))
        {
            return args[1];
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(args[1]);
#endif
        RecyclableObject* object = RecyclableObject::FromVar(args[1]);
        RecyclableObject* newPrototype = RecyclableObject::FromVar(args[2]);

        // 5. Let status be O.[[SetPrototypeOf]](proto).
        // 6. ReturnIfAbrupt(status).
        // 7. If status is false, throw a TypeError exception.
        ChangePrototype(object, newPrototype, /*shouldThrow*/true, scriptContext);

        // 8. Return O.
        return object;
    }

    Var JavascriptObject::EntrySeal(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_seal);

        // Spec update in Rev29 under section 19.1.2.17
        if (args.Info.Count < 2)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
        else if (!JavascriptOperators::IsObject(args[1]))
        {
            return args[1];
        }


        RecyclableObject *object = RecyclableObject::FromVar(args[1]);

        GlobalObject* globalObject = object->GetLibrary()->GetGlobalObject();
        if (globalObject != object && globalObject && (globalObject->ToThis() == object))
        {
            globalObject->Seal();
        }

        object->Seal();
        return object;
    }

    Var JavascriptObject::EntryFreeze(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_freeze);

        // Spec update in Rev29 under section 19.1.2.5
        if (args.Info.Count < 2)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
        else if (!JavascriptOperators::IsObject(args[1]))
        {
            return args[1];
        }

        RecyclableObject *object = RecyclableObject::FromVar(args[1]);

        GlobalObject* globalObject = object->GetLibrary()->GetGlobalObject();
        if (globalObject != object && globalObject && (globalObject->ToThis() == object))
        {
            globalObject->Freeze();
        }

        object->Freeze();
        return object;
    }

    Var JavascriptObject::EntryPreventExtensions(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_preventExtensions);

        // Spec update in Rev29 under section 19.1.2.15
        if (args.Info.Count < 2)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
        else if (!JavascriptOperators::IsObject(args[1]))
        {
            return args[1];
        }

        RecyclableObject *object = RecyclableObject::FromVar(args[1]);

        GlobalObject* globalObject = object->GetLibrary()->GetGlobalObject();
        if (globalObject != object && globalObject && (globalObject->ToThis() == object))
        {
            globalObject->PreventExtensions();
        }

        object->PreventExtensions();

        return object;
    }

    Var JavascriptObject::EntryIsSealed(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_isSealed);

        if (args.Info.Count < 2 || !JavascriptOperators::IsObject(args[1]))
        {
            return scriptContext->GetLibrary()->GetTrue();
        }

        RecyclableObject *object = RecyclableObject::FromVar(args[1]);

        BOOL isSealed = object->IsSealed();

        GlobalObject* globalObject = object->GetLibrary()->GetGlobalObject();
        if (isSealed && globalObject != object && globalObject && (globalObject->ToThis() == object))
        {
            isSealed = globalObject->IsSealed();
        }

        return scriptContext->GetLibrary()->GetTrueOrFalse(isSealed);
    }

    Var JavascriptObject::EntryIsFrozen(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_isFrozen);

        if (args.Info.Count < 2 || !JavascriptOperators::IsObject(args[1]))
        {
            return scriptContext->GetLibrary()->GetTrue();
        }

        RecyclableObject *object = RecyclableObject::FromVar(args[1]);

        BOOL isFrozen = object->IsFrozen();

        GlobalObject* globalObject = object->GetLibrary()->GetGlobalObject();
        if (isFrozen && globalObject != object && globalObject && (globalObject->ToThis() == object))
        {
            isFrozen = globalObject->IsFrozen();
        }

        return scriptContext->GetLibrary()->GetTrueOrFalse(isFrozen);
    }

    Var JavascriptObject::EntryIsExtensible(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_isExtensible);

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2 || !JavascriptOperators::IsObject(args[1]))
        {
            return scriptContext->GetLibrary()->GetFalse();
        }

        RecyclableObject *object = RecyclableObject::FromVar(args[1]);
        BOOL isExtensible = object->IsExtensible();

        GlobalObject* globalObject = object->GetLibrary()->GetGlobalObject();
        if (isExtensible && globalObject != object && globalObject && (globalObject->ToThis() == object))
        {
            isExtensible = globalObject->IsExtensible();
        }

        return scriptContext->GetLibrary()->GetTrueOrFalse(isExtensible);
    }

    Var JavascriptObject::EntryGetOwnPropertyNames(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_getOwnPropertyNames);

        Var tempVar = args.Info.Count < 2 ? scriptContext->GetLibrary()->GetUndefined() : args[1];
        RecyclableObject *object = RecyclableObject::FromVar(JavascriptOperators::ToObject(tempVar, scriptContext));

        if (object->GetTypeId() == TypeIds_HostDispatch)
        {
            Var result;
            if (object->InvokeBuiltInOperationRemotely(EntryGetOwnPropertyNames, args, &result))
            {
                return result;
            }
        }

        return JavascriptOperators::GetOwnPropertyNames(object, scriptContext);
    }

    Var JavascriptObject::EntryGetOwnPropertySymbols(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        Var tempVar = args.Info.Count < 2 ? scriptContext->GetLibrary()->GetUndefined() : args[1];
        RecyclableObject *object = RecyclableObject::FromVar(JavascriptOperators::ToObject(tempVar, scriptContext));

        if (object->GetTypeId() == TypeIds_HostDispatch)
        {
            Var result;
            if (object->InvokeBuiltInOperationRemotely(EntryGetOwnPropertySymbols, args, &result))
            {
                return result;
            }
        }

        return JavascriptOperators::GetOwnPropertySymbols(object, scriptContext);
    }

    Var JavascriptObject::EntryKeys(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_keys);

        Var tempVar = args.Info.Count < 2 ? scriptContext->GetLibrary()->GetUndefined() : args[1];
        RecyclableObject *object = RecyclableObject::FromVar(JavascriptOperators::ToObject(tempVar, scriptContext));

        if (object->GetTypeId() == TypeIds_HostDispatch)
        {
            Var result;
            if (object->InvokeBuiltInOperationRemotely(EntryKeys, args, &result))
            {
                return result;
            }
        }
        return JavascriptOperators::GetOwnEnumerablePropertyNames(object, scriptContext);
    }

    Var JavascriptObject::GetValuesOrEntries(RecyclableObject* object, bool valuesToReturn, ScriptContext* scriptContext)
    {
        Assert(object != nullptr);
        Assert(scriptContext != nullptr);
        JavascriptArray* valuesArray = scriptContext->GetLibrary()->CreateArray(0);

        JavascriptArray* ownKeysResult = JavascriptOperators::GetOwnPropertyNames(object, scriptContext);
        uint32 length = ownKeysResult->GetLength();

        Var nextKey;
        const PropertyRecord* propertyRecord = nullptr;
        PropertyId propertyId;
        for (uint32 i = 0, index = 0; i < length; i++)
        {
            nextKey = ownKeysResult->DirectGetItem(i);
            Assert(JavascriptString::Is(nextKey));

            PropertyDescriptor propertyDescriptor;

            JavascriptConversion::ToPropertyKey(nextKey, scriptContext, &propertyRecord);
            propertyId = propertyRecord->GetPropertyId();
            Assert(propertyId != Constants::NoProperty);

            if (JavascriptOperators::GetOwnPropertyDescriptor(object, propertyId, scriptContext, &propertyDescriptor))
            {
                if (propertyDescriptor.IsEnumerable())
                {
                    Var value = JavascriptOperators::GetProperty(object, propertyId, scriptContext);
                    if (!valuesToReturn)
                    {
                        // For Object.entries each entry is key, value pair
                        JavascriptArray* entry = scriptContext->GetLibrary()->CreateArray(2);
                        entry->DirectSetItemAt(0, CrossSite::MarshalVar(scriptContext, nextKey));
                        entry->DirectSetItemAt(1, CrossSite::MarshalVar(scriptContext, value));
                        value = entry;
                    }
                    valuesArray->DirectSetItemAt(index++, CrossSite::MarshalVar(scriptContext, value));
                }
            }
        }

        return valuesArray;
    }

    Var JavascriptObject::EntryValues(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_values);

        Var tempVar = args.Info.Count < 2 ? scriptContext->GetLibrary()->GetUndefined() : args[1];
        RecyclableObject *object = RecyclableObject::FromVar(JavascriptOperators::ToObject(tempVar, scriptContext));

        return GetValuesOrEntries(object, true /*valuesToReturn*/, scriptContext);
    }

    Var JavascriptObject::EntryEntries(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_entries);

        Var tempVar = args.Info.Count < 2 ? scriptContext->GetLibrary()->GetUndefined() : args[1];
        RecyclableObject *object = RecyclableObject::FromVar(JavascriptOperators::ToObject(tempVar, scriptContext));

        return GetValuesOrEntries(object, false /*valuesToReturn*/, scriptContext);
    }

    JavascriptArray* JavascriptObject::CreateOwnSymbolPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext)
    {
        return CreateKeysHelper(object, scriptContext, TRUE, true /*includeSymbolsOnly */, false, true /*includeSpecialProperties*/);
    }

    JavascriptArray* JavascriptObject::CreateOwnStringPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext)
    {
        return CreateKeysHelper(object, scriptContext, TRUE, false, true /*includeStringsOnly*/, true /*includeSpecialProperties*/);
    }

    JavascriptArray* JavascriptObject::CreateOwnStringSymbolPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext)
    {
        return CreateKeysHelper(object, scriptContext, TRUE, true/*includeSymbolsOnly*/, true /*includeStringsOnly*/, true /*includeSpecialProperties*/);
    }

    JavascriptArray* JavascriptObject::CreateOwnEnumerableStringPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext)
    {
        return CreateKeysHelper(object, scriptContext, FALSE, false, true/*includeStringsOnly*/, false);
    }

    JavascriptArray* JavascriptObject::CreateOwnEnumerableStringSymbolPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext)
    {
        return CreateKeysHelper(object, scriptContext, FALSE, true/*includeSymbolsOnly*/, true/*includeStringsOnly*/, false);
    }

    // 9.1.12 [[OwnPropertyKeys]] () in RC#4 dated April 3rd 2015.
    JavascriptArray* JavascriptObject::CreateKeysHelper(RecyclableObject* object, ScriptContext* scriptContext, BOOL includeNonEnumerable, bool includeSymbolProperties, bool includeStringProperties, bool includeSpecialProperties)
    {
        //1. Let keys be a new empty List.
        //2. For each own property key P of O that is an integer index, in ascending numeric index order
        //      a. Add P as the last element of keys.
        //3. For each own property key P of O that is a String but is not an integer index, in property creation order
        //      a. Add P as the last element of keys.
        //4. For each own property key P of O that is a Symbol, in property creation order
        //      a. Add P as the last element of keys.
        //5. Return keys.
        AssertMsg(includeStringProperties || includeSymbolProperties, "Should either get string or symbol properties.");

        JavascriptStaticEnumerator enumerator;
        JavascriptArray* newArr = scriptContext->GetLibrary()->CreateArray(0);
        JavascriptArray* newArrForSymbols = scriptContext->GetLibrary()->CreateArray(0);
        EnumeratorFlags flags = EnumeratorFlags::None;
        if (includeNonEnumerable)
        {
            flags |= EnumeratorFlags::EnumNonEnumerable;
        }
        if (includeSymbolProperties)
        {
            flags |= EnumeratorFlags::EnumSymbols;
        }
        if (!object->GetEnumerator(&enumerator, flags, scriptContext))
        {
            return newArr;  // Return an empty array if we don't have an enumerator
        }

        JavascriptString * propertyName = nullptr;
        PropertyId propertyId;
        uint32 propertyIndex = 0;
        uint32 symbolIndex = 0;
        const PropertyRecord* propertyRecord;
        JavascriptSymbol* symbol;

        while ((propertyName = enumerator.MoveAndGetNext(propertyId)) != NULL)
        {
            if (propertyName)
            {
                if (includeSymbolProperties)
                {
                    propertyRecord = scriptContext->GetPropertyName(propertyId);

                    if (propertyRecord->IsSymbol())
                    {
                        symbol = scriptContext->GetLibrary()->CreateSymbol(propertyRecord);
                        // no need to marshal symbol because it is created from scriptContext
                        newArrForSymbols->DirectSetItemAt(symbolIndex++, symbol);
                        continue;
                    }
                }
                if (includeStringProperties)
                {
                    newArr->DirectSetItemAt(propertyIndex++, CrossSite::MarshalVar(scriptContext, propertyName));
                }
            }
        }

        // Special properties
        if (includeSpecialProperties && includeStringProperties)
        {
            uint32 index = 0;
            while (object->GetSpecialPropertyName(index, &propertyName, scriptContext))
            {
                newArr->DirectSetItemAt(propertyIndex++, propertyName);
                index++;
            }
        }

        // Append all the symbols at the end of list
        uint32 totalSymbols = newArrForSymbols->GetLength();
        for (uint32 symIndex = 0; symIndex < totalSymbols; symIndex++)
        {
            newArr->DirectSetItemAt(propertyIndex++, newArrForSymbols->DirectGetItem(symIndex));
        }

        return newArr;
    }

    // args[1] this object to operate on.
    // args[2] property name.
    // args[3] object that attributes for the new descriptor.
    Var JavascriptObject::EntryDefineProperty(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2 || !JavascriptOperators::IsObject(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Object.defineProperty"));
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(args[1]);
#endif
        RecyclableObject* obj = RecyclableObject::FromVar(args[1]);

        // If the object is HostDispatch try to invoke the operation remotely
        if (obj->GetTypeId() == TypeIds_HostDispatch)
        {
            if (obj->InvokeBuiltInOperationRemotely(EntryDefineProperty, args, NULL))
            {
                return obj;
            }
        }

        Var propertyKey = args.Info.Count > 2 ? args[2] : obj->GetLibrary()->GetUndefined();
        PropertyRecord const * propertyRecord;
        JavascriptConversion::ToPropertyKey(propertyKey, scriptContext, &propertyRecord);

        Var descVar = args.Info.Count > 3 ? args[3] : obj->GetLibrary()->GetUndefined();
        PropertyDescriptor propertyDescriptor;
        if (!JavascriptOperators::ToPropertyDescriptor(descVar, &propertyDescriptor, scriptContext))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_PropertyDescriptor_Invalid, scriptContext->GetPropertyName(propertyRecord->GetPropertyId())->GetBuffer());
        }

        if (CONFIG_FLAG(UseFullName))
        {
            ModifyGetterSetterFuncName(propertyRecord, propertyDescriptor, scriptContext);
        }

        DefineOwnPropertyHelper(obj, propertyRecord->GetPropertyId(), propertyDescriptor, scriptContext);

        return obj;
    }

    Var JavascriptObject::EntryDefineProperties(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_defineProperties);

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2 || !JavascriptOperators::IsObject(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Object.defineProperties"));
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(args[1]);
#endif
        RecyclableObject *object = RecyclableObject::FromVar(args[1]);

        // If the object is HostDispatch try to invoke the operation remotely
        if (object->GetTypeId() == TypeIds_HostDispatch)
        {
            if (object->InvokeBuiltInOperationRemotely(EntryDefineProperties, args, NULL))
            {
                return object;
            }
        }

        Var propertiesVar = args.Info.Count > 2 ? args[2] : object->GetLibrary()->GetUndefined();
        RecyclableObject* properties = nullptr;
        if (FALSE == JavascriptConversion::ToObject(propertiesVar, scriptContext, &properties))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NullOrUndefined, _u("Object.defineProperties"));
        }

        return DefinePropertiesHelper(object, properties, scriptContext);
    }

    // args[1] property name.
    // args[2] function object to use as the getter function.
    Var JavascriptObject::EntryDefineGetter(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // For browser interop, simulate LdThis by calling OP implementation directly.
        // Do not have module id here so use the global id, 0.
        //
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(args[0]);
#endif
        RecyclableObject* obj = nullptr;
        if (!JavascriptConversion::ToObject(args[0], scriptContext, &obj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.__defineGetter__"));
        }

        Var getterFunc = args.Info.Count > 2 ? args[2] : obj->GetLibrary()->GetUndefined();

        if (!JavascriptConversion::IsCallable(getterFunc))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Object.prototype.__defineGetter__"));
        }

        Var propertyKey = args.Info.Count > 1 ? args[1] : obj->GetLibrary()->GetUndefined();
        const PropertyRecord* propertyRecord;
        JavascriptConversion::ToPropertyKey(propertyKey, scriptContext, &propertyRecord);

        PropertyDescriptor propertyDescriptor;
        propertyDescriptor.SetEnumerable(true);
        propertyDescriptor.SetConfigurable(true);
        propertyDescriptor.SetGetter(getterFunc);

        DefineOwnPropertyHelper(obj, propertyRecord->GetPropertyId(), propertyDescriptor, scriptContext);

        return obj->GetLibrary()->GetUndefined();
    }

    // args[1] property name.
    // args[2] function object to use as the setter function.
    Var JavascriptObject::EntryDefineSetter(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // For browser interop, simulate LdThis by calling OP implementation directly.
        // Do not have module id here so use the global id, 0.
        //
        RecyclableObject* obj = nullptr;
        if (!JavascriptConversion::ToObject(args[0], scriptContext, &obj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.__defineSetter__"));
        }

        Var setterFunc = args.Info.Count > 2 ? args[2] : obj->GetLibrary()->GetUndefined();

        if (!JavascriptConversion::IsCallable(setterFunc))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Object.prototype.__defineSetter__"));
        }

        Var propertyKey = args.Info.Count > 1 ? args[1] : obj->GetLibrary()->GetUndefined();
        const PropertyRecord* propertyRecord;
        JavascriptConversion::ToPropertyKey(propertyKey, scriptContext, &propertyRecord);

        PropertyDescriptor propertyDescriptor;
        propertyDescriptor.SetEnumerable(true);
        propertyDescriptor.SetConfigurable(true);
        propertyDescriptor.SetSetter(setterFunc);

        DefineOwnPropertyHelper(obj, propertyRecord->GetPropertyId(), propertyDescriptor, scriptContext);

        return obj->GetLibrary()->GetUndefined();
    }

    // args[1] property name.
    Var JavascriptObject::EntryLookupGetter(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        RecyclableObject* obj = nullptr;
        if (!JavascriptConversion::ToObject(args[0], scriptContext, &obj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.__lookupGetter__"));
        }

        Var propertyKey = args.Info.Count > 1 ? args[1] : obj->GetLibrary()->GetUndefined();
        const PropertyRecord* propertyRecord;
        JavascriptConversion::ToPropertyKey(propertyKey, scriptContext, &propertyRecord);

        Var getter = nullptr;
        Var unused = nullptr;
        if (JavascriptOperators::GetAccessors(obj, propertyRecord->GetPropertyId(), scriptContext, &getter, &unused))
        {
            if (getter != nullptr)
            {
                return getter;
            }
        }

        return obj->GetLibrary()->GetUndefined();
    }

    // args[1] property name.
    Var JavascriptObject::EntryLookupSetter(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        RecyclableObject* obj = nullptr;
        if (!JavascriptConversion::ToObject(args[0], scriptContext, &obj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Object.prototype.__lookupSetter__"));
        }

        Var propertyKey = args.Info.Count > 1 ? args[1] : obj->GetLibrary()->GetUndefined();
        const PropertyRecord* propertyRecord;
        JavascriptConversion::ToPropertyKey(propertyKey, scriptContext, &propertyRecord);

        Var unused = nullptr;
        Var setter = nullptr;
        if (JavascriptOperators::GetAccessors(obj, propertyRecord->GetPropertyId(), scriptContext, &unused, &setter))
        {
            if (setter != nullptr)
            {
                return setter;
            }
        }

        return obj->GetLibrary()->GetUndefined();
    }

    Var JavascriptObject::EntryIs(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        Var x = args.Info.Count > 1 ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        Var y = args.Info.Count > 2 ? args[2] : scriptContext->GetLibrary()->GetUndefined();

        return JavascriptBoolean::ToVar(JavascriptConversion::SameValue(x, y), scriptContext);
    }

    //ES6 19.1.2.1
    Var JavascriptObject::EntryAssign(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // 1. Let to be ToObject(target).
        // 2. ReturnIfAbrupt(to).
        // 3  If only one argument was passed, return to.
        RecyclableObject* to = nullptr;
        if (args.Info.Count == 1 || !JavascriptConversion::ToObject(args[1], scriptContext, &to))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Object.assign"));
        }

        if (args.Info.Count < 3)
        {
            return to;
        }

        // 4. Let sources be the List of argument values starting with the second argument.
        // 5. For each element nextSource of sources, in ascending index order,
        for (unsigned int i = 2; i < args.Info.Count; i++)
        {
            //      a. If nextSource is undefined or null, let keys be an empty List.
            //      b. Else,
            //          i.Let from be ToObject(nextSource).
            //          ii.ReturnIfAbrupt(from).
            //          iii.Let keys be from.[[OwnPropertyKeys]]().
            //          iv.ReturnIfAbrupt(keys).
            if (JavascriptOperators::IsUndefinedOrNull(args[i]))
            {
                continue;
            }

            RecyclableObject* from = nullptr;
            if (!JavascriptConversion::ToObject(args[i], scriptContext, &from))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Object.assign"));
            }

#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(from);
#endif

            // if proxy, take slow path by calling [[OwnPropertyKeys]] on source
            if (JavascriptProxy::Is(from))
            {
                AssignForProxyObjects(from, to, scriptContext);
            }
            // else use enumerator to extract keys from source
            else
            {
                AssignForGenericObjects(from, to, scriptContext);
            }
        }

        // 6. Return to.
        return to;
    }

    void JavascriptObject::AssignForGenericObjects(RecyclableObject* from, RecyclableObject* to, ScriptContext* scriptContext)
    {
        JavascriptStaticEnumerator enumerator;
        if (!from->GetEnumerator(&enumerator, EnumeratorFlags::SnapShotSemantics | EnumeratorFlags::EnumSymbols, scriptContext))
        {
            //nothing to enumerate, continue with the nextSource.
            return;
        }

        PropertyId nextKey = Constants::NoProperty;
        Var propValue = nullptr;
        JavascriptString * propertyName = nullptr;

        //enumerate through each property of properties and fetch the property descriptor
        while ((propertyName = enumerator.MoveAndGetNext(nextKey)) != NULL)
        {
            if (nextKey == Constants::NoProperty)
            {
                PropertyRecord const * propertyRecord = nullptr;

                scriptContext->GetOrAddPropertyRecord(propertyName->GetString(), propertyName->GetLength(), &propertyRecord);
                nextKey = propertyRecord->GetPropertyId();
            }

            if (!JavascriptOperators::GetOwnProperty(from, nextKey, &propValue, scriptContext))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Operand_Invalid_NeedObject, _u("Object.assign"));
            }

            if (!JavascriptOperators::SetProperty(to, to, nextKey, propValue, scriptContext, PropertyOperationFlags::PropertyOperation_ThrowIfNonWritable))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Operand_Invalid_NeedObject, _u("Object.assign"));
            }
        }
    }

    void JavascriptObject::AssignForProxyObjects(RecyclableObject* from, RecyclableObject* to, ScriptContext* scriptContext)
    {
         JavascriptArray *keys = JavascriptOperators::GetOwnEnumerablePropertyNamesSymbols(from, scriptContext);

        //      c. Repeat for each element nextKey of keys in List order,
        //          i. Let desc be from.[[GetOwnProperty]](nextKey).
        //          ii. ReturnIfAbrupt(desc).
        //          iii. if desc is not undefined and desc.[[Enumerable]] is true, then
        //              1. Let propValue be Get(from, nextKey).
        //              2. ReturnIfAbrupt(propValue).
        //              3. Let status be Set(to, nextKey, propValue, true);
        //              4. ReturnIfAbrupt(status).
        uint32 length = keys->GetLength();
        Var nextKey;
        const PropertyRecord* propertyRecord = nullptr;
        PropertyId propertyId;
        Var propValue = nullptr;
        for (uint32 j = 0; j < length; j++)
        {
            PropertyDescriptor propertyDescriptor;
            nextKey = keys->DirectGetItem(j);
            AssertMsg(JavascriptSymbol::Is(nextKey) || JavascriptString::Is(nextKey), "Invariant check during ownKeys proxy trap should make sure we only get property key here. (symbol or string primitives)");
            // Spec doesn't strictly call for us to use ToPropertyKey but since we know nextKey is already a symbol or string primitive, ToPropertyKey will be a nop and return us the propertyRecord
            JavascriptConversion::ToPropertyKey(nextKey, scriptContext, &propertyRecord);
            propertyId = propertyRecord->GetPropertyId();
            AssertMsg(propertyId != Constants::NoProperty, "AssignForProxyObjects - OwnPropertyKeys returned a propertyId with value NoProperty.");
            if (JavascriptOperators::GetOwnPropertyDescriptor(from, propertyRecord->GetPropertyId(), scriptContext, &propertyDescriptor))
            {
                if (propertyDescriptor.IsEnumerable())
                {
                    if (!JavascriptOperators::GetOwnProperty(from, propertyId, &propValue, scriptContext))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_Operand_Invalid_NeedObject, _u("Object.assign"));
                    }
                    if (!JavascriptOperators::SetProperty(to, to, propertyId, propValue, scriptContext))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_Operand_Invalid_NeedObject, _u("Object.assign"));
                    }
                }
            }
        }
    }

    //ES5 15.2.3.5
    Var JavascriptObject::EntryCreate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        Recycler *recycler = scriptContext->GetRecycler();


        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_create)

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NotObjectOrNull, _u("Object.create"));
        }

        TypeId typeId = JavascriptOperators::GetTypeId(args[1]);
        if (typeId != TypeIds_Null && !JavascriptOperators::IsObjectType(typeId))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NotObjectOrNull, _u("Object.create"));
        }

        //Create a new DynamicType with first argument as prototype and non shared type
        RecyclableObject *prototype = RecyclableObject::FromVar(args[1]);
        DynamicType *objectType = DynamicType::New(scriptContext, TypeIds_Object, prototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), false);

        //Create a new Object using this type.
        DynamicObject* object = DynamicObject::New(recycler, objectType);
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(object));
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            object = DynamicObject::FromVar(JavascriptProxy::AutoProxyWrapper(object));
        }
#endif

        if (args.Info.Count > 2 && JavascriptOperators::GetTypeId(args[2]) != TypeIds_Undefined)
        {
            RecyclableObject* properties = nullptr;
            if (FALSE == JavascriptConversion::ToObject(args[2], scriptContext, &properties))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NullOrUndefined, _u("Object.create"));
            }
            return DefinePropertiesHelper(object, properties, scriptContext);
        }
        return object;
    }

    Var JavascriptObject::DefinePropertiesHelper(RecyclableObject *object, RecyclableObject* props, ScriptContext *scriptContext)
    {
        if (JavascriptProxy::Is(props))
        {
            return DefinePropertiesHelperForProxyObjects(object, props, scriptContext);
        }
        else
        {
            return DefinePropertiesHelperForGenericObjects(object, props, scriptContext);
        }
    }


    Var JavascriptObject::DefinePropertiesHelperForGenericObjects(RecyclableObject *object, RecyclableObject* props, ScriptContext *scriptContext)
    {
        size_t descSize = 16;
        size_t descCount = 0;
        struct DescriptorMap
        {
            Field(PropertyRecord const *) propRecord;
            Field(PropertyDescriptor) descriptor;
            Field(Var) originalVar;
        };

        JavascriptStaticEnumerator enumerator;
        if (!props->GetEnumerator(&enumerator, EnumeratorFlags::EnumSymbols, scriptContext))
        {
            return object;
        }

        ENTER_PINNED_SCOPE(DescriptorMap, descriptors);
        descriptors = RecyclerNewArray(scriptContext->GetRecycler(), DescriptorMap, descSize);

        PropertyId propId;
        PropertyRecord const * propertyRecord;
        JavascriptString* propertyName = nullptr;

        //enumerate through each property of properties and fetch the property descriptor
        while ((propertyName = enumerator.MoveAndGetNext(propId)) != NULL)
        {
            if (propId == Constants::NoProperty) //try current property id query first
            {
                scriptContext->GetOrAddPropertyRecord(propertyName->GetString(), propertyName->GetLength(), &propertyRecord);
                propId = propertyRecord->GetPropertyId();
            }
            else
            {
                propertyRecord = scriptContext->GetPropertyName(propId);
            }

            if (descCount == descSize)
            {
                //reallocate - consider linked list of DescriptorMap if the descSize is too high
                descSize = AllocSizeMath::Mul(descCount, 2);
                __analysis_assume(descSize == descCount * 2);
                DescriptorMap *temp = RecyclerNewArray(scriptContext->GetRecycler(), DescriptorMap, descSize);

                for (size_t i = 0; i < descCount; i++)
                {
                    temp[i] = descriptors[i];
                }
                descriptors = temp;
            }

            Var tempVar = JavascriptOperators::GetProperty(props, propId, scriptContext);

            AnalysisAssert(descCount < descSize);
            if (!JavascriptOperators::ToPropertyDescriptor(tempVar, &descriptors[descCount].descriptor, scriptContext))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_PropertyDescriptor_Invalid, scriptContext->GetPropertyName(propId)->GetBuffer());
            }
            // In proxy, we need to get back the original ToPropertDescriptor var in [[defineProperty]] trap.
            descriptors[descCount].originalVar = tempVar;

            if (CONFIG_FLAG(UseFullName))
            {
                ModifyGetterSetterFuncName(propertyRecord, descriptors[descCount].descriptor, scriptContext);
            }

            descriptors[descCount].propRecord = propertyRecord;

            descCount++;
        }

        //Once all the property descriptors are in place set each property descriptor to the object
        for (size_t i = 0; i < descCount; i++)
        {
            DefineOwnPropertyHelper(object, descriptors[i].propRecord->GetPropertyId(), descriptors[i].descriptor, scriptContext);
        }

        LEAVE_PINNED_SCOPE();

        return object;
    }

    //ES5 15.2.3.7
    Var JavascriptObject::DefinePropertiesHelperForProxyObjects(RecyclableObject *object, RecyclableObject* props, ScriptContext *scriptContext)
    {
        Assert(JavascriptProxy::Is(props));

        //1.  If Type(O) is not Object throw a TypeError exception.
        //2.  Let props be ToObject(Properties).

        size_t descCount = 0;
        struct DescriptorMap
        {
            Field(PropertyRecord const *) propRecord;
            Field(PropertyDescriptor) descriptor;
        };

        //3.  Let keys be props.[[OwnPropertyKeys]]().
        //4.  ReturnIfAbrupt(keys).
        //5.  Let descriptors be an empty List.
        JavascriptArray* keys = JavascriptOperators::GetOwnEnumerablePropertyNamesSymbols(props, scriptContext);
        uint32 length = keys->GetLength();

        ENTER_PINNED_SCOPE(DescriptorMap, descriptors);
        descriptors = RecyclerNewArray(scriptContext->GetRecycler(), DescriptorMap, length);

        //6.  Repeat for each element nextKey of keys in List order,
        //    1.  Let propDesc be props.[[GetOwnProperty]](nextKey).
        //    2.  ReturnIfAbrupt(propDesc).
        //    3.  If propDesc is not undefined and propDesc.[[Enumerable]] is true, then
        //        1.  Let descObj be Get(props, nextKey).
        //        2.  ReturnIfAbrupt(descObj).
        //        3.  Let desc be ToPropertyDescriptor(descObj).
        //        4.  ReturnIfAbrupt(desc).
        //        5.  Append the pair(a two element List) consisting of nextKey and desc to the end of descriptors.
        Var nextKey;

        const PropertyRecord* propertyRecord = nullptr;
        PropertyId propertyId;
        Var descObj;
        for (uint32 j = 0; j < length; j++)
        {
            PropertyDescriptor propertyDescriptor;
            nextKey = keys->DirectGetItem(j);
            AssertMsg(JavascriptSymbol::Is(nextKey) || JavascriptString::Is(nextKey), "Invariant check during ownKeys proxy trap should make sure we only get property key here. (symbol or string primitives)");
            JavascriptConversion::ToPropertyKey(nextKey, scriptContext, &propertyRecord);
            propertyId = propertyRecord->GetPropertyId();
            AssertMsg(propertyId != Constants::NoProperty, "DefinePropertiesHelper - OwnPropertyKeys returned a propertyId with value NoProperty.");

            if (JavascriptOperators::GetOwnPropertyDescriptor(props, propertyRecord->GetPropertyId(), scriptContext, &propertyDescriptor))
            {
                if (propertyDescriptor.IsEnumerable())
                {
                    descObj = JavascriptOperators::GetProperty(props, propertyId, scriptContext);

                    if (!JavascriptOperators::ToPropertyDescriptor(descObj, &descriptors[descCount].descriptor, scriptContext))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_PropertyDescriptor_Invalid, scriptContext->GetPropertyName(propertyId)->GetBuffer());
                    }

                    if (CONFIG_FLAG(UseFullName))
                    {
                        ModifyGetterSetterFuncName(propertyRecord, descriptors[descCount].descriptor, scriptContext);
                    }

                    descriptors[descCount].propRecord = propertyRecord;

                    descCount++;
                }
            }
        }

        //7.  For each pair from descriptors in list order,
        //    1.  Let P be the first element of pair.
        //    2.  Let desc be the second element of pair.
        //    3.  Let status be DefinePropertyOrThrow(O, P, desc).
        //    4.  ReturnIfAbrupt(status).

        for (size_t i = 0; i < descCount; i++)
        {
            DefineOwnPropertyHelper(object, descriptors[i].propRecord->GetPropertyId(), descriptors[i].descriptor, scriptContext);
        }

        LEAVE_PINNED_SCOPE();

        //8.  Return O.
        return object;
    }

    Var JavascriptObject::GetPrototypeOf(RecyclableObject* obj, ScriptContext* scriptContext)
    {
        return obj->IsExternal() ? obj->GetConfigurablePrototype(scriptContext) : obj->GetPrototype();
    }

    //
    // Check if "proto" is a prototype of "object" (on its prototype chain).
    //
    bool JavascriptObject::IsPrototypeOf(RecyclableObject* proto, RecyclableObject* object, ScriptContext* scriptContext)
    {
        return JavascriptOperators::MapObjectAndPrototypesUntil<false>(object, [=](RecyclableObject* obj)
        {
            return obj == proto;
        });
    }

    static const size_t ConstructNameGetSetLength = 5;    // 5 = 1 ( for .) + 3 (get or set) + 1 for null)

    /*static*/
    char16 * JavascriptObject::ConstructName(const PropertyRecord * propertyRecord, const char16 * getOrSetStr, ScriptContext* scriptContext)
    {
        Assert(propertyRecord);
        Assert(scriptContext);
        char16 * finalName = nullptr;
        size_t propertyLength = (size_t)propertyRecord->GetLength();
        if (propertyLength > 0)
        {
            size_t totalChars;
            if (SizeTAdd(propertyLength, ConstructNameGetSetLength, &totalChars) == S_OK)
            {
                finalName = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, totalChars);
                Assert(finalName != nullptr);
                const char16* propertyName = propertyRecord->GetBuffer();
                Assert(propertyName != nullptr);
                wcscpy_s(finalName, totalChars, propertyName);

                Assert(getOrSetStr != nullptr);
                Assert(wcslen(getOrSetStr) == 4);

                wcscpy_s(finalName + propertyLength, ConstructNameGetSetLength, getOrSetStr);
            }
        }
        return finalName;
    }

    /*static*/
    void JavascriptObject::ModifyGetterSetterFuncName(const PropertyRecord * propertyRecord, const PropertyDescriptor& descriptor, ScriptContext* scriptContext)
    {
        Assert(scriptContext);
        Assert(propertyRecord);
        if (descriptor.GetterSpecified() || descriptor.SetterSpecified())
        {
            charcount_t propertyLength = propertyRecord->GetLength();

            if (descriptor.GetterSpecified()
                && Js::ScriptFunction::Is(descriptor.GetGetter())
                && _wcsicmp(Js::ScriptFunction::FromVar(descriptor.GetGetter())->GetFunctionProxy()->GetDisplayName(), _u("get")) == 0)
            {
                // modify to name.get
                const char16* finalName = ConstructName(propertyRecord, _u(".get"), scriptContext);
                if (finalName != nullptr)
                {
                    FunctionProxy::SetDisplayNameFlags flags = (FunctionProxy::SetDisplayNameFlags) (FunctionProxy::SetDisplayNameFlagsDontCopy | FunctionProxy::SetDisplayNameFlagsRecyclerAllocated);

                    Js::ScriptFunction::FromVar(descriptor.GetGetter())->GetFunctionProxy()->SetDisplayName(finalName,
                        propertyLength + 4 /*".get"*/, propertyLength + 1, flags);
                }
            }

            if (descriptor.SetterSpecified()
                && Js::ScriptFunction::Is(descriptor.GetSetter())
                && _wcsicmp(Js::ScriptFunction::FromVar(descriptor.GetSetter())->GetFunctionProxy()->GetDisplayName(), _u("set")) == 0)
            {
                // modify to name.set
                const char16* finalName = ConstructName(propertyRecord, _u(".set"), scriptContext);
                if (finalName != nullptr)
                {
                    FunctionProxy::SetDisplayNameFlags flags = (FunctionProxy::SetDisplayNameFlags) (FunctionProxy::SetDisplayNameFlagsDontCopy | FunctionProxy::SetDisplayNameFlagsRecyclerAllocated);

                    Js::ScriptFunction::FromVar(descriptor.GetSetter())->GetFunctionProxy()->SetDisplayName(finalName,
                        propertyLength + 4 /*".set"*/, propertyLength + 1, flags);
                }
            }
        }
    }

    BOOL JavascriptObject::DefineOwnPropertyHelper(RecyclableObject* obj, PropertyId propId, const PropertyDescriptor& descriptor, ScriptContext* scriptContext, bool throwOnError /* = true*/)
    {
        BOOL returnValue;
        obj->ThrowIfCannotDefineProperty(propId, descriptor);

        const Type* oldType = obj->GetType();
        obj->ClearWritableDataOnlyDetectionBit();

        // HostDispatch: it doesn't support changing property attributes and default attributes are not per ES5,
        // so there is no benefit in using ES5 DefineOwnPropertyDescriptor for it, use old implementation.
        if (TypeIds_HostDispatch != obj->GetTypeId())
        {
            if (DynamicObject::IsAnyArray(obj))
            {
                returnValue = JavascriptOperators::DefineOwnPropertyForArray(
                    JavascriptArray::FromAnyArray(obj), propId, descriptor, throwOnError, scriptContext);
            }
            else
            {
                returnValue = JavascriptOperators::DefineOwnPropertyDescriptor(obj, propId, descriptor, throwOnError, scriptContext);
                if (propId == PropertyIds::__proto__)
                {
                    scriptContext->GetLibrary()->GetObjectPrototypeObject()->PostDefineOwnProperty__proto__(obj);
                }
            }
        }
        else
        {
            returnValue = JavascriptOperators::SetPropertyDescriptor(obj, propId, descriptor);
        }

        if (propId == PropertyIds::_symbolSpecies && obj == scriptContext->GetLibrary()->GetArrayConstructor())
        {
            scriptContext->GetLibrary()->SetArrayObjectHasUserDefinedSpecies(true);
        }

        if (obj->IsWritableDataOnlyDetectionBitSet())
        {
            if (obj->GetType() == oldType)
            {
                // Also, if the object's type has not changed, we need to ensure that
                // the cached property string for this property, if any, does not
                // specify this object's type.
                scriptContext->InvalidatePropertyStringCache(propId, obj->GetType());
            }
        }

        if (descriptor.IsAccessorDescriptor())
        {
            scriptContext->optimizationOverrides.SetSideEffects(Js::SideEffects_Accessor);
        }
        return returnValue;
    }
}

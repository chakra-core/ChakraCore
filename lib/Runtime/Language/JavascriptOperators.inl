//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    __forceinline TypeId JavascriptOperators::GetTypeId(const Var aValue)
    {
        AssertMsg(aValue != nullptr, "GetTypeId aValue is null");

        if (TaggedInt::Is(aValue))
        {
            return TypeIds_Integer;
        }
#if FLOATVAR
        else if (JavascriptNumber::Is_NoTaggedIntCheck(aValue))
        {
            return TypeIds_Number;
        }
#endif
        else
        {
            auto typeId = RecyclableObject::FromVar(aValue)->GetTypeId();
#if DBG
            auto isExternal = RecyclableObject::FromVar(aValue)->CanHaveInterceptors();
            AssertMsg(typeId < TypeIds_Limit || isExternal, "GetTypeId aValue has invalid TypeId");
#endif
            return typeId;
        }
    }

    __forceinline TypeId JavascriptOperators::GetTypeIdNoCheck(const Var aValue)
    {
        AssertMsg(aValue != nullptr, "GetTypeId aValue is null");

        if (TaggedInt::Is(aValue))
        {
            return TypeIds_Integer;
        }
#if FLOATVAR
        else if (JavascriptNumber::Is_NoTaggedIntCheck(aValue))
        {
            return TypeIds_Number;
        }
#endif
        else
        {
            auto typeId = RecyclableObject::FromVar(aValue)->GetTypeId();
            return typeId;
        }
    }

    // A helper function which will do the IteratorStep and fetch value - however in the event of an exception it will perform the IteratorClose as well.
    template <typename THandler>
    void JavascriptOperators::DoIteratorStepAndValue(RecyclableObject* iterator, ScriptContext* scriptContext, THandler handler)
    {
        Var nextItem = nullptr;
        bool shouldCallReturn = false;
        try
        {
            while (JavascriptOperators::IteratorStepAndValue(iterator, scriptContext, &nextItem))
            {
                shouldCallReturn = true;
                handler(nextItem);
                shouldCallReturn = false;
            }
        }
        catch (const JavascriptException& err)
        {
            JavascriptExceptionObject * exceptionObj = err.GetAndClear();
            if (shouldCallReturn)
            {
                // Closing the iterator
                JavascriptOperators::IteratorClose(iterator, scriptContext);
            }
            JavascriptExceptionOperators::DoThrow(exceptionObj, scriptContext);
        }
    }

    template <BOOL stopAtProxy, class Func>
    void JavascriptOperators::MapObjectAndPrototypes(RecyclableObject* object, Func func)
    {
        MapObjectAndPrototypesUntil<stopAtProxy>(object, [=](RecyclableObject* obj)
        {
            func(obj);
            return false; // this will map whole prototype chain
        });
    }

    template <BOOL stopAtProxy, class Func>
    bool JavascriptOperators::MapObjectAndPrototypesUntil(RecyclableObject* object, Func func)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(object);
        while (typeId != TypeIds_Null && (!stopAtProxy || typeId != TypeIds_Proxy))
        {
            if (func(object))
            {
                return true;
            }

            object = object->GetPrototype();
            typeId = JavascriptOperators::GetTypeId(object);
        }

        return false;
    }

    // Checks to see if any object in the prototype chain has a property descriptor for the given property
    // that specifies either an accessor or a non-writable attribute.
    // If TRUE, check flags for details.
    template<typename PropertyKeyType, bool doFastProtoChainCheck, bool isRoot>
    BOOL JavascriptOperators::CheckPrototypesForAccessorOrNonWritablePropertyCore(RecyclableObject* instance,
        PropertyKeyType propertyKey, Var* setterValue, DescriptorFlags* flags, PropertyValueInfo* info, ScriptContext* scriptContext)
    {
        Assert(setterValue);
        Assert(flags);

        // Do a quick check to see if all objects in the prototype chain are known to have only
        // writable data properties (i.e. no accessors or non-writable properties).
        if (doFastProtoChainCheck && CheckIfObjectAndPrototypeChainHasOnlyWritableDataProperties(instance))
        {
            return FALSE;
        }

        if (isRoot)
        {
            *flags = JavascriptOperators::GetRootSetter(instance, propertyKey, setterValue, info, scriptContext);
        }
        if (*flags == None)
        {
            *flags = JavascriptOperators::GetterSetter(instance, propertyKey, setterValue, info, scriptContext);
        }

        return ((*flags & Accessor) == Accessor) || ((*flags & Proxy) == Proxy)|| ((*flags & Data) == Data && (*flags & Writable) == None);
    }

    template<typename PropertyKeyType>
    BOOL JavascriptOperators::CheckPrototypesForAccessorOrNonWritablePropertySlow(RecyclableObject* instance, PropertyKeyType propertyKey, Var* setterValue, DescriptorFlags* flags, bool isRoot, ScriptContext* scriptContext)
    {
        // This is used in debug verification, do not doFastProtoChainCheck to avoid side effect (doFastProtoChainCheck may update HasWritableDataOnly flags).
        if (isRoot)
        {
            return CheckPrototypesForAccessorOrNonWritablePropertyCore<PropertyKeyType, /*doFastProtoChainCheck*/false, true>(instance, propertyKey, setterValue, flags, nullptr, scriptContext);
        }
        else
        {
            return CheckPrototypesForAccessorOrNonWritablePropertyCore<PropertyKeyType, /*doFastProtoChainCheck*/false, false>(instance, propertyKey, setterValue, flags, nullptr, scriptContext);
        }
    }
}

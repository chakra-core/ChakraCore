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
        catch (JavascriptExceptionObject * exceptionObj)
        {
            if (shouldCallReturn)
            {
                // Closing the iterator
                JavascriptOperators::IteratorClose(iterator, scriptContext);
            }
            throw exceptionObj;
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

}

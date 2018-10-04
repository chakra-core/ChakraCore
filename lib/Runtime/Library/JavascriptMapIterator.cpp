//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptMapIterator::JavascriptMapIterator(DynamicType* type, JavascriptMap* map, JavascriptMapIteratorKind kind):
        DynamicObject(type),
        m_map(map),
        m_mapIterator(map->GetIterator()),
        m_kind(kind)
    {
        Assert(type->GetTypeId() == TypeIds_MapIterator);
    }

    Var JavascriptMapIterator::EntryNext(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        Assert(!(callInfo.Flags & CallFlags_New));

        Var thisObj = args[0];

        if (!VarIs<JavascriptMapIterator>(thisObj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedMapIterator, _u("Map Iterator.prototype.next"));
        }

        JavascriptMapIterator* iterator = VarTo<JavascriptMapIterator>(thisObj);
        JavascriptMap* map = iterator->m_map;
        auto& mapIterator = iterator->m_mapIterator;

        if (map == nullptr || !mapIterator.Next())
        {
            iterator->m_map = nullptr;
            return library->CreateIteratorResultObjectUndefinedTrue();
        }

        auto entry = mapIterator.Current();
        Var result;

        if (iterator->m_kind == JavascriptMapIteratorKind::KeyAndValue)
        {
            JavascriptArray* keyValueTuple = library->CreateArray(2);
            keyValueTuple->SetItem(0, entry.Key(), PropertyOperation_None);
            keyValueTuple->SetItem(1, entry.Value(), PropertyOperation_None);
            result = keyValueTuple;
        }
        else if (iterator->m_kind == JavascriptMapIteratorKind::Key)
        {
            result = entry.Key();
        }
        else
        {
            Assert(iterator->m_kind == JavascriptMapIteratorKind::Value);
            result = entry.Value();
        }

        return library->CreateIteratorResultObjectValueFalse(result);
    }
} //namespace Js

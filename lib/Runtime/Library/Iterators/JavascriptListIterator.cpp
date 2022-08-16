//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptListIterator::JavascriptListIterator(DynamicType* type, ListForListIterator* list) :
        DynamicObject(type),
        listForIterator(list),
        index(0)
    {
        Assert(type->GetTypeId() == TypeIds_ListIterator);
        count = list->Count();
    }

    Var JavascriptListIterator::EntryNext(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        Assert(!(callInfo.Flags & CallFlags_New));

        Var thisObj = args[0];

        if (!VarIs<JavascriptListIterator>(thisObj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedListIterator, _u("ListIterator.next"));
        }

        JavascriptListIterator* iterator = VarTo<JavascriptListIterator>(thisObj);
        ListForListIterator* list = iterator->listForIterator;

        if (list == nullptr)
        {
            return library->CreateIteratorResultObjectDone();
        }

        if (iterator->index >= iterator->count)
        {
            // Nulling out the listForIterator field is important so that the iterator
            // does not keep the list alive after iteration is completed.
            iterator->listForIterator = nullptr;
            return library->CreateIteratorResultObjectDone();
        }

        Var current = list->Item(iterator->index);

        iterator->index++;

        return library->CreateIteratorResultObject(current);
    }
} // namespace Js

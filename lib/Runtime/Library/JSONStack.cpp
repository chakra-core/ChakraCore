//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "JSONStack.h"
namespace JSON
{
#if defined(_JS_VALUE) || (_DOM_VALUE)
#error "Something went wrong!"
#endif

#define _JS_VALUE tempValues[0]
#define _DOM_VALUE   tempValues[1]

    bool StrictEqualsObjectComparer::Equals(Js::Var x, Js::Var y)
    {
        return JSONStack::Equals(x,y);
    }

    JSONStack::JSONStack(ArenaAllocator *allocator, Js::ScriptContext *context)
      : jsObjectStack(allocator), domObjectStack(nullptr),
        alloc(allocator), scriptContext(context)
    {
        // most of the time, the size of the stack is 1 object.
        // use direct member instead of expensive Push / Pop / Has
        _JS_VALUE = nullptr;
        _DOM_VALUE = nullptr;
    }

    bool JSONStack::Equals(Js::Var x, Js::Var y)
    {
        return Js::JavascriptOperators::StrictEqual(x, y, ((Js::RecyclableObject *)x)->GetScriptContext()) == TRUE;
    }

    bool JSONStack::Has(Js::Var data, bool bJsObject) const
    {
        if (bJsObject)
        {
            if (_JS_VALUE)
            {
                return (data == _JS_VALUE);
            }
            return jsObjectStack.Has(data);
        }
        else if (domObjectStack)
        {
            if (_DOM_VALUE)
            {
                return (data == _DOM_VALUE);
            }
            return domObjectStack->Contains(data);
        }
        return false;
    }

    bool JSONStack::Push(Js::Var data, bool bJsObject)
    {
        if (bJsObject)
        {
            bool result = true;
            if (_JS_VALUE)
            {
                jsObjectStack.Push(_JS_VALUE);
                _JS_VALUE = nullptr;
            }

            if (jsObjectStack.Count())
            {
                result = jsObjectStack.Push(data);
            }
            else
            {
                _JS_VALUE = data;
            }

            return result;
        }

        EnsuresDomObjectStack();

        if (_DOM_VALUE)
        {
            domObjectStack->Add(_DOM_VALUE);
            _DOM_VALUE = nullptr;
        }

        if (domObjectStack->Count())
        {
            domObjectStack->Add(data);
        }
        else
        {
            _DOM_VALUE = data;
        }
        return true;
    }

    void JSONStack::Pop(bool bJsObject)
    {
        if (bJsObject)
        {
            if (_JS_VALUE)
            {
                _JS_VALUE = nullptr;
            }
            else
            {
                jsObjectStack.Pop();
            }
            return;
        }
        AssertMsg(domObjectStack != NULL, "Misaligned pop");

        if (_DOM_VALUE)
        {
            _DOM_VALUE = nullptr;
        }
        else
        {
            domObjectStack->RemoveAtEnd();
        }
    }

    void JSONStack::EnsuresDomObjectStack(void)
    {
        if (!domObjectStack)
        {
            domObjectStack = DOMObjectStack::New(alloc);
        }
    }
} // namespace JSON

#undef _JS_VALUE
#undef _DOM_VALUE

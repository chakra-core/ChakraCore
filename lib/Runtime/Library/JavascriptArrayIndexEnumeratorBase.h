//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptArrayIndexEnumeratorBase _ABSTRACT : public JavascriptEnumerator
    {
    protected:
        JavascriptArray* arrayObject;
        uint32 index;
        bool doneArray;
        EnumeratorFlags flags;

        DEFINE_VTABLE_CTOR_ABSTRACT(JavascriptArrayIndexEnumeratorBase, JavascriptEnumerator)

        JavascriptArrayIndexEnumeratorBase(JavascriptArray* arrayObject, EnumeratorFlags flags, ScriptContext* scriptContext);
        virtual uint32 GetCurrentItemIndex()  override { return index; }
    };
}

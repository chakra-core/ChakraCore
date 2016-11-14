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
        Field(JavascriptArray*) arrayObject;
        Field(uint32) index;
        Field(bool) doneArray;
        Field(EnumeratorFlags) flags;

        DEFINE_VTABLE_CTOR_ABSTRACT(JavascriptArrayIndexEnumeratorBase, JavascriptEnumerator)

        JavascriptArrayIndexEnumeratorBase(JavascriptArray* arrayObject, EnumeratorFlags flags, ScriptContext* scriptContext);
        virtual uint32 GetCurrentItemIndex()  override { return index; }
    };
}

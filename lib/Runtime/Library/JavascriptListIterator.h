//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptListIterator : public DynamicObject
    {
    private:
        Field(ListForListIterator*) listForIterator;
        Field(uint) index;
        Field(uint) count;

    protected:
        DEFINE_VTABLE_CTOR(JavascriptListIterator, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptListIterator);

    public:
        JavascriptListIterator(DynamicType* type, ListForListIterator* list);

        static bool Is(Var aValue);
        static JavascriptListIterator* FromVar(Var aValue);

        class EntryInfo
        {
        public:
            static FunctionInfo Next;
        };

        static Var EntryNext(RecyclableObject* function, CallInfo callInfo, ...);

    };
} // namespace Js


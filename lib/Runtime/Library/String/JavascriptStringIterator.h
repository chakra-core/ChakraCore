//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptStringIterator : public DynamicObject
    {
    private:
        Field(JavascriptString*)   m_string;
        Field(charcount_t)         m_nextIndex;

    protected:
        DEFINE_VTABLE_CTOR(JavascriptStringIterator, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptStringIterator);

    public:
        JavascriptStringIterator(DynamicType* type, JavascriptString* string);

        class EntryInfo
        {
        public:
            static FunctionInfo Next;
        };

        static Var EntryNext(RecyclableObject* function, CallInfo callInfo, ...);

    public:
        JavascriptString* GetStringForHeapEnum() { return m_string; }
    };

    template <> inline bool VarIsImpl<JavascriptStringIterator>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_StringIterator;
    }
} // namespace Js

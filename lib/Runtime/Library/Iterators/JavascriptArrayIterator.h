//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    enum class JavascriptArrayIteratorKind
    {
        Key,
        Value,
        KeyAndValue,
    };

    class JavascriptArrayIterator : public DynamicObject
    {
    private:
        Field(Var)                         m_iterableObject;
        Field(int64)                       m_nextIndex;
        Field(JavascriptArrayIteratorKind) m_kind;

    protected:
        DEFINE_VTABLE_CTOR(JavascriptArrayIterator, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptArrayIterator);

    public:
        JavascriptArrayIterator(DynamicType* type, Var iterable, JavascriptArrayIteratorKind kind);

        class EntryInfo
        {
        public:
            static FunctionInfo Next;
        };

        static Var EntryNext(RecyclableObject* function, CallInfo callInfo, ...);

    public:
        Var GetIteratorObjectForHeapEnum() { return m_iterableObject; }
    };

    template <> inline bool VarIsImpl<JavascriptArrayIterator>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_ArrayIterator;
    }
} // namespace Js

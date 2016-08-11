//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptStringEnumerator : public JavascriptEnumerator
    {
    private:
        JavascriptString* stringObject;
        int index;
    protected:
        DEFINE_VTABLE_CTOR(JavascriptStringEnumerator, JavascriptEnumerator);
        DEFINE_MARSHAL_ENUMERATOR_TO_SCRIPT_CONTEXT(JavascriptStringEnumerator);

    public:
        JavascriptStringEnumerator(JavascriptString* stringObject, ScriptContext * requestContext);
        virtual void Reset() override;
        virtual Var MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };

    class JavascriptStringObjectEnumerator : public JavascriptEnumerator
    {
    private:
        JavascriptStringEnumerator* stringEnumerator;
        JavascriptStringObject* stringObject;
        JavascriptEnumerator* objectEnumerator;
        BOOL enumNonEnumerable;
        bool enumSymbols;

    protected:
        DEFINE_VTABLE_CTOR(JavascriptStringObjectEnumerator, JavascriptEnumerator);
        DEFINE_MARSHAL_ENUMERATOR_TO_SCRIPT_CONTEXT(JavascriptStringObjectEnumerator);

    public:
        JavascriptStringObjectEnumerator(JavascriptStringObject* stringObject, ScriptContext * requestContext, BOOL enumNonEnumerable, bool enumSymbols = false);
        virtual void Reset() override;
        virtual Var MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };
}

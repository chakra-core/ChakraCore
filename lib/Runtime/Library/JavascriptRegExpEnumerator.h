//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptRegExpEnumerator : public JavascriptEnumerator
    {
    private:
        JavascriptRegExpConstructor* regExpObject;
        uint index;
        EnumeratorFlags flags;
    protected:
        DEFINE_VTABLE_CTOR(JavascriptRegExpEnumerator, JavascriptEnumerator);

    public:
        JavascriptRegExpEnumerator(JavascriptRegExpConstructor* regExpObject, EnumeratorFlags flags, ScriptContext * requestContext);
        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };
}

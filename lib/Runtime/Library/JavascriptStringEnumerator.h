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
        Field(JavascriptString*) stringObject;
        Field(int) index;
    protected:
        DEFINE_VTABLE_CTOR(JavascriptStringEnumerator, JavascriptEnumerator);

    public:
        JavascriptStringEnumerator(JavascriptString* stringObject, ScriptContext * requestContext);
        virtual void Reset() override;
        virtual JavascriptString * MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };
}

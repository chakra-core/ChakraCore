//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptArrayIndexEnumerator : public JavascriptArrayIndexEnumeratorBase
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptArrayIndexEnumerator, JavascriptArrayIndexEnumeratorBase);

    public:
        JavascriptArrayIndexEnumerator(JavascriptArray* arrayObject, EnumeratorFlags flags, ScriptContext* scriptContext);
        virtual void Reset() override;
        virtual JavascriptString * MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };
}

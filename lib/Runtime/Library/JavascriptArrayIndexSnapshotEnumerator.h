//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptArrayIndexSnapshotEnumerator : public JavascriptArrayIndexEnumeratorBase
    {
    private:
        Field(uint32) initialLength;

    protected:
        DEFINE_VTABLE_CTOR(JavascriptArrayIndexSnapshotEnumerator, JavascriptArrayIndexEnumeratorBase);

    public:
        JavascriptArrayIndexSnapshotEnumerator(JavascriptArray* arrayObject, EnumeratorFlags flags, ScriptContext* scriptContext);
        virtual void Reset() override;
        virtual JavascriptString * MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr);
    };
}

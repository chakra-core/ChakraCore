//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class TypedArrayIndexEnumerator : public JavascriptEnumerator
    {
    private:
        Field(TypedArrayBase*) typedArrayObject;
        Field(uint32) index;
        Field(bool) doneArray;
        Field(EnumeratorFlags) flags;

    protected:
        DEFINE_VTABLE_CTOR(TypedArrayIndexEnumerator, JavascriptEnumerator);

    public:
        TypedArrayIndexEnumerator(TypedArrayBase* typeArrayBase, EnumeratorFlags flags, ScriptContext* scriptContext);
        virtual JavascriptString * MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
        virtual void Reset() override;
    };
}

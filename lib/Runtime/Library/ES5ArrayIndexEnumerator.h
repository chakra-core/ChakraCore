//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ES5ArrayIndexEnumerator : public JavascriptArrayIndexEnumeratorBase
    {
    private:
        Field(uint32) initialLength;                   // The initial array length when this enumerator is created
        Field(uint32) dataIndex;                       // Current data index
        Field(uint32) descriptorIndex;                 // Current descriptor index
        Field(IndexPropertyDescriptor*) descriptor;    // Current descriptor
        Field(void *) descriptorValidationToken;
    protected:
        DEFINE_VTABLE_CTOR(ES5ArrayIndexEnumerator, JavascriptArrayIndexEnumeratorBase);

    private:
        ES5Array* GetArray() const { return ES5Array::FromVar(arrayObject); }

    public:
        ES5ArrayIndexEnumerator(ES5Array* arrayObject, EnumeratorFlags flags, ScriptContext* scriptContext);
        virtual void Reset() override;
        virtual JavascriptString * MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };
}

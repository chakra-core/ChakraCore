//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptBigIntObject : public DynamicObject
    {
    private:
        Field(JavascriptBigInt*) value;

        DEFINE_VTABLE_CTOR(JavascriptBigIntObject, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptBigIntObject);
    public:
        JavascriptBigIntObject(JavascriptBigInt* value, DynamicType * type);

        JavascriptBigInt* GetValue() const;
    };

    template <> inline bool VarIsImpl<JavascriptBigIntObject>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_BigIntObject;
    }
}

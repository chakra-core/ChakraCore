//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptBigIntObject::JavascriptBigIntObject(JavascriptBigInt* value, DynamicType * type)
        : DynamicObject(type), value(value)
    {
        Assert(type->GetTypeId() == TypeIds_BigIntObject);
    }

    JavascriptBigInt* JavascriptBigIntObject::GetValue() const
    {
        return this->value;
    }
} // namespace Js

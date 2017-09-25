//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class LiteralString : public JavascriptString
    {
    protected:
        LiteralString(StaticType* type);
        LiteralString(StaticType* type, const char16* content, charcount_t charLength);
        DEFINE_VTABLE_CTOR(LiteralString, JavascriptString);

    public:
        static LiteralString* New(StaticType* type, const char16* content, charcount_t charLength, Recycler* recycler);
        static LiteralString* CreateEmptyString(StaticType* type);
    };
}

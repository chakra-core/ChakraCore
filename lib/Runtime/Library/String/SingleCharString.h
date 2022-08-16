//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // A JavascriptString containing a single char.
    // Caution: GetString and GetSz return interior pointers.
    // So, if allocated in Recycler memory, these objects must
    // remain pinned to prevent orphaning via interior pointers (GetString, GetSz)
    class SingleCharString sealed : public JavascriptString
    {
    public:
        static SingleCharString* New(char16 ch, ScriptContext* scriptContext);
        static SingleCharString* New(char16 ch, ScriptContext* scriptContext, ArenaAllocator* arena);

        virtual void const * GetOriginalStringReference() override;

    protected:
        DEFINE_VTABLE_CTOR(SingleCharString, JavascriptString);

    private:
        SingleCharString(char16 ch, StaticType * type);
        Field(char16) m_buff[2] = { 0 }; // the 2nd is always NULL so that GetSz works
    };

} // namespace Js

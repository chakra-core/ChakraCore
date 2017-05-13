//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class InternalString
    {
        Field(charcount_t) m_charLength;
        Field(unsigned char) m_offset;
        Field(const char16*) m_content;

    public:
        InternalString() : m_charLength(0), m_content(nullptr), m_offset(0) { };
        InternalString(const char16* content, DECLSPEC_GUARD_OVERFLOW charcount_t charLength, unsigned char offset = 0);
        InternalString(const char16* content, _no_write_barrier_tag, DECLSPEC_GUARD_OVERFLOW charcount_t charLength, unsigned char offset = 0);

        static InternalString* New(ArenaAllocator* alloc, const char16* content, DECLSPEC_GUARD_OVERFLOW charcount_t length);
        static InternalString* New(Recycler* recycler, const char16* content, DECLSPEC_GUARD_OVERFLOW charcount_t length);
        static InternalString* NewNoCopy(ArenaAllocator* alloc, const char16* content, DECLSPEC_GUARD_OVERFLOW charcount_t length);

        inline charcount_t GetLength() const
        {
            return m_charLength;
        }

        inline const char16* GetBuffer() const
        {
            return m_content + m_offset;
        }
    };

    struct InternalStringComparer
    {
        inline static bool Equals(InternalString const& str1, InternalString const& str2)
        {
            return str1.GetLength() == str2.GetLength() &&
                JsUtil::CharacterBuffer<char16>::StaticEquals(str1.GetBuffer(), str2.GetBuffer(), str1.GetLength());
        }

        inline static hash_t GetHashCode(InternalString const& str)
        {
            return JsUtil::CharacterBuffer<char16>::StaticGetHashCode(str.GetBuffer(), str.GetLength());
        }
    };
}

template<>
struct DefaultComparer<Js::InternalString> : public Js::InternalStringComparer {};

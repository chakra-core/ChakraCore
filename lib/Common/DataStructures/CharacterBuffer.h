//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace JsUtil
{
    static const charcount_t MAX_FAST_HASH_LENGTH = 256;

    // A buffer of characters, may have embedded null.
    template <typename T>
    class CharacterBuffer
    {
    public:
        CharacterBuffer() : string(nullptr), len((charcount_t)-1) {}
        CharacterBuffer(T const * string, charcount_t len) : string(string), len(len) {}
        CharacterBuffer(const CharacterBuffer& other) : string(other.string), len(other.len) {}

        bool operator==(CharacterBuffer const& other) const
        {
            Assert(string != nullptr);
            if (this->len != other.len)
            {
                return false;
            }
            return this->string == other.string || StaticEquals(string, other.string, this->len);
        }

        operator hash_t() const
        {
            Assert(string != nullptr);
            return StaticGetHashCode(string, len);
        }

        hash_t FastHash() const
        {
            Assert(string != nullptr);
            return InternalGetHashCode<true>(string, len);
        }

        CharacterBuffer& operator=(T const * s)
        {
            Assert(s == nullptr);
            string = nullptr;
            len = (charcount_t)-1;
            return *this;
        }

        static bool StaticEquals(__in_z T const * s1, __in_z T const* s2, _In_ charcount_t length);

        static hash_t StaticGetHashCode(__in_z T const * s, _In_ charcount_t length)
        {
            return InternalGetHashCode<false>(s, length);
        }

        template <bool fastHash>
        static hash_t InternalGetHashCode(__in_z T const * s, _In_ charcount_t length)
        {
            hash_t hash = CC_HASH_OFFSET_VALUE;
            charcount_t hashLength = length;
            if (fastHash)
            {
                hashLength = min(length, MAX_FAST_HASH_LENGTH);
            }
            for (charcount_t i = 0; i < hashLength; i++)
            {
                CC_HASH_LOGIC(hash, s[i]);
            }
            return hash;
        }

        T const * GetBuffer() const { return string; }
        charcount_t GetLength() const { return len; }
    private:
        Field(T const *) string;
        Field(charcount_t) len;
    };

    template<>
    inline bool
    CharacterBuffer<WCHAR>::StaticEquals(__in_z WCHAR const * s1, __in_z WCHAR const * s2, _In_ charcount_t length)
    {
        return (s1 == s2) || wmemcmp(s1, s2, length) == 0;
    }

    template<>
    inline bool
    CharacterBuffer<unsigned char>::StaticEquals(__in_z unsigned char const * s1, __in_z unsigned char const *s2, _In_ charcount_t length)
    {
        return (s1 == s2) || memcmp(s1, s2, length) == 0;
    }
}

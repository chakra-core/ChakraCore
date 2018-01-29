//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Codex/Utf8Codex.h"

namespace Js
{
    class Utf8String : public JavascriptString
    {
    private:
        typedef struct {
            FieldNoBarrier(size_t) length;
            Field(char*) buffer;
        } PrefixedUtf8String;
        Field(PrefixedUtf8String*) utf8String;

    protected:
        DEFINE_VTABLE_CTOR(Utf8String, JavascriptString);

    private:
        
        void SetUtf8Buffer(_In_reads_(utf8Length) char* buffer, size_t utf8Length)
        {
            this->utf8String = RecyclerNew(this->GetRecycler(), PrefixedUtf8String);
            this->utf8String->length = utf8Length;
            this->utf8String->buffer = buffer;
        }

    public:

        Utf8String(_In_ JavascriptString* originalString, _In_reads_(utf8Length) char* buffer, size_t utf8Length, _In_ StaticType* type) :
            JavascriptString(type),
            utf8String(nullptr)
        {
            SetUtf8Buffer(buffer, utf8Length);

            this->SetLength(originalString->GetLength());
            this->SetBuffer(originalString->UnsafeGetBuffer());
        }

        Utf8String(_In_reads_(utf8Length) char* buffer, size_t utf8Length, _In_ StaticType* type) :
            JavascriptString(type),
            utf8String(nullptr)
        {
            SetUtf8Buffer(buffer, utf8Length);

            charcount_t utf16Length = 0;
            utf8::DecodeOptions opts = utf8::DecodeOptions::doDefault;
            LPCUTF8 buf = reinterpret_cast<LPCUTF8>(buffer);
            LPCUTF8 end = buf + utf8Length;
            while (buf < end)
            {
                if ((*buf & 0x80) == 0)
                {
                    // Single byte character
                    utf16Length++;
                    buf++;
                }
                else
                {
                    // Decode a single utf16 character, and increment the pointer
                    // to the start of the next character.
                    char16 c1 = *buf;
                    ++buf;
                    utf8::DecodeTail(c1, buf, end, opts);
                    utf16Length++;
                }
            }

            this->SetLength(utf16Length);
            this->SetBuffer(nullptr);
        }


        size_t Utf8Length() const
        {
            return this->utf8String->length;
        }

        const char* Utf8Buffer() const
        {
            return this->utf8String->buffer;
        }

        const char16* GetSz() override
        {
            if (this->IsFinalized())
            {
                return this->UnsafeGetBuffer();
            }

            // TODO: This is currently wrong in the presence of unmatched surrogate pairs.
            // They will be converted to a 3 byte replacement character in utf8, and then
            // converted back into a 1 utf16 character 0xfffd representation of that, not the original
            // utf16 surrogate pair character
            char16* buffer = RecyclerNewArrayLeaf(this->GetRecycler(), char16, this->GetLength() + 1);

            // Fetching the char* from the Field(char*) first so we can then cast to LPCUTF8
            const char* bufferStart = this->utf8String->buffer;
            LPCUTF8 start = reinterpret_cast<LPCUTF8>(bufferStart);
            utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(buffer, start, start + this->utf8String->length);

            buffer[this->GetLength()] = 0;

            this->SetBuffer(buffer);
            return this->UnsafeGetBuffer();
        }

        static bool Is(RecyclableObject* obj)
        {
            return VirtualTableInfo<Js::Utf8String>::HasVirtualTable(obj);
        }

        template <typename StringType>
        static Utf8String * ConvertString(StringType * originalString, _In_reads_(utf8Length) char* buffer, size_t utf8Length)
        {
            VirtualTableInfo<Utf8String>::SetVirtualTable(originalString);
            Utf8String * convertedString = (Utf8String *)originalString;

            convertedString->SetUtf8Buffer(buffer, utf8Length);

            // length and buffer are preserved from the original string, since that is part of JavascriptString

            return convertedString;
        }
    };
}
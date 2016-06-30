//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // Used when the final string has a known final length
    class BufferStringBuilder
    {
    public:
        BufferStringBuilder(charcount_t charLength, ScriptContext* scriptContext)
            : m_string( WritableString::New(charLength, scriptContext) )
        {
        }

        JavascriptString* ToString();

        void DbgAssertNotFrozen() const
        {
            AssertMsg(this->m_string != nullptr, "Do not call BufferStringBuilder methods after ToString() has been called.");
        }

        void SetContent(
            const char16* prefix, charcount_t cchPrefix,
            const char16* content, charcount_t cchContent,
            const char16* suffix, charcount_t cchSuffix)
        {
            DbgAssertNotFrozen();
            this->m_string->SetContent(prefix, cchPrefix, content, cchContent, suffix, cchSuffix);
        }

        // Caution: Do not retain the writable buffer after ToString has been called
        char16* DangerousGetWritableBuffer()
        {
            DbgAssertNotFrozen();
            return this->m_string->GetWritableBuffer();
        }

        class WritableString sealed : public JavascriptString
        {
        public:
            static WritableString* New(charcount_t length, ScriptContext* scriptContext);
            char16* GetWritableBuffer() const
            {
                return const_cast< char16* >( this->UnsafeGetBuffer() );
            }

            void SetContent(const char16* content, charcount_t offset, charcount_t length);
            void SetContent(const char16* prefix, charcount_t cchPrefix,
                            const char16* content, charcount_t cchContent,
                            const char16* suffix, charcount_t cchSuffix);

        protected:
            DEFINE_VTABLE_CTOR(WritableString, JavascriptString);
            DECLARE_CONCRETE_STRING_CLASS;

        private:
            WritableString(StaticType * type, charcount_t length, const char16* szValue)
                : JavascriptString(type, length, szValue)
            {
            }

            static char16* SafeCopyAndAdvancePtr(__out_ecount(cchDst) char16* dst, charcount_t& cchDst, __in_ecount(cch) const char16* ptr, charcount_t cch);
        };

    private:
        WritableString* m_string;
#if DBG
        static const char16 k_dbgFill = _u('\xCDCD');
#endif
    };

    // Needed by diagnostics vtable access
    typedef BufferStringBuilder::WritableString BufferStringBuilderWritableString;

} // namespace Js

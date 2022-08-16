//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    char16* BufferStringBuilder::WritableString::SafeCopyAndAdvancePtr(__out_ecount(cchDst) char16* dst, charcount_t& cchDst, __in_ecount(cch) const char16* ptr, charcount_t cch)
    {
        Assert( IsValidCharCount(cch) );
        Assert( dst != nullptr);
        Assert( ptr != nullptr);

        js_wmemcpy_s(dst, cchDst, ptr, cch);

        cchDst -= cch;
        return dst + cch;
    }

    void BufferStringBuilder::WritableString::SetContent(
            const char16* prefix, charcount_t cchPrefix,
            const char16* content, charcount_t cchContent,
            const char16* suffix, charcount_t cchSuffix)
    {
        char16* dst = GetWritableBuffer();

        charcount_t cchRemain = GetLength();
        dst = SafeCopyAndAdvancePtr(dst, cchRemain, prefix, cchPrefix);
        dst = SafeCopyAndAdvancePtr(dst, cchRemain, content, cchContent);
        (void)SafeCopyAndAdvancePtr(dst, cchRemain, suffix, cchSuffix);
    }

    BufferStringBuilder::WritableString* BufferStringBuilder::WritableString::New(charcount_t length, ScriptContext* scriptContext)
    {
        Recycler* recycler = scriptContext->GetRecycler();

        // Allocate recycler memory to store the string plus a terminating NUL
        charcount_t allocSize = JavascriptString::SafeSzSize(length);
        char16* buffer = RecyclerNewArrayLeaf(recycler, char16, allocSize);

#if DBG
        // Fill with a debug pattern (including the space for the NUL terminator)
        wmemset( buffer, k_dbgFill, allocSize );
#endif

        WritableString* newInstance = RecyclerNew(recycler, WritableString, scriptContext->GetLibrary()->GetStringTypeStatic(), length, buffer);
        return newInstance;
    }

    JavascriptString* BufferStringBuilder::ToString()
    {
        DbgAssertNotFrozen();

        char16* buffer = DangerousGetWritableBuffer();
        charcount_t length = this->m_string->GetLength();

#if DBG
        // Can't do this check easily when user use the debug filled character
#if 0

        for( charcount_t i = 0; i != length; ++i )
        {
            // This could be tripped if the buffer actually contains a char
            // with the debug fill value. In that case, this assert may
            // need to be downgraded to a warning.
            AssertMsg( buffer[i] != k_dbgFill, "BufferString was not completely filled before ToString" );
        }
#endif
        AssertMsg( buffer[length] == k_dbgFill, "BufferString sentinel was overwritten." );
#endif

        // NUL terminate
        buffer[length] = _u('\0');

        JavascriptString* result = this->m_string;

#ifdef PROFILE_STRINGS
        StringProfiler::RecordNewString( this->m_string->GetScriptContext(), buffer, length );
#endif
        // Prevent further calls to ToString, SetContent etc.
        this->m_string = nullptr;

        return result;
    }

} // namespace Js

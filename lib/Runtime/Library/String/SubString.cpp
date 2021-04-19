//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    DEFINE_RECYCLER_TRACKER_PERF_COUNTER(SubString);

    inline SubString::SubString(void const * originalFullStringReference, const char16* subString, charcount_t length, ScriptContext *scriptContext) :
        JavascriptString(scriptContext->GetLibrary()->GetStringTypeStatic())
    {
        this->SetBuffer(subString);
        this->originalFullStringReference = originalFullStringReference;
        this->SetLength(length);

#ifdef PROFILE_STRINGS
        StringProfiler::RecordNewString( scriptContext, this->UnsafeGetBuffer(), this->GetLength() );
#endif
    }

    JavascriptString* SubString::New(JavascriptString* string, charcount_t start, charcount_t length)
    {
        AssertMsg( IsValidCharCount(start), "start is out of range" );
        AssertMsg( IsValidCharCount(length), "length is out of range" );

        ScriptContext *scriptContext = string->GetScriptContext();
        if (!length)
        {
            return scriptContext->GetLibrary()->GetEmptyString();
        }

        Recycler* recycler = scriptContext->GetRecycler();

        AssertOrFailFast(string->GetLength() >= start + length);
        const char16 * subString = string->GetString() + start;
        void const * originalFullStringReference = string->GetOriginalStringReference();

#if SYSINFO_IMAGE_BASE_AVAILABLE
        AssertMsg(AutoSystemInfo::IsJscriptModulePointer((void*)originalFullStringReference)
            || recycler->IsValidObject((void*)originalFullStringReference)
            || (VirtualTableInfo<PropertyRecord>::HasVirtualTable((void*)originalFullStringReference) && ((PropertyRecord*)originalFullStringReference)->IsBound())
            || (string->GetLength() == 1 && originalFullStringReference == scriptContext->GetLibrary()->GetCharStringCache().GetStringForChar(string->GetString()[0])->UnsafeGetBuffer()),
            "Owning pointer for SubString must be static or GC pointer, property record bound by thread allocator, or character buffer in global string cache");
#endif

        return RecyclerNew(recycler, SubString, originalFullStringReference, subString, length, scriptContext);
    }

    JavascriptString* SubString::New(const char16* string, charcount_t start, charcount_t length, ScriptContext *scriptContext)
    {
        AssertMsg( IsValidCharCount(start), "start is out of range" );
        AssertMsg( IsValidCharCount(length), "length is out of range" );

        if (!length)
        {
            return scriptContext->GetLibrary()->GetEmptyString();
        }

        Recycler* recycler = scriptContext->GetRecycler();
        return RecyclerNew(recycler, SubString, string, string + start, length, scriptContext);
    }

    const char16* SubString::GetSz()
    {
        if (originalFullStringReference)
        {
            Recycler* recycler = this->GetScriptContext()->GetRecycler();
            char16 * newInstance = AllocateLeafAndCopySz(recycler, UnsafeGetBuffer(), GetLength());
            this->SetBuffer(newInstance);

            // We don't need the string reference anymore, set it to nullptr and use this to know our string is nullptr terminated
            originalFullStringReference = nullptr;
        }

        return UnsafeGetBuffer();
    }

    void const * SubString::GetOriginalStringReference()
    {
        if (originalFullStringReference != nullptr)
        {
            return originalFullStringReference;
        }
        return __super::GetOriginalStringReference();
    }

    size_t SubString::GetAllocatedByteCount() const
    {
        if (originalFullStringReference)
        {
            return 0;
        }
        return __super::GetAllocatedByteCount();
    }

    bool SubString::IsSubstring() const
    {
        if (originalFullStringReference)
        {
            return true;
        }
        return false;
    }

    void SubString::CachePropertyRecord(_In_ PropertyRecord const* propertyRecord)
    {
        // Now that the property record owns a copy of the string data, transform
        // this instance into a more efficient type.
        this->originalFullStringReference = nullptr;
        LiteralStringWithPropertyStringPtr* converted = LiteralStringWithPropertyStringPtr::ConvertString(this);
        converted->CachePropertyRecordImpl(propertyRecord);
    }
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

#include "Library/String/ProfileString.h"
#include "Library/String/SingleCharString.h"

using namespace Js;

    CharStringCache::CharStringCache() : charStringCache(nullptr)
    {
        ClearArray(charStringCacheA);
    }

    JavascriptString* CharStringCache::GetStringForCharA(char c)
    {
        AssertMsg(JavascriptString::IsASCII7BitChar(c), "GetStringForCharA must be called with ASCII 7bit chars only");

        PropertyString * str = charStringCacheA[(int)c];
        if (str == nullptr)
        {
            PropertyRecord const * propertyRecord;
            char16 wc = c;
            JavascriptLibrary * javascriptLibrary = JavascriptLibrary::FromCharStringCache(this);
            javascriptLibrary->GetScriptContext()->GetOrAddPropertyRecord(&wc, 1, &propertyRecord);
            str = javascriptLibrary->CreatePropertyString(propertyRecord);
            charStringCacheA[(int)c] = str;
        }

        return str;
    }


    JavascriptString* CharStringCache::GetStringForChar(char16 c)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(GetStringForChar);
#ifdef PROFILE_STRINGS
        StringProfiler::RecordSingleCharStringRequest(JavascriptLibrary::FromCharStringCache(this)->GetScriptContext());
#endif
        if (JavascriptString::IsASCII7BitChar(c))
        {
            return GetStringForCharA(JavascriptString::ToASCII7BitChar(c));
        }

        return GetStringForCharW(c);
        JIT_HELPER_END(GetStringForChar);
    }
    JIT_HELPER_TEMPLATE(GetStringForChar, GetStringForCharCodePoint)

    JavascriptString* CharStringCache::GetStringForCharW(char16 c)
    {
        Assert(!JavascriptString::IsASCII7BitChar(c));
        JavascriptString* str;
        ScriptContext * scriptContext = JavascriptLibrary::FromCharStringCache(this)->GetScriptContext();
        if (!scriptContext->IsClosed())
        {
            if (charStringCache == nullptr)
            {
                Recycler * recycler = scriptContext->GetRecycler();
                charStringCache = RecyclerNew(recycler, CharStringCacheMap, recycler, 17);
            }
            if (!charStringCache->TryGetValue(c, &str))
            {
                str = SingleCharString::New(c, scriptContext);
                charStringCache->Add(c, str);
            }
        }
        else
        {
            str = SingleCharString::New(c, scriptContext);
        }
        return str;
    }

    JavascriptString* CharStringCache::GetStringForCharSP(codepoint_t c)
    {
        Assert(c >= 0x10000);
        CompileAssert(sizeof(char16) * 2 == sizeof(codepoint_t));

        ScriptContext* scriptContext = JavascriptLibrary::FromCharStringCache(this)->GetScriptContext();

        // #sec - string.fromcodepoint: "If nextCP < 0 or nextCP > 0x10FFFF, throw a RangeError exception"
        if (c > 0x10FFFF)
        {
            JavascriptError::ThrowRangeError(scriptContext, JSERR_InvalidCodePoint, scriptContext->GetIntegerString(c));
        }

        char16 buffer[2];

        Js::NumberUtilities::CodePointAsSurrogatePair(c, buffer, buffer + 1);
        JavascriptString* str = JavascriptString::NewCopyBuffer(buffer, 2, scriptContext);
        // TODO: perhaps do some sort of cache for supplementary characters
        return str;
    }

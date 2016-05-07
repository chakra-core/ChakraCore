//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// ChakraDiag does not link with Runtime.lib and does not include .cpp files, so this file will be included as a header
// For these reasons, we need the functions marked as inline in this file to remain inline
#include "RuntimeLibraryPch.h"
#include "DataStructures/LargeStack.h"

namespace Js
{
    #pragma region StringCopyInfo
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    inline StringCopyInfo::StringCopyInfo()
    {
        // This constructor is just to satisfy LargeStack for now, as it creates an array of these. Ideally, it should only
        // instantiate this class for pushed items using the copy constructor.
    #if DBG
        isInitialized = false;
    #endif
    }

    // In the ChakraDiag case, this file is #included so the method needs to be marked as inline
    // In the ChakraCore case, it's compiled standalone and then linked with, and the StringCopyInfo
    // constructor is referenced by other translation units so it needs to not be inline
    JS_DIAG_INLINE StringCopyInfo::StringCopyInfo(
        JavascriptString *const sourceString,
        _Inout_count_(sourceString->m_charLength) char16 *const destinationBuffer)
        : sourceString(sourceString), destinationBuffer(destinationBuffer)
    {
        Assert(sourceString);
        Assert(destinationBuffer);

    #if DBG
        isInitialized = true;
    #endif
    }

    JS_DIAG_INLINE JavascriptString *StringCopyInfo::SourceString() const
    {
        Assert(isInitialized);

        return sourceString;
    }

    JS_DIAG_INLINE char16 *StringCopyInfo::DestinationBuffer() const
    {
        Assert(isInitialized);
        return destinationBuffer;
    }

    #ifndef IsJsDiag

    void StringCopyInfo::InstantiateForceInlinedMembers()
    {
        // Force-inlined functions defined in a translation unit need a reference from an extern non-force-inlined function in
        // the same translation unit to force an instantiation of the force-inlined function. Otherwise, if the force-inlined
        // function is not referenced in the same translation unit, it will not be generated and the linker is not able to find
        // the definition to inline the function in other translation units.
        AnalysisAssert(false);

        StringCopyInfo copyInfo;
        JavascriptString *const string = nullptr;
        char16 *const buffer = nullptr;

        (StringCopyInfo());
        (StringCopyInfo(string, buffer));
        copyInfo.SourceString();
        copyInfo.DestinationBuffer();
    }

    #endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    #pragma endregion

    #pragma region StringCopyInfoStack
    #ifndef IsJsDiag
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    StringCopyInfoStack::StringCopyInfoStack(ScriptContext *const scriptContext)
        : scriptContext(scriptContext), allocator(nullptr), stack(nullptr)
    {
        Assert(scriptContext);
    }

    StringCopyInfoStack::~StringCopyInfoStack()
    {
        if (allocator)
        {
            scriptContext->ReleaseTemporaryAllocator(allocator);
        }
    }

    bool StringCopyInfoStack::IsEmpty()
    {
        Assert(!allocator == !stack);

        return !stack || !!stack->Empty();
    }

    void StringCopyInfoStack::Push(const StringCopyInfo copyInfo)
    {
        Assert(!allocator == !stack);

        if(!stack)
            CreateStack();
        stack->Push(copyInfo);
    }

    const StringCopyInfo StringCopyInfoStack::Pop()
    {
        Assert(allocator);
        Assert(stack);

        return stack->Pop();
    }

    void StringCopyInfoStack::CreateStack()
    {
        Assert(!allocator);
        Assert(!stack);

        allocator = scriptContext->GetTemporaryAllocator(_u("StringCopyInfoStack"));
        Assert(allocator);
        stack = LargeStack<StringCopyInfo>::New(allocator->GetAllocator());
        Assert(stack);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    #endif
    #pragma endregion
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if !defined(_M_X64)
#error Amd64StackFrame is not supported on this architecture.
#endif

///
/// Amd64StackFrame public API
///
/// Constructor/Destructor
/// GetAddressOfReturnAddress
/// GetInstructionPointer
/// InitializeByReturnAddress
/// Next
/// SkipToFrame
///
/// Amd64ContextsManager
///
namespace Js
{
bool
Amd64StackFrame::InitializeByFrameId(void * frame, ScriptContext* scriptContext)
{
    this->frame = (void **)frame;

    this->stackCheckCodeHeight =
        scriptContext->GetThreadContext()->DoInterruptProbe() ? stackCheckCodeHeightWithInterruptProbe
        : scriptContext->GetThreadContext()->IsThreadBound() ? stackCheckCodeHeightThreadBound
        : stackCheckCodeHeightNotThreadBound;

    return Next();
}

bool
Amd64StackFrame::InitializeByReturnAddress(void * returnAddress, ScriptContext* scriptContext)
{
    void ** framePtr;
    __asm
    {
        mov framePtr, rbp;
    }
    this->frame = framePtr;

    this->stackCheckCodeHeight =
        scriptContext->GetThreadContext()->DoInterruptProbe() ? stackCheckCodeHeightWithInterruptProbe
        : scriptContext->GetThreadContext()->IsThreadBound() ? stackCheckCodeHeightThreadBound
        : stackCheckCodeHeightNotThreadBound;

    while (Next())
    {
        if (this->codeAddr == returnAddress)
        {
            return true;
        }
    }
    return false;
}

bool
Amd64StackFrame::Next()
{
    this->addressOfCodeAddr = this->GetAddressOfReturnAddress();
    this->codeAddr = this->GetReturnAddress();
    this->frame = (void **)this->frame[0];
    return frame != nullptr;
}

bool
Amd64StackFrame::SkipToFrame(void * frameAddress)
{
    this->frame = (void **)frameAddress;
    return Next();
}

bool
Amd64StackFrame::IsInStackCheckCode(void *entry, void *codeAddr, size_t stackCheckCodeHeight)
{
    return ((size_t(codeAddr) - size_t(entry)) <= stackCheckCodeHeight);
}

// Dummy constructor since we don't need to manager the contexts here
Amd64ContextsManager::Amd64ContextsManager() {}
};


//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCorePch.h"

#ifdef STACK_BACK_TRACE
#include "Core/StackBackTrace.h"
#include "Core/DbgHelpSymbolManager.h"

StackBackTrace::StackBackTrace(ULONG framesToSkip, ULONG framesToCapture) : requestedFramesToCapture(framesToCapture)
{
    this->Capture(framesToSkip);
}

// Don't capture, just remember requestedFramesToCapture/allocate buffer for them.
StackBackTrace::StackBackTrace(ULONG framesToCaptureLater) : requestedFramesToCapture(framesToCaptureLater), framesCount(0)
{
}

// This can be called multiple times, together with Create, in which case we will use (overwrite) same buffer.
ULONG StackBackTrace::Capture(ULONG framesToSkip)
{
    this->framesCount = CaptureStackBackTrace(framesToSkip + BaseFramesToSkip, this->requestedFramesToCapture, this->stackBackTrace, NULL);
    return this->framesCount;
}

size_t
StackBackTrace::Print()
{
    DbgHelpSymbolManager::EnsureInitialized();

    size_t retValue = 0;

    for(ULONG i = 0; i < this->framesCount; i++)
    {
        PVOID address = this->stackBackTrace[i];
        retValue += Output::Print(_u(" "));
        retValue += DbgHelpSymbolManager::PrintSymbol(address);
        retValue += Output::Print(_u("\n"));
    }

    retValue += Output::Print(_u("\n"));
    return retValue;
}
#endif

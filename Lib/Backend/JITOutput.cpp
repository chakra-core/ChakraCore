//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITOutput::JITOutput()
{
    m_outputData.writeableEPData.hasJittedStackClosure = false;
    // TODO: (michhol) validate initial values
    m_outputData.writeableEPData.localVarSlotsOffset = 0;
    m_outputData.writeableEPData.localVarChangedOffset = 0;
}

void
JITOutput::SetHasJITStackClosure()
{
    m_outputData.writeableEPData.hasJittedStackClosure = true;
}

void
JITOutput::SetVarSlotsOffset(int32 offset)
{
    m_outputData.writeableEPData.localVarSlotsOffset = offset;
}

void
JITOutput::SetVarChangedOffset(int32 offset)
{
    m_outputData.writeableEPData.localVarChangedOffset = offset;
}

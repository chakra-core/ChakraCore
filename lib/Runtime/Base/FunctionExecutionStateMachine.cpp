//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"
#include "FunctionExecutionStateMachine.h"

namespace Js
{
    FunctionExecutionStateMachine::FunctionExecutionStateMachine() :
        executionMode(ExecutionMode::Interpreter)
    {
    }

    ExecutionMode FunctionExecutionStateMachine::GetExecutionMode() const
    {
        return executionMode;
    }

    void FunctionExecutionStateMachine::SetExecutionMode(ExecutionMode mode)
    {
        executionMode = mode;
    }
}
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class FunctionExecutionStateMachine
    {
    public:
        FunctionExecutionStateMachine();

        ExecutionMode GetExecutionMode() const;
        void SetExecutionMode(ExecutionMode mode);

    private:
        // Tracks the current execution mode. See ExecutionModes.h for more info.
        FieldWithBarrier(ExecutionMode) executionMode;
    };
};

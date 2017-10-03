//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "catch.hpp"
#include "FunctionExecutionTest.h"

// Definition of tests that exercise FunctionExecutionStateMachine
namespace FunctionExecutionTest
{
    // Simple test case to validate that the state machine was created.
    TEST_CASE("FuncExe_BasicTest")
    {
        Js::FunctionExecutionStateMachine f;
        Js::FunctionBody body;
        f.InitializeExecutionModeAndLimits(&body);
        CHECK(f.GetExecutionMode() == ExecutionMode::Interpreter);
    }
}
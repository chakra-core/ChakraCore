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

    ExecutionMode FunctionExecutionStateMachine::GetDefaultInterpreterExecutionMode(FunctionBody* functionBody) const
    {
        if (!functionBody->DoInterpreterProfile())
        {
            VerifyExecutionMode(ExecutionMode::Interpreter, functionBody);
            return ExecutionMode::Interpreter;
        }
        if (functionBody->DoInterpreterAutoProfile())
        {
            VerifyExecutionMode(ExecutionMode::AutoProfilingInterpreter, functionBody);
            return ExecutionMode::AutoProfilingInterpreter;
        }
        VerifyExecutionMode(ExecutionMode::ProfilingInterpreter, functionBody);
        return ExecutionMode::ProfilingInterpreter;
    }

    ExecutionMode FunctionExecutionStateMachine::GetInterpreterExecutionMode(const bool isPostBailout, FunctionBody* functionBody)
    {
        Assert(initializedExecutionModeAndLimits);

        if (isPostBailout && functionBody->DoInterpreterProfile())
        {
            return ExecutionMode::ProfilingInterpreter;
        }

        switch (GetExecutionMode())
        {
        case ExecutionMode::Interpreter:
        case ExecutionMode::AutoProfilingInterpreter:
        case ExecutionMode::ProfilingInterpreter:
            return GetExecutionMode();

        case ExecutionMode::SimpleJit:
            if (CONFIG_FLAG(NewSimpleJit))
            {
                return GetDefaultInterpreterExecutionMode(functionBody);
            }
            // fall through

        case ExecutionMode::FullJit:
        {
            const ExecutionMode executionMode =
                functionBody->DoInterpreterProfile() ? ExecutionMode::ProfilingInterpreter : ExecutionMode::Interpreter;
            VerifyExecutionMode(executionMode, functionBody);
            return executionMode;
        }

        default:
            Assert(false);
            __assume(false);
        }
    }

    bool FunctionExecutionStateMachine::IsInterpreterExecutionMode() const
    {
        return GetExecutionMode() <= ExecutionMode::ProfilingInterpreter;
    }

    // Safely moves from one execution mode to another and updates appropriate execution state for the next
    // mode. Note that there are other functions that modify executionMode that do not involve this function.
    // This function transitions ExecutionMode in the following order:
    //
    //       +-- Interpreter
    //       |
    //       |  AutoProfilingInterpreter --+
    //       |      |   ^                  |
    //       |      |   |                  v
    //       |      |   |               SimpleJit
    //       |      v   |                  |
    //       |  ProfilingInterpreter  <----+
    //       |      |
    //       |      |
    //       |      v
    //       +-> FullJit
    //
    // Transition to the next mode occurs when the limit for the current execution mode reaches 0.
    // Returns true when a transition occurs (i.e., the execution mode was updated since the beginning of
    // this function call). Otherwise, returns false to indicate no change in state.
    // See more details of each mode in ExecutionModes.h
    bool FunctionExecutionStateMachine::TryTransitionToNextExecutionMode(FunctionBody* functionBody)
    {
        Assert(initializedExecutionModeAndLimits);

        switch (GetExecutionMode())
        {
            // Managing transition from Interpreter
        case ExecutionMode::Interpreter:
            if (GetInterpretedCount() < interpreterLimit)
            {
                VerifyExecutionMode(GetExecutionMode(), functionBody);
                return false;
            }
            CommitExecutedIterations(interpreterLimit, interpreterLimit);
            goto TransitionToFullJit;

            // Managing transition to and from AutoProfilingInterpreter
        TransitionToAutoProfilingInterpreter:
            if (autoProfilingInterpreter0Limit != 0 || autoProfilingInterpreter1Limit != 0)
            {
                SetExecutionMode(ExecutionMode::AutoProfilingInterpreter);
                SetInterpretedCount(0);
                return true;
            }
            goto TransitionFromAutoProfilingInterpreter;

        case ExecutionMode::AutoProfilingInterpreter:
        {
            // autoProfilingInterpreter0Limit is the default limit for this mode
            // autoProfilingInterpreter1Limit becomes the limit for this mode when this FunctionBody has
            // already previously run in ProfilingInterpreter and AutoProfilingInterpreter
            uint16 &autoProfilingInterpreterLimit =
                autoProfilingInterpreter0Limit == 0 && profilingInterpreter0Limit == 0
                ? autoProfilingInterpreter1Limit
                : autoProfilingInterpreter0Limit;
            if (GetInterpretedCount() < autoProfilingInterpreterLimit)
            {
                VerifyExecutionMode(GetExecutionMode(), functionBody);
                return false;
            }
            CommitExecutedIterations(autoProfilingInterpreterLimit, autoProfilingInterpreterLimit);
            // fall through
        }

    TransitionFromAutoProfilingInterpreter:
        Assert(autoProfilingInterpreter0Limit == 0 || autoProfilingInterpreter1Limit == 0);
        if (profilingInterpreter0Limit == 0 && autoProfilingInterpreter1Limit == 0)
        {
            goto TransitionToSimpleJit;
        }
        // fall through

        // Managing transition to and from ProfilingInterpreter
    TransitionToProfilingInterpreter:
        if (profilingInterpreter0Limit != 0 || profilingInterpreter1Limit != 0)
        {
            SetExecutionMode(ExecutionMode::ProfilingInterpreter);
            SetInterpretedCount(0);
            return true;
        }
        goto TransitionFromProfilingInterpreter;

        case ExecutionMode::ProfilingInterpreter:
        {
            // profilingInterpreter0Limit is the default limit for this mode
            // profilingInterpreter1Limit becomes the limit for this mode when this FunctionBody has already
            // previously run in ProfilingInterpreter, AutoProfilingInterpreter, and SimpleJIT
            uint16 &profilingInterpreterLimit =
                profilingInterpreter0Limit == 0 && autoProfilingInterpreter1Limit == 0 && simpleJitLimit == 0
                ? profilingInterpreter1Limit
                : profilingInterpreter0Limit;
            if (GetInterpretedCount() < profilingInterpreterLimit)
            {
                VerifyExecutionMode(GetExecutionMode(), functionBody);
                return false;
            }
            CommitExecutedIterations(profilingInterpreterLimit, profilingInterpreterLimit);
            // fall through
        }

    TransitionFromProfilingInterpreter:
        Assert(profilingInterpreter0Limit == 0 || profilingInterpreter1Limit == 0);
        if (autoProfilingInterpreter1Limit == 0 && simpleJitLimit == 0 && profilingInterpreter1Limit == 0)
        {
            goto TransitionToFullJit;
        }
        goto TransitionToAutoProfilingInterpreter;

        // Managing transition to and from SimpleJit
    TransitionToSimpleJit:
        if (simpleJitLimit != 0)
        {
            SetExecutionMode(ExecutionMode::SimpleJit);

            // Zero the interpreted count here too, so that we can determine how many interpreter iterations ran
            // while waiting for simple JIT
            SetInterpretedCount(0);
            return true;
        }
        goto TransitionToProfilingInterpreter;

        case ExecutionMode::SimpleJit:
        {
            FunctionEntryPointInfo *const simpleJitEntryPointInfo = functionBody->GetSimpleJitEntryPointInfo();
            if (!simpleJitEntryPointInfo || simpleJitEntryPointInfo->callsCount != 0)
            {
                VerifyExecutionMode(GetExecutionMode(), functionBody);
                return false;
            }
            CommitExecutedIterations(simpleJitLimit, simpleJitLimit);
            goto TransitionToProfilingInterpreter;
        }

    TransitionToFullJit:
        if (!PHASE_OFF(FullJitPhase, functionBody))
        {
            SetExecutionMode(ExecutionMode::FullJit);
            return true;
        }
        // fall through

        // Managing transtion to FullJit
        case ExecutionMode::FullJit:
            VerifyExecutionMode(GetExecutionMode(), functionBody);
            return false;

        default:
            Assert(false);
            __assume(false);
        }
    }

    void FunctionExecutionStateMachine::TryTransitionToNextInterpreterExecutionMode(FunctionBody* functionBody)
    {
        Assert(IsInterpreterExecutionMode());

        TryTransitionToNextExecutionMode(functionBody);
        SetExecutionMode(GetInterpreterExecutionMode(false, functionBody));
    }

    void FunctionExecutionStateMachine::CommitExecutedIterations(uint16 &limit, const uint executedIterations)
    {
        Assert(initializedExecutionModeAndLimits);
        Assert(
            &limit == &interpreterLimit ||
            &limit == &autoProfilingInterpreter0Limit ||
            &limit == &profilingInterpreter0Limit ||
            &limit == &autoProfilingInterpreter1Limit ||
            &limit == &simpleJitLimit ||
            &limit == &profilingInterpreter1Limit);

        const uint16 clampedExecutedIterations = executedIterations >= limit ? limit : static_cast<uint16>(executedIterations);
        Assert(fullJitThreshold >= clampedExecutedIterations);
        fullJitThreshold -= clampedExecutedIterations;
        limit -= clampedExecutedIterations;
        VerifyExecutionModeLimits();

        if (&limit == &profilingInterpreter0Limit ||
            (!CONFIG_FLAG(NewSimpleJit) && &limit == &simpleJitLimit) ||
            &limit == &profilingInterpreter1Limit)
        {
            const uint16 newCommittedProfiledIterations = committedProfiledIterations + clampedExecutedIterations;
            committedProfiledIterations =
                newCommittedProfiledIterations >= committedProfiledIterations ? newCommittedProfiledIterations : UINT16_MAX;
        }
    }

    void FunctionExecutionStateMachine::VerifyExecutionModeLimits() const
    {
        Assert(initializedExecutionModeAndLimits);
        Assert(
            (
                interpreterLimit +
                autoProfilingInterpreter0Limit +
                profilingInterpreter0Limit +
                autoProfilingInterpreter1Limit +
                simpleJitLimit +
                profilingInterpreter1Limit
                ) == fullJitThreshold);
    }

    void FunctionExecutionStateMachine::VerifyExecutionMode(const ExecutionMode executionMode, FunctionBody* functionBody) const
    {
#if DBG
        Assert(initializedExecutionModeAndLimits);
        Assert(executionMode < ExecutionMode::Count);

        switch (executionMode)
        {
        case ExecutionMode::Interpreter:
            Assert(!functionBody->DoInterpreterProfile());
            break;

        case ExecutionMode::AutoProfilingInterpreter:
            Assert(functionBody->DoInterpreterProfile());
            Assert(functionBody->DoInterpreterAutoProfile());
            break;

        case ExecutionMode::ProfilingInterpreter:
            Assert(functionBody->DoInterpreterProfile());
            break;

        case ExecutionMode::SimpleJit:
            Assert(functionBody->DoSimpleJit());
            break;

        case ExecutionMode::FullJit:
            Assert(!PHASE_OFF(FullJitPhase, functionBody));
            break;

        default:
            Assert(false);
            __assume(false);
        }
#endif
    }
}
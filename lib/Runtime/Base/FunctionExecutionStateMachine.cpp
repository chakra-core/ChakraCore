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

    void FunctionExecutionStateMachine::InitializeExecutionModeAndLimits(FunctionBody* functionBody)
    {
        DebugOnly(initializedExecutionModeAndLimits = true);

        const ConfigFlagsTable &configFlags = Configuration::Global.flags;

        interpreterLimit = 0;
        autoProfilingInterpreter0Limit = static_cast<uint16>(configFlags.AutoProfilingInterpreter0Limit);
        profilingInterpreter0Limit = static_cast<uint16>(configFlags.ProfilingInterpreter0Limit);
        autoProfilingInterpreter1Limit = static_cast<uint16>(configFlags.AutoProfilingInterpreter1Limit);
        simpleJitLimit = static_cast<uint16>(configFlags.SimpleJitLimit);
        profilingInterpreter1Limit = static_cast<uint16>(configFlags.ProfilingInterpreter1Limit);

        // Based on which execution modes are disabled, calculate the number of additional iterations that need to be covered by
        // the execution mode that will scale with the full JIT threshold
        uint16 scale = 0;
        const bool doInterpreterProfile = functionBody->DoInterpreterProfile();
        if (!doInterpreterProfile)
        {
            scale +=
                autoProfilingInterpreter0Limit +
                profilingInterpreter0Limit +
                autoProfilingInterpreter1Limit +
                profilingInterpreter1Limit;
            autoProfilingInterpreter0Limit = 0;
            profilingInterpreter0Limit = 0;
            autoProfilingInterpreter1Limit = 0;
            profilingInterpreter1Limit = 0;
        }
        else if (!functionBody->DoInterpreterAutoProfile())
        {
            scale += autoProfilingInterpreter0Limit + autoProfilingInterpreter1Limit;
            autoProfilingInterpreter0Limit = 0;
            autoProfilingInterpreter1Limit = 0;
            if (!CONFIG_FLAG(NewSimpleJit))
            {
                simpleJitLimit += profilingInterpreter0Limit;
                profilingInterpreter0Limit = 0;
            }
        }
        if (!functionBody->DoSimpleJit())
        {
            if (!CONFIG_FLAG(NewSimpleJit) && doInterpreterProfile)
            {
                // The old simple JIT is off, but since it does profiling, it will be replaced with the profiling interpreter
                profilingInterpreter1Limit += simpleJitLimit;
            }
            else
            {
                scale += simpleJitLimit;
            }
            simpleJitLimit = 0;
        }
        if (PHASE_OFF(FullJitPhase, functionBody))
        {
            scale += profilingInterpreter1Limit;
            profilingInterpreter1Limit = 0;
        }

        uint16 fullJitThreshold =
            static_cast<uint16>(
                configFlags.AutoProfilingInterpreter0Limit +
                configFlags.ProfilingInterpreter0Limit +
                configFlags.AutoProfilingInterpreter1Limit +
                configFlags.SimpleJitLimit +
                configFlags.ProfilingInterpreter1Limit);
        if (!configFlags.EnforceExecutionModeLimits)
        {
            /*
            Scale the full JIT threshold based on some heuristics:
            - If the % of code in loops is > 50, scale by 1
            - Byte-code size of code outside loops
            - If the size is < 50, scale by 1.2
            - If the size is < 100, scale by 1.4
            - If the size is >= 100, scale by 1.6
            */
            const uint loopPercentage = functionBody->GetByteCodeInLoopCount() * 100 / max(1u, functionBody->GetByteCodeCount());
            const int byteCodeSizeThresholdForInlineCandidate = CONFIG_FLAG(LoopInlineThreshold);
            bool delayFullJITThisFunc =
                (CONFIG_FLAG(DelayFullJITSmallFunc) > 0) && (functionBody->GetByteCodeWithoutLDACount() <= (uint)byteCodeSizeThresholdForInlineCandidate);

            if (loopPercentage <= 50 || delayFullJITThisFunc)
            {
                const uint straightLineSize = functionBody->GetByteCodeCount() - functionBody->GetByteCodeInLoopCount();
                double fullJitDelayMultiplier;
                if (delayFullJITThisFunc)
                {
                    fullJitDelayMultiplier = CONFIG_FLAG(DelayFullJITSmallFunc) / 10.0;
                }
                else if (straightLineSize < 50)
                {
                    fullJitDelayMultiplier = 1.2;
                }
                else if (straightLineSize < 100)
                {
                    fullJitDelayMultiplier = 1.4;
                }
                else
                {
                    fullJitDelayMultiplier = 1.6;
                }

                const uint16 newFullJitThreshold = static_cast<uint16>(fullJitThreshold * fullJitDelayMultiplier);
                scale += newFullJitThreshold - fullJitThreshold;
                fullJitThreshold = newFullJitThreshold;
            }
        }

        Assert(fullJitThreshold >= scale);
        this->fullJitThreshold = fullJitThreshold - scale;
        SetInterpretedCount(0);
        SetExecutionMode(GetDefaultInterpreterExecutionMode(functionBody));
        SetFullJitThreshold(fullJitThreshold, functionBody);
        TryTransitionToNextInterpreterExecutionMode(functionBody);
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

    uint16 FunctionExecutionStateMachine::GetSimpleJitExecutedIterations(FunctionBody* functionBody) const
    {
        Assert(initializedExecutionModeAndLimits);
        Assert(GetExecutionMode() == ExecutionMode::SimpleJit);

        FunctionEntryPointInfo *const simpleJitEntryPointInfo = functionBody->GetSimpleJitEntryPointInfo();
        if (!simpleJitEntryPointInfo)
        {
            return 0;
        }

        // Simple JIT counts down and transitions on overflow
        const uint32 callCount = simpleJitEntryPointInfo->callsCount;
        Assert(simpleJitLimit == 0 ? callCount == 0 : simpleJitLimit > callCount);
        return callCount == 0 ?
            static_cast<uint16>(simpleJitLimit) :
            static_cast<uint16>(simpleJitLimit) - static_cast<uint16>(callCount) - 1;
    }

    void FunctionExecutionStateMachine::SetSimpleJitCallCount(const uint16 simpleJitLimit, FunctionBody* functionBody) const
    {
        Assert(GetExecutionMode() == ExecutionMode::SimpleJit);
        Assert(functionBody->GetDefaultFunctionEntryPointInfo() == functionBody->GetSimpleJitEntryPointInfo());

        // Simple JIT counts down and transitions on overflow
        const uint8 limit = static_cast<uint8>(min(0xffui16, simpleJitLimit));
        functionBody->GetSimpleJitEntryPointInfo()->callsCount = limit == 0 ? 0 : limit - 1;
    }

    void FunctionExecutionStateMachine::SetFullJitThreshold(const uint16 newFullJitThreshold, FunctionBody* functionBody, const bool skipSimpleJit)
    {
        Assert(initializedExecutionModeAndLimits);
        Assert(GetExecutionMode() != ExecutionMode::FullJit);

        int scale = newFullJitThreshold - fullJitThreshold;
        if (scale == 0)
        {
            VerifyExecutionModeLimits();
            return;
        }
        fullJitThreshold = newFullJitThreshold;

        const auto ScaleLimit = [&](uint16 &limit) -> bool
        {
            Assert(scale != 0);
            const int limitScale = max(-static_cast<int>(limit), scale);
            const int newLimit = limit + limitScale;
            Assert(static_cast<int>(static_cast<uint16>(newLimit)) == newLimit);
            limit = static_cast<uint16>(newLimit);
            scale -= limitScale;
            Assert(limit == 0 || scale == 0);

            if (&limit == &simpleJitLimit)
            {
                FunctionEntryPointInfo *const simpleJitEntryPointInfo = functionBody->GetSimpleJitEntryPointInfo();
                if (functionBody->GetDefaultFunctionEntryPointInfo() == simpleJitEntryPointInfo)
                {
                    Assert(GetExecutionMode() == ExecutionMode::SimpleJit);
                    const int newSimpleJitCallCount = max(0, (int)simpleJitEntryPointInfo->callsCount + limitScale);
                    Assert(static_cast<int>(static_cast<uint16>(newSimpleJitCallCount)) == newSimpleJitCallCount);
                    SetSimpleJitCallCount(static_cast<uint16>(newSimpleJitCallCount), functionBody);
                }
            }

            return scale == 0;
        };

        /*
        Determine which execution mode's limit scales with the full JIT threshold, in order of preference:
        - New simple JIT
        - Auto-profiling interpreter 1
        - Auto-profiling interpreter 0
        - Interpreter
        - Profiling interpreter 0 (when using old simple JIT)
        - Old simple JIT
        - Profiling interpreter 1
        - Profiling interpreter 0 (when using new simple JIT)
        */
        const bool doSimpleJit = functionBody->DoSimpleJit();
        const bool doInterpreterProfile = functionBody->DoInterpreterProfile();
        const bool fullyScaled =
            (CONFIG_FLAG(NewSimpleJit) && doSimpleJit && ScaleLimit(simpleJitLimit)) ||
            (
                doInterpreterProfile
                ? functionBody->DoInterpreterAutoProfile() &&
                (ScaleLimit(autoProfilingInterpreter1Limit) || ScaleLimit(autoProfilingInterpreter0Limit))
                : ScaleLimit(interpreterLimit)
                ) ||
                (
                    CONFIG_FLAG(NewSimpleJit)
                    ? doInterpreterProfile &&
                    (ScaleLimit(profilingInterpreter1Limit) || ScaleLimit(profilingInterpreter0Limit))
                    : (doInterpreterProfile && ScaleLimit(profilingInterpreter0Limit)) ||
                    (doSimpleJit && ScaleLimit(simpleJitLimit)) ||
                    (doInterpreterProfile && ScaleLimit(profilingInterpreter1Limit))
                    );
        Assert(fullyScaled);
        Assert(scale == 0);

        if (GetExecutionMode() != ExecutionMode::SimpleJit)
        {
            Assert(IsInterpreterExecutionMode());
            if (simpleJitLimit != 0 &&
                (skipSimpleJit || simpleJitLimit < DEFAULT_CONFIG_MinSimpleJitIterations) &&
                !PHASE_FORCE(Phase::SimpleJitPhase, functionBody))
            {
                // Simple JIT code has not yet been generated, and was either requested to be skipped, or the limit was scaled
                // down too much. Skip simple JIT by moving any remaining iterations to an equivalent interpreter execution
                // mode.
                (CONFIG_FLAG(NewSimpleJit) ? autoProfilingInterpreter1Limit : profilingInterpreter1Limit) += simpleJitLimit;
                simpleJitLimit = 0;
                TryTransitionToNextInterpreterExecutionMode(functionBody);
            }
        }

        VerifyExecutionModeLimits();
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

    bool FunctionExecutionStateMachine::TryTransitionToJitExecutionMode(FunctionBody* functionBody)
    {
        const ExecutionMode previousExecutionMode = GetExecutionMode();

        TryTransitionToNextExecutionMode(functionBody);
        switch (GetExecutionMode())
        {
        case ExecutionMode::SimpleJit:
            break;

        case ExecutionMode::FullJit:
            if (fullJitRequeueThreshold == 0)
            {
                break;
            }
            --fullJitRequeueThreshold;
            return false;

        default:
            return false;
        }

        if (GetExecutionMode() != previousExecutionMode)
        {
            functionBody->TraceExecutionMode();
        }
        return true;
    }

    void FunctionExecutionStateMachine::TransitionToSimpleJitExecutionMode(FunctionBody* functionBody)
    {
        CommitExecutedIterations(functionBody);

        interpreterLimit = 0;
        autoProfilingInterpreter0Limit = 0;
        profilingInterpreter0Limit = 0;
        autoProfilingInterpreter1Limit = 0;
        fullJitThreshold = simpleJitLimit + profilingInterpreter1Limit;

        VerifyExecutionModeLimits();
        SetExecutionMode(ExecutionMode::SimpleJit);
    }

    void FunctionExecutionStateMachine::TransitionToFullJitExecutionMode(FunctionBody* functionBody)
    {
        CommitExecutedIterations(functionBody);

        interpreterLimit = 0;
        autoProfilingInterpreter0Limit = 0;
        profilingInterpreter0Limit = 0;
        autoProfilingInterpreter1Limit = 0;
        simpleJitLimit = 0;
        profilingInterpreter1Limit = 0;
        fullJitThreshold = 0;

        VerifyExecutionModeLimits();
        SetExecutionMode(ExecutionMode::FullJit);
    }

    void FunctionExecutionStateMachine::CommitExecutedIterations(FunctionBody* functionBody)
    {
        Assert(initializedExecutionModeAndLimits);

        switch (GetExecutionMode())
        {
        case ExecutionMode::Interpreter:
            CommitExecutedIterations(interpreterLimit, GetInterpretedCount());
            break;

        case ExecutionMode::AutoProfilingInterpreter:
            CommitExecutedIterations(
                autoProfilingInterpreter0Limit == 0 && profilingInterpreter0Limit == 0
                ? autoProfilingInterpreter1Limit
                : autoProfilingInterpreter0Limit,
                GetInterpretedCount());
            break;

        case ExecutionMode::ProfilingInterpreter:
            CommitExecutedIterations(
                functionBody->GetSimpleJitEntryPointInfo()
                ? profilingInterpreter1Limit
                : profilingInterpreter0Limit,
                GetInterpretedCount());
            break;

        case ExecutionMode::SimpleJit:
            CommitExecutedIterations(simpleJitLimit, GetSimpleJitExecutedIterations(functionBody));
            break;

        case ExecutionMode::FullJit:
            break;

        default:
            Assert(false);
            __assume(false);
        }
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
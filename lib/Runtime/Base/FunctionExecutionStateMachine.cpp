//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"
#include "FunctionExecutionStateMachine.h"
#include "Warnings.h"

namespace Js
{
    FunctionExecutionStateMachine::FunctionExecutionStateMachine() :
        owner(nullptr),
        executionState(ExecutionState::Interpreter),
        interpreterLimit(0),
        autoProfilingInterpreter0Limit(0),
        profilingInterpreter0Limit(0),
        autoProfilingInterpreter1Limit(0),
        simpleJitLimit(0),
        profilingInterpreter1Limit(0),
        interpretedCount(0),
        fullJitThreshold(0),
        fullJitRequeueThreshold(0),
        committedProfiledIterations(0),
        lastInterpretedCount(0)
#if DBG
        ,initializedExecutionModeAndLimits(false)
#endif
    {
    }

    void FunctionExecutionStateMachine::InitializeExecutionModeAndLimits(FunctionBody* functionBody)
    {
#if DBG
        initializedExecutionModeAndLimits = true;
#endif
        // Assert we're either uninitialized, or being reinitialized on the same FunctionBody
        Assert(owner == nullptr || owner == functionBody);
        owner = functionBody;

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
        const bool doInterpreterProfile = owner->DoInterpreterProfile();
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
        else if (!owner->DoInterpreterAutoProfile())
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
        if (!owner->DoSimpleJit())
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
        if (PHASE_OFF(FullJitPhase, owner))
        {
            scale += profilingInterpreter1Limit;
            profilingInterpreter1Limit = 0;
        }

        uint16 fullJitThresholdConfig =
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
            const uint loopPercentage = owner->GetByteCodeInLoopCount() * 100 / max(1u, owner->GetByteCodeCount());
            const int byteCodeSizeThresholdForInlineCandidate = CONFIG_FLAG(LoopInlineThreshold);
            bool delayFullJITThisFunc =
                (CONFIG_FLAG(DelayFullJITSmallFunc) > 0) && (owner->GetByteCodeWithoutLDACount() <= (uint)byteCodeSizeThresholdForInlineCandidate);

            if (loopPercentage <= 50 || delayFullJITThisFunc)
            {
                const uint straightLineSize = owner->GetByteCodeCount() - owner->GetByteCodeInLoopCount();
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

                const uint16 newFullJitThreshold = static_cast<uint16>(fullJitThresholdConfig * fullJitDelayMultiplier);
                scale += newFullJitThreshold - fullJitThresholdConfig;
                fullJitThresholdConfig = newFullJitThreshold;
            }
        }

        Assert(fullJitThresholdConfig >= scale);
        fullJitThreshold = fullJitThresholdConfig - scale;
        SetInterpretedCount(0);
        SetDefaultInterpreterExecutionMode();
        SetFullJitThreshold(fullJitThresholdConfig);
        TryTransitionToNextInterpreterExecutionMode();
    }

    void FunctionExecutionStateMachine::ReinitializeExecutionModeAndLimits(FunctionBody* functionBody)
    {
        // TODO: Investigate what it would take to make this invariant hold. Currently fails in AsmJS tests
        // Assert(initializedExecutionModeAndLimits);

        fullJitRequeueThreshold = 0;
        committedProfiledIterations = 0;
        InitializeExecutionModeAndLimits(functionBody);
    }

    bool FunctionExecutionStateMachine::InterpretedSinceCallCountCollection() const
    {
        return interpretedCount != lastInterpretedCount;
    }

    void FunctionExecutionStateMachine::CollectInterpretedCounts()
    {
        lastInterpretedCount = interpretedCount;
    }

    ExecutionMode FunctionExecutionStateMachine::GetExecutionMode() const
    {
        ExecutionMode executionMode = StateToMode(executionState);

        VerifyExecutionMode(executionMode);
        return executionMode;
    }

    void FunctionExecutionStateMachine::SetExecutionState(ExecutionState state)
    {
        // TODO: Investigate what it would take to make this invariant hold
        // Assert(state == GetDefaultInterpreterExecutionState() || IsTerminalState(state));
        VerifyExecutionMode(StateToMode(state));
        executionState = state;
    }

    void FunctionExecutionStateMachine::SetAsmJsExecutionMode()
    {
        SetExecutionState(ExecutionState::FullJit);
    }

    void FunctionExecutionStateMachine::SetDefaultInterpreterExecutionMode()
    {
        SetExecutionState(GetDefaultInterpreterExecutionState());
    }

    FunctionExecutionStateMachine::ExecutionState FunctionExecutionStateMachine::GetDefaultInterpreterExecutionState() const
    {
        if (!owner->DoInterpreterProfile())
        {
            VerifyExecutionMode(ExecutionMode::Interpreter);
            return ExecutionState::Interpreter;
        }
        else if (owner->DoInterpreterAutoProfile())
        {
            VerifyExecutionMode(ExecutionMode::AutoProfilingInterpreter);
            return ExecutionState::AutoProfilingInterpreter0;
        }
        else
        {
            VerifyExecutionMode(ExecutionMode::ProfilingInterpreter);
            return ExecutionState::ProfilingInterpreter0;
        }
    }

    ExecutionMode FunctionExecutionStateMachine::GetInterpreterExecutionMode(const bool isPostBailout)
    {
        Assert(initializedExecutionModeAndLimits);

        if (isPostBailout && owner->DoInterpreterProfile())
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
                return StateToMode(GetDefaultInterpreterExecutionState());
            }
            // fall through

        case ExecutionMode::FullJit:
        {
            const ExecutionMode executionMode =
                owner->DoInterpreterProfile() ? ExecutionMode::ProfilingInterpreter : ExecutionMode::Interpreter;
            VerifyExecutionMode(executionMode);
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

    uint16 FunctionExecutionStateMachine::GetSimpleJitExecutedIterations() const
    {
        Assert(initializedExecutionModeAndLimits);
        Assert(GetExecutionMode() == ExecutionMode::SimpleJit);

        FunctionEntryPointInfo *const simpleJitEntryPointInfo = owner->GetSimpleJitEntryPointInfo();
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

    void FunctionExecutionStateMachine::SetSimpleJitCallCount(const uint16 simpleJitLimit) const
    {
        Assert(GetExecutionMode() == ExecutionMode::SimpleJit);
        Assert(owner->GetDefaultFunctionEntryPointInfo() == owner->GetSimpleJitEntryPointInfo());

        // Simple JIT counts down and transitions on overflow
        const uint8 limit = static_cast<uint8>(min(0xffui16, simpleJitLimit));
        owner->GetSimpleJitEntryPointInfo()->callsCount = limit == 0 ? 0 : limit - 1;
    }

    void FunctionExecutionStateMachine::SetFullJitRequeueThreshold(const uint16 newFullJitRequeueThreshold)
    {
        fullJitRequeueThreshold = newFullJitRequeueThreshold;
    }

    void FunctionExecutionStateMachine::SetFullJitThreshold(const uint16 newFullJitThreshold, const bool skipSimpleJit)
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
                FunctionEntryPointInfo *const simpleJitEntryPointInfo = owner->GetSimpleJitEntryPointInfo();
                if (owner->GetDefaultFunctionEntryPointInfo() == simpleJitEntryPointInfo)
                {
                    Assert(GetExecutionMode() == ExecutionMode::SimpleJit);
                    const int newSimpleJitCallCount = max(0, (int)simpleJitEntryPointInfo->callsCount + limitScale);
                    Assert(static_cast<int>(static_cast<uint16>(newSimpleJitCallCount)) == newSimpleJitCallCount);
                    SetSimpleJitCallCount(static_cast<uint16>(newSimpleJitCallCount));
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
        const bool doSimpleJit = owner->DoSimpleJit();
        const bool doInterpreterProfile = owner->DoInterpreterProfile();
        const bool fullyScaled =
            (CONFIG_FLAG(NewSimpleJit) && doSimpleJit && ScaleLimit(simpleJitLimit)) ||
            (
                doInterpreterProfile
                ? owner->DoInterpreterAutoProfile() &&
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
                !PHASE_FORCE(Phase::SimpleJitPhase, owner))
            {
                // Simple JIT code has not yet been generated, and was either requested to be skipped, or the limit was scaled
                // down too much. Skip simple JIT by moving any remaining iterations to an equivalent interpreter execution
                // mode.
                (CONFIG_FLAG(NewSimpleJit) ? autoProfilingInterpreter1Limit : profilingInterpreter1Limit) += simpleJitLimit;
                simpleJitLimit = 0;
                TryTransitionToNextInterpreterExecutionMode();
            }
        }

        VerifyExecutionModeLimits();
    }

    FunctionExecutionStateMachine::ExecutionState FunctionExecutionStateMachine::ModeToState(ExecutionMode mode) const
    {
        switch (mode)
        {
        case ExecutionMode::AutoProfilingInterpreter:
            return ExecutionState::AutoProfilingInterpreter0;

        case ExecutionMode::ProfilingInterpreter:
            return ExecutionState::ProfilingInterpreter0;

        case ExecutionMode::SimpleJit:
            return ExecutionState::SimpleJit;

        case ExecutionMode::FullJit:
            return ExecutionState::FullJit;

        default:
            Assert(!"Unexpected ExecutionMode for ExecutionState");
            // fall through
        case ExecutionMode::Interpreter:
            return ExecutionState::Interpreter;
        }
    }

    ExecutionMode  FunctionExecutionStateMachine::StateToMode(ExecutionState state) const
    {
        switch (state)
        {
        case ExecutionState::AutoProfilingInterpreter0:
        case ExecutionState::AutoProfilingInterpreter1:
            return ExecutionMode::AutoProfilingInterpreter;

        case ExecutionState::ProfilingInterpreter0:
        case ExecutionState::ProfilingInterpreter1:
            return ExecutionMode::ProfilingInterpreter;

        case ExecutionState::SimpleJit:
            return ExecutionMode::SimpleJit;

        case ExecutionState::FullJit:
            return ExecutionMode::FullJit;

        default:
            Assert(!"Unexpected ExecutionState for ExecutionMode");
            // fall through
        case ExecutionState::Interpreter:
            return ExecutionMode::Interpreter;
        }
    }

    uint16& FunctionExecutionStateMachine::GetStateLimit(ExecutionState state)
    {
        switch (state)
        {
        case ExecutionState::Interpreter:
            return interpreterLimit;

        case ExecutionState::AutoProfilingInterpreter0:
            return autoProfilingInterpreter0Limit;

        case ExecutionState::AutoProfilingInterpreter1:
            return autoProfilingInterpreter1Limit;

        case ExecutionState::ProfilingInterpreter0:
            return profilingInterpreter0Limit;

        case ExecutionState::ProfilingInterpreter1:
            return profilingInterpreter1Limit;

        case ExecutionState::SimpleJit:
            return simpleJitLimit;

        default:
            Assert(!"Unexpected ExecutionState for limit");
            return interpreterLimit;
        }
    }

    // An execution state is terminal if the current FunctionExecutionStateMachine's limits
    // allow the state to continue to run.
    // FullJit is always a terminal state and is the last terminal state.
    bool FunctionExecutionStateMachine::IsTerminalState(ExecutionState state)
    {
        return state == ExecutionState::FullJit || GetStateLimit(state) != 0;
    }

    // Safely moves from one execution mode to another and updates appropriate class members for the next
    // mode. Note that there are other functions that modify execution state that do not involve this function.
    // This function transitions ExecutionMode as ExecutionState in the following order:
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
    // Returns true when a transition occurs (i.e., the execution state was updated since the beginning of
    // this function call). Otherwise, returns false to indicate no change in state.
    // See more details of each mode in ExecutionModes.h
    bool FunctionExecutionStateMachine::TryTransitionToNextExecutionMode()
    {
        Assert(initializedExecutionModeAndLimits);

        bool isStateChanged = false;
        if (executionState != ExecutionState::FullJit)
        {
            bool isTransitionNeeded;
            uint16& stateLimit = GetStateLimit(executionState);
            FunctionEntryPointInfo *const simpleJitEntryPointInfo = owner->GetSimpleJitEntryPointInfo();

            // Determine if the current state should not transition when
            // - for non-JITed states, the interpreted count is less than the limit
            // - for JITed states (specifically, SimpleJIT because it can transition), the callsCount
            //   is non-zero. CallsCount starts at the limit and decrements to 0 to indicate transition.
            if ((executionState != ExecutionState::SimpleJit && GetInterpretedCount() < stateLimit)
                || (simpleJitEntryPointInfo != nullptr && simpleJitEntryPointInfo->callsCount > 0))
            {
                // Since the current state is under its limit, no transition is needed.
                // Simply verify the current state's execution mode before returning.
                isTransitionNeeded = false;
            }
            else
            {
                // Since the current state's limit is reached, transition from this state to the next state
                // First, save data from the current state
                CommitExecutedIterations(stateLimit, stateLimit);

                // Then, reset data for the next state
                SetInterpretedCount(0);

                isTransitionNeeded = true;
            }

            if (isTransitionNeeded)
            {
                // Keep advancing the state until a terminal state is found or until there are no more 
                // states to reach. The path of advancement is described in the banner comment above.
                ExecutionState newState = executionState;
                while (isTransitionNeeded && !IsTerminalState(newState))
                {
                    if (newState != ExecutionState::Interpreter)
                    {
                        // Most states simply advance to the next state
                        newState = static_cast<ExecutionState>(static_cast<uint8>(newState) + 1);
                    }
                    else
                    {
                        // Interpreter advances straight to FullJit
                        newState = ExecutionState::FullJit;
                    }

                    // If FullJit is the next state, but FullJit is disabled, then no transition
                    // is needed.
                    if (newState == ExecutionState::FullJit && PHASE_OFF(FullJitPhase, owner))
                    {
                        isTransitionNeeded = false;
                    }
                    else
                    {
                        // Otherwise, transition is needed because there is new state available
                        isTransitionNeeded = true;
                    }
                }

                // Only update the execution state when the new state is a terminal state
                if (isTransitionNeeded && IsTerminalState(newState))
                {
                    Assert(newState != executionState);
                    SetExecutionState(newState);
                    isStateChanged = true;
                }
            }
        }

        return isStateChanged;
    }
    
    void FunctionExecutionStateMachine::TryTransitionToNextInterpreterExecutionMode()
    {
        Assert(IsInterpreterExecutionMode());

        TryTransitionToNextExecutionMode();
        SetExecutionState(ModeToState(GetInterpreterExecutionMode(false)));
    }

    bool FunctionExecutionStateMachine::TryTransitionToJitExecutionMode()
    {
        const ExecutionMode previousExecutionMode = GetExecutionMode();

        TryTransitionToNextExecutionMode();
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
            owner->TraceExecutionMode();
        }
        return true;
    }

    void FunctionExecutionStateMachine::TransitionToSimpleJitExecutionMode()
    {
        CommitExecutedIterations();

        interpreterLimit = 0;
        autoProfilingInterpreter0Limit = 0;
        profilingInterpreter0Limit = 0;
        autoProfilingInterpreter1Limit = 0;
        fullJitThreshold = simpleJitLimit + profilingInterpreter1Limit;

        VerifyExecutionModeLimits();
        SetExecutionState(ExecutionState::SimpleJit);
    }

    void FunctionExecutionStateMachine::TransitionToFullJitExecutionMode()
    {
        CommitExecutedIterations();

        interpreterLimit = 0;
        autoProfilingInterpreter0Limit = 0;
        profilingInterpreter0Limit = 0;
        autoProfilingInterpreter1Limit = 0;
        simpleJitLimit = 0;
        profilingInterpreter1Limit = 0;
        fullJitThreshold = 0;

        VerifyExecutionModeLimits();
        SetExecutionState(ExecutionState::FullJit);
    }

    void FunctionExecutionStateMachine::SetIsSpeculativeJitCandidate()
    {
        // This function is a candidate for speculative JIT. Ensure that it is profiled immediately by transitioning out of the
        // auto-profiling interpreter mode.
        if (GetExecutionMode() != ExecutionMode::AutoProfilingInterpreter || GetProfiledIterations() != 0)
        {
            return;
        }

        owner->TraceExecutionMode("IsSpeculativeJitCandidate (before)");

        if (autoProfilingInterpreter0Limit != 0)
        {
            (profilingInterpreter0Limit == 0 ? profilingInterpreter0Limit : autoProfilingInterpreter1Limit) +=
                autoProfilingInterpreter0Limit;
            autoProfilingInterpreter0Limit = 0;
        }
        else if (profilingInterpreter0Limit == 0)
        {
            profilingInterpreter0Limit += autoProfilingInterpreter1Limit;
            autoProfilingInterpreter1Limit = 0;
        }

        owner->TraceExecutionMode("IsSpeculativeJitCandidate");

        TryTransitionToNextInterpreterExecutionMode();
    }

    void FunctionExecutionStateMachine::ResetSimpleJitLimit()
    {
        Assert(initializedExecutionModeAndLimits);

        SetExecutionState(ExecutionState::SimpleJit);

        const uint16 simpleJitNewLimit = static_cast<uint8>(Configuration::Global.flags.SimpleJitLimit);
        if (simpleJitLimit < simpleJitNewLimit)
        {
            fullJitThreshold += simpleJitNewLimit - simpleJitLimit;
            simpleJitLimit = simpleJitNewLimit;
        }

        SetInterpretedCount(0);
    }

    uint16 FunctionExecutionStateMachine::GetProfiledIterations() const
    {
        Assert(initializedExecutionModeAndLimits);

        uint16 profiledIterations = committedProfiledIterations;
        switch (GetExecutionMode())
        {
        case ExecutionMode::ProfilingInterpreter:
        {
            uint32 interpretedCount = GetInterpretedCount();
            const uint16 clampedInterpretedCount =
                interpretedCount <= UINT16_MAX
                ? static_cast<uint16>(interpretedCount)
                : UINT16_MAX;
            const uint16 newProfiledIterations = profiledIterations + clampedInterpretedCount;
            profiledIterations = newProfiledIterations >= profiledIterations ? newProfiledIterations : UINT16_MAX;
            break;
        }

        case ExecutionMode::SimpleJit:
            if (!CONFIG_FLAG(NewSimpleJit))
            {
                const uint16 newProfiledIterations = profiledIterations + GetSimpleJitExecutedIterations();
                profiledIterations = newProfiledIterations >= profiledIterations ? newProfiledIterations : UINT16_MAX;
            }
            break;
        }
        return profiledIterations;
    }

    void FunctionExecutionStateMachine::CommitExecutedIterations()
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
                owner->GetSimpleJitEntryPointInfo()
                ? profilingInterpreter1Limit
                : profilingInterpreter0Limit,
                GetInterpretedCount());
            break;

        case ExecutionMode::SimpleJit:
            CommitExecutedIterations(simpleJitLimit, GetSimpleJitExecutedIterations());
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

    void FunctionExecutionStateMachine::VerifyExecutionMode(const ExecutionMode executionMode) const
    {
#if DBG
        Assert(initializedExecutionModeAndLimits);
        Assert(executionMode < ExecutionMode::Count);

        switch (executionMode)
        {
        case ExecutionMode::Interpreter:
            Assert(!owner->DoInterpreterProfile());
            break;

        case ExecutionMode::AutoProfilingInterpreter:
            Assert(owner->DoInterpreterProfile());
            Assert(owner->DoInterpreterAutoProfile());
            break;

        case ExecutionMode::ProfilingInterpreter:
            Assert(owner->DoInterpreterProfile());
            break;

        case ExecutionMode::SimpleJit:
            Assert(owner->DoSimpleJit());
            break;

        case ExecutionMode::FullJit:
            Assert(!PHASE_OFF(FullJitPhase, owner));
            break;

        default:
            Assert(false);
            __assume(false);
        }
#else
        UNREFERENCED_PARAMETER(executionMode);
#endif
    }

    void FunctionExecutionStateMachine::PrintLimits() const
    {
        Output::Print(
            _u("limits: %hu.%hu.%hu.%hu.%hu = %hu"),
            interpreterLimit + autoProfilingInterpreter0Limit,
            profilingInterpreter0Limit,
            autoProfilingInterpreter1Limit,
            simpleJitLimit,
            profilingInterpreter1Limit,
            fullJitThreshold);
    }

    void FunctionExecutionStateMachine::AssertIsInitialized() const
    {
#if DBG
        Assert(initializedExecutionModeAndLimits);
        Assert(owner != nullptr);
#endif
    }
}

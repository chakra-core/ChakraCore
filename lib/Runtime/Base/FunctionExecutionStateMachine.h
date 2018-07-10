//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
#if DBG && defined(ENABLE_SCRIPT_DEBUGGING)
    enum DebuggerMode : unsigned int;
#endif

    class FunctionExecutionStateMachine
    {
    public:
        FunctionExecutionStateMachine();
        void InitializeExecutionModeAndLimits(FunctionBody* functionBody);
        void ReinitializeExecutionModeAndLimits(FunctionBody* functionBody);

        // Public Getters and Setters
        ExecutionMode GetExecutionMode() const;
        void SetDefaultInterpreterExecutionMode();
        void SetAsmJsExecutionMode();

        // Interpreter-related functions
        ExecutionMode GetInterpreterExecutionMode(const bool isPostBailout);
        bool IsInterpreterExecutionMode() const;
        uint16 GetProfiledIterations() const;
        uint32 GetInterpretedCount() const { return interpretedCount; }
        uint32 SetInterpretedCount(uint32 val) { return interpretedCount = val; }
        uint32 IncreaseInterpretedCount() { return interpretedCount++; }
        bool InterpretedSinceCallCountCollection() const;
        void CollectInterpretedCounts();
        void CommitExecutedIterations();

        // JIT-relatedfunctions
        void SetIsSpeculativeJitCandidate();
        uint16 GetSimpleJitLimit() const { return simpleJitLimit; }
        void ResetSimpleJitLimit();
        uint16 GetSimpleJitExecutedIterations() const;
        void SetSimpleJitCallCount(const uint16 simpleJitLimit) const;
        void SetFullJitThreshold(const uint16 newFullJitThreshold, const bool skipSimpleJit = false);
        uint16 GetFullJitThreshold() const { return fullJitThreshold; }
        void SetFullJitRequeueThreshold(const uint16 newFullJitRequeueThreshold);

        // Transition functions
        bool TryTransitionToNextExecutionMode();
        void TryTransitionToNextInterpreterExecutionMode();
        bool TryTransitionToJitExecutionMode();
        void TransitionToSimpleJitExecutionMode();
        void TransitionToFullJitExecutionMode();
        
        // Debug functions
        void PrintLimits() const;
        void AssertIsInitialized() const;

    private:
        // This enum creates a linear view of the progression of ExecutionModes, as
        // described in the banner comment for TryTransitionToNextExecutionMode. Each
        // state has an associated member variable for the state's limit.
        // For more information about ExecutionModes, see ExecutionModes.h.
        enum class ExecutionState : uint8
        {
            Interpreter,
            AutoProfilingInterpreter0,
            ProfilingInterpreter0,
            AutoProfilingInterpreter1,
            SimpleJit,
            ProfilingInterpreter1,
            FullJit
        };


        ExecutionState ModeToState(ExecutionMode mode)   const;
        ExecutionMode  StateToMode(ExecutionState state) const;

        uint16& GetStateLimit(ExecutionState state);
        bool IsTerminalState(ExecutionState state);
        void SetExecutionState(ExecutionState state);
        ExecutionState GetDefaultInterpreterExecutionState() const;

        void CommitExecutedIterations(uint16 &limit, const uint executedIterations);
        
        void VerifyExecutionMode(const ExecutionMode executionMode) const;
        void VerifyExecutionModeLimits() const;
        
        // This state machine should be a member of this owner FunctionBody
        FieldWithBarrier(FunctionBody*) owner;

        // Tracks the current execution state. See ExecutionModes.h for more info.
        FieldWithBarrier(ExecutionState) executionState;

        // Each of the following limits below is decremented when transitioning from its related mode:
        // Number of times to run interpreter (no profiling) before advancing to next mode
        FieldWithBarrier(uint16) interpreterLimit;
        // Number of times to run interpreter (min profiling) before advancing to next mode
        FieldWithBarrier(uint16) autoProfilingInterpreter0Limit;
        // Number of times to run interpreter (full profiling) before advancing to next mode
        FieldWithBarrier(uint16) profilingInterpreter0Limit;
        // Number of times to run interpreter (min profiling) after already running min and full profiling
        FieldWithBarrier(uint16) autoProfilingInterpreter1Limit;
        // Number of times to run simple JIT before advancing to next mode
        FieldWithBarrier(uint16) simpleJitLimit;
        // Number of times to run interpreter (full profiling) before advancing to next mode
        FieldWithBarrier(uint16) profilingInterpreter1Limit;

        // Total limit to run in non-full JIT execution mode. Typically the sum of the other limits
        FieldWithBarrier(uint16) fullJitThreshold;
        // Number of attempts to schedule FullJIT until it becomes forced
        FieldWithBarrier(uint16) fullJitRequeueThreshold;
        // Total number of times this function has run under the interpreter with full profiling
        FieldWithBarrier(uint16) committedProfiledIterations;
        // Number of times this function has run under the interpreter in the current execution mode
        FieldWithBarrier(uint32) interpretedCount;
        // Used to detect when interpretedCount changed from a particular call
        FieldWithBarrier(uint32) lastInterpretedCount;

#if DBG
        FieldWithBarrier(bool) initializedExecutionModeAndLimits;
        // Temporary debug flags for automation
        FieldWithBarrier(bool) hasBeenReinitialized;
#ifdef ENABLE_SCRIPT_DEBUGGING
        FieldWithBarrier(Js::DebuggerMode) initDebuggerMode;
        FieldWithBarrier(Js::DebuggerMode) reinitDebuggerMode;
#endif
#endif
    };
};

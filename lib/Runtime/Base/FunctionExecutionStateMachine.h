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
        void InitializeExecutionModeAndLimits(FunctionBody* functionBody);
        void ReinitializeExecutionModeAndLimits(FunctionBody* functionBody);

        // Public Getters and Setters
        ExecutionMode GetExecutionMode() const;
        void SetExecutionMode(ExecutionMode mode); // This should eventually become private
        ExecutionMode GetDefaultInterpreterExecutionMode(FunctionBody* functionBody) const;
        ExecutionMode GetInterpreterExecutionMode(const bool isPostBailout, FunctionBody* functionBody);
        bool IsInterpreterExecutionMode() const;

        uint32 GetInterpretedCount() const { return interpretedCount; }
        uint32 SetInterpretedCount(uint32 val) { return interpretedCount = val; }
        uint32 IncreaseInterpretedCount() { return interpretedCount++; }
        void CommitExecutedIterations(FunctionBody* functionBody);

        uint16 GetSimpleJitExecutedIterations(FunctionBody* functionBody) const;
        void SetFullJitThreshold(const uint16 newFullJitThreshold, FunctionBody* functionBody, const bool skipSimpleJit = false);
        void SetSimpleJitCallCount(const uint16 simpleJitLimit, FunctionBody* functionBody) const;

        // Transition functions
        bool TryTransitionToNextExecutionMode(FunctionBody* functionBody);
        void TryTransitionToNextInterpreterExecutionMode(FunctionBody* functionBody);
        bool TryTransitionToJitExecutionMode(FunctionBody* functionBody);
        void TransitionToSimpleJitExecutionMode(FunctionBody* functionBody);
        void TransitionToFullJitExecutionMode(FunctionBody* functionBody);


    private:
        void VerifyExecutionMode(const ExecutionMode executionMode, FunctionBody* functionBody) const;
        void VerifyExecutionModeLimits() const;
        
        void CommitExecutedIterations(uint16 &limit, const uint executedIterations);


        // Tracks the current execution mode. See ExecutionModes.h for more info.
        FieldWithBarrier(ExecutionMode) executionMode;

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

#if DBG
        FieldWithBarrier(bool) initializedExecutionModeAndLimits : 1;
#endif
    };
};

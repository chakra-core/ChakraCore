//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#pragma warning(disable:26434) // Function definition hides non-virtual function in base class
#pragma warning(disable:26439) // Implicit noexcept
#pragma warning(disable:26451) // Arithmetic overflow
#pragma warning(disable:26495) // Uninitialized member variable
#include "catch.hpp"
#include "FunctionExecutionTest.h"

// Definition of tests that exercise FunctionExecutionStateMachine
namespace FunctionExecutionTest
{
    // Simple test case to validate that the state machine was created.
    TEST_CASE("FuncExe_BasicTest")
    {
        Js::FunctionExecutionStateMachine f;
        Js::FunctionBody body(false, false, false);
        f.InitializeExecutionModeAndLimits(&body);
        CHECK(f.GetExecutionMode() == ExecutionMode::Interpreter);
    }

    // Mimics script that repeatedly calls a function (not in a loop, but multiple times). This pattern hits all execution modes:
    //ExecutionMode - function : testfn((#1.1), #2), mode : AutoProfilingInterpreter, size : 36, limits : 12.4.89.21.0 = 126, event : IsSpeculativeJitCandidate(before)
    //ExecutionMode - function : testfn((#1.1), #2), mode : AutoProfilingInterpreter, size : 36, limits : 0.4.101.21.0 = 126, event : IsSpeculativeJitCandidate
    //ExecutionMode - function : testfn((#1.1), #2), mode : ProfilingInterpreter, size : 36, limits : 0.4.101.21.0 = 126
    //ExecutionMode - function : testfn((#1.1), #2), mode : AutoProfilingInterpreter, size : 36, limits : 0.0.101.21.0 = 122
    //ExecutionMode - function : testfn((#1.1), #2), mode : SimpleJit, size : 36, limits : 0.0.0.21.0 = 21
    //ExecutionMode - function : testfn((#1.1), #2), mode : SimpleJit, size : 36, limits : 0.0.0.21.0 = 21
    //ExecutionMode - function : testfn((#1.1), #2), mode : FullJit, size : 36, limits : 0.0.0.0.0 = 0
    template <bool TFullJitPhase, ExecutionMode TFinalMode>
    void NormalExecution()
    {
        bool prevValue = FullJitPhaseOffFlag;
        FullJitPhaseOffFlag = TFullJitPhase;

        // Setup the function environment
        int calls = 0;
        Js::Configuration::Global.flags.SetDefaults();
        Js::FunctionExecutionStateMachine f;
        Js::FunctionBody body(true, true, true);

        // Transition from Interpreter to AutoProfilingInterpreter
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::InitializeExecutionModeAndLimits
        //02 chakra!Js::FunctionBody::MarkScript
        //03 chakra!Js::ByteCodeWriter::End
        //04 chakra!ByteCodeGenerator::EmitOneFunction
        //05 chakra!ByteCodeGenerator::EmitScopeList
        //06 chakra!ByteCodeGenerator::EmitScopeList
        //07 chakra!ByteCodeGenerator::EmitScopeList
        //08 chakra!ByteCodeGenerator::EmitProgram
        //09 chakra!ByteCodeGenerator::Generate
        //0a chakra!GenerateByteCode
        //0b chakra!ScriptEngine::CompileUTF8Core
        //0c chakra!ScriptEngine::CompileUTF8
        //0d chakra!ScriptEngine::DefaultCompile
        //0e chakra!ScriptEngine::CreateScriptBody
        //0f chakra!ScriptEngine::ParseScriptTextCore
        //10 chakra!ScriptEngine::ParseScriptText
        f.InitializeExecutionModeAndLimits(&body);
        CHECK(f.GetExecutionMode() == ExecutionMode::AutoProfilingInterpreter);
        CHECK(f.GetInterpretedCount() == 0);
        f.PrintLimits();

        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextInterpreterExecutionMode
        //03 chakra!Js::FunctionExecutionStateMachine::SetIsSpeculativeJitCandidate
        //04 chakra!Js::FunctionBody::SetIsSpeculativeJitCandidate
        //05 chakra!CodeGenWorkItem::ShouldSpeculativelyJitBasedOnProfile
        //06 chakra!CodeGenWorkItem::ShouldSpeculativelyJit
        //07 chakra!NativeCodeGenerator::GetJobToProcessProactively
        //08 chakra!JsUtil::ForegroundJobProcessor::PrioritizeManagerAndWait<NativeCodeGenerator>
        //09 chakra!JsUtil::JobProcessor::PrioritizeManagerAndWait<NativeCodeGenerator>
        //0a chakra!NativeCodeGenerator::EnterScriptStart
        //0b chakra!NativeCodeGenEnterScriptStart
        //0c chakra!Js::ScriptContext::OnScriptStart
        //0d chakra!Js::JavascriptFunction::CallRootFunctionInternal
        //0e chakra!Js::JavascriptFunction::CallRootFunction
        //0f chakra!ScriptSite::CallRootFunction
        //10 chakra!ScriptSite::Execute
        //11 chakra!ScriptEngine::ExecutePendingScripts
        //12 chakra!ScriptEngine::ParseScriptTextCore
        //13 chakra!ScriptEngine::ParseScriptText
        f.SetIsSpeculativeJitCandidate();
        CHECK(f.GetExecutionMode() == ExecutionMode::ProfilingInterpreter);
        CHECK(f.GetInterpretedCount() == 0);

        // "Run" the function in the interpreter
        for (; calls < 4; calls++)
        {
            f.IncreaseInterpretedCount();
        }
        CHECK(f.GetExecutionMode() == ExecutionMode::ProfilingInterpreter);
        CHECK(f.GetInterpretedCount() == 4);

        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionExecutionStateMachine::TryTransitionToJitExecutionMode
        //03 chakra!Js::FunctionBody::TryTransitionToJitExecutionMode
        //04 chakra!NativeCodeGenerator::Prioritize
        //05 chakra!JsUtil::ForegroundJobProcessor::PrioritizeJob<NativeCodeGenerator, Js::FunctionEntryPointInfo * __ptr64>
        //06 chakra!JsUtil::JobProcessor::PrioritizeJob<NativeCodeGenerator, Js::FunctionEntryPointInfo * __ptr64>
        //07 chakra!NativeCodeGenerator::CheckCodeGen
        //08 chakra!NativeCodeGenerator::CheckCodeGenThunk
        //09 chakra!amd64_CallFunction
        //0a chakra!Js::JavascriptFunction::CallFunction<1>
        //0b chakra!Js::InterpreterStackFrame::OP_CallCommon<Js::OpLayoutDynamicProfile<Js::OpLayoutT_CallIWithICIndex<Js::LayoutSizePolicy<0> > > >
        //0c chakra!Js::InterpreterStackFrame::OP_CallI<Js::OpLayoutDynamicProfile<Js::OpLayoutT_CallIWithICIndex<Js::LayoutSizePolicy<0> > > >
        //0d chakra!Js::InterpreterStackFrame::ProcessUnprofiled
        //0e chakra!Js::InterpreterStackFrame::Process
        //0f chakra!Js::InterpreterStackFrame::InterpreterHelper
        //10 chakra!Js::InterpreterStackFrame::InterpreterThunk
        //11 0x0
        //12 chakra!amd64_CallFunction
        //13 chakra!Js::JavascriptFunction::CallFunction<1>
        //14 chakra!Js::JavascriptFunction::CallRootFunctionInternal
        //15 chakra!Js::JavascriptFunction::CallRootFunction
        //16 chakra!ScriptSite::CallRootFunction
        //17 chakra!ScriptSite::Execute
        //18 chakra!ScriptEngine::ExecutePendingScripts
        //19 chakra!ScriptEngine::ParseScriptTextCore
        //1a chakra!ScriptEngine::ParseScriptText
        f.TryTransitionToJitExecutionMode();
        CHECK(f.GetExecutionMode() == ExecutionMode::AutoProfilingInterpreter);
        CHECK(f.GetInterpretedCount() == 0);

        // "Run" the function in the interpreter
        for (; calls < 105; calls++)
        {
            f.IncreaseInterpretedCount();
        }
        CHECK(f.GetExecutionMode() == ExecutionMode::AutoProfilingInterpreter);
        CHECK(f.GetInterpretedCount() == 0x65);

        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionExecutionStateMachine::TryTransitionToJitExecutionMode
        //03 chakra!Js::FunctionBody::TryTransitionToJitExecutionMode
        //04 chakra!NativeCodeGenerator::Prioritize
        //05 chakra!JsUtil::ForegroundJobProcessor::PrioritizeJob<NativeCodeGenerator, Js::FunctionEntryPointInfo * __ptr64>
        //06 chakra!JsUtil::JobProcessor::PrioritizeJob<NativeCodeGenerator, Js::FunctionEntryPointInfo * __ptr64>
        //07 chakra!NativeCodeGenerator::CheckCodeGen
        //08 chakra!NativeCodeGenerator::CheckCodeGenThunk
        //09 chakra!amd64_CallFunction
        //0a chakra!Js::JavascriptFunction::CallFunction<1>
        //0b chakra!Js::InterpreterStackFrame::OP_CallCommon<Js::OpLayoutDynamicProfile<Js::OpLayoutT_CallIWithICIndex<Js::LayoutSizePolicy<0> > > >
        //0c chakra!Js::InterpreterStackFrame::OP_CallI<Js::OpLayoutDynamicProfile<Js::OpLayoutT_CallIWithICIndex<Js::LayoutSizePolicy<0> > > >
        //0d chakra!Js::InterpreterStackFrame::ProcessUnprofiled
        //0e chakra!Js::InterpreterStackFrame::Process
        //0f chakra!Js::InterpreterStackFrame::InterpreterHelper
        //10 chakra!Js::InterpreterStackFrame::InterpreterThunk
        //11 0x0
        //12 chakra!amd64_CallFunction
        //13 chakra!Js::JavascriptFunction::CallFunction<1>
        //14 chakra!Js::JavascriptFunction::CallRootFunctionInternal
        //15 chakra!Js::JavascriptFunction::CallRootFunction
        //16 chakra!ScriptSite::CallRootFunction
        //17 chakra!ScriptSite::Execute
        //18 chakra!ScriptEngine::ExecutePendingScripts
        //19 chakra!ScriptEngine::ParseScriptTextCore
        //1a chakra!ScriptEngine::ParseScriptText
        f.TryTransitionToJitExecutionMode();
        CHECK(f.GetExecutionMode() == ExecutionMode::SimpleJit);
        CHECK(f.GetInterpretedCount() == 0);

        // Simple JIT EntryPoint Info keeps a count from the limit and decrements per call.
        // Since the stub entry point is initialized to 0, proceed with transition.

        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionBody::TryTransitionToNextExecutionMode
        //03 chakra!NativeCodeGenerator::TransitionFromSimpleJit
        //04 chakra!NativeCodeGenerator::Jit_TransitionFromSimpleJit
        //05 0x0
        //06 chakra!amd64_CallFunction
        //07 chakra!Js::JavascriptFunction::CallFunction<1>
        //08 chakra!Js::InterpreterStackFrame::OP_CallCommon<Js::OpLayoutDynamicProfile<Js::OpLayoutT_CallIWithICIndex<Js::LayoutSizePolicy<0> > > >
        //09 chakra!Js::InterpreterStackFrame::OP_CallI<Js::OpLayoutDynamicProfile<Js::OpLayoutT_CallIWithICIndex<Js::LayoutSizePolicy<0> > > >
        //0a chakra!Js::InterpreterStackFrame::ProcessUnprofiled
        //0b chakra!Js::InterpreterStackFrame::Process
        //0c chakra!Js::InterpreterStackFrame::InterpreterHelper
        //0d chakra!Js::InterpreterStackFrame::InterpreterThunk
        //0e 0x0
        //0f chakra!amd64_CallFunction
        //10 chakra!Js::JavascriptFunction::CallFunction<1>
        //11 chakra!Js::JavascriptFunction::CallRootFunctionInternal
        //12 chakra!Js::JavascriptFunction::CallRootFunction
        //13 chakra!ScriptSite::CallRootFunction
        //14 chakra!ScriptSite::Execute
        //15 chakra!ScriptEngine::ExecutePendingScripts
        //16 chakra!ScriptEngine::ParseScriptTextCore
        //17 chakra!ScriptEngine::ParseScriptText
        f.TryTransitionToNextExecutionMode();
        CHECK(f.GetExecutionMode() == TFinalMode);
        CHECK(f.GetInterpretedCount() == 0);

        FullJitPhaseOffFlag = prevValue;
    }

    TEST_CASE("FuncExe_NormalExecution")
    {
        NormalExecution<false, ExecutionMode::FullJit>();
    }

    TEST_CASE("FuncExe_NormalExecutionNoFullJit")
    {
        NormalExecution<true, ExecutionMode::SimpleJit>();
    }

    // test what happens when we jit a Loop Body
    TEST_CASE("FuncExe_NormalExecutionOfLoop")
    {

    }

    // Emulate/test what happens when we have the cmd args similar to JS unittests as Interpreted:
    // -bvt -BaselineMode  -DumpOnCrash -maxInterpretCount:1 -maxSimpleJitRunCount:1 -bgjit-
    TEST_CASE("FuncExe_JSUnitTestInterpreted")
    {
        Js::Configuration::Global.flags.SetInterpretedValues();
        Js::FunctionExecutionStateMachine f;
        Js::FunctionBody body(true, true, true);

        // to AutoProf
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::InitializeExecutionModeAndLimits
        //02 chakra!Js::FunctionBody::MarkScript
        //03 chakra!Js::ByteCodeWriter::End
        // ...and then to Prof
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextInterpreterExecutionMode
        //03 chakra!Js::FunctionExecutionStateMachine::InitializeExecutionModeAndLimits
        //04 chakra!Js::FunctionBody::MarkScript
        //05 chakra!Js::ByteCodeWriter::End
        f.InitializeExecutionModeAndLimits(&body);
        CHECK(f.GetExecutionMode() == ExecutionMode::ProfilingInterpreter);
        CHECK(f.GetInterpretedCount() == 0);

        // Run the function once under the Interpreter
        f.IncreaseInterpretedCount();

        // to Simple
        //# Call Site
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionExecutionStateMachine::TryTransitionToJitExecutionMode
        //03 chakra!Js::FunctionBody::TryTransitionToJitExecutionMode
        //04 chakra!NativeCodeGenerator::CheckCodeGen
        //05 chakra!NativeCodeGenerator::CheckCodeGenThunk
        //06 chakra!amd64_CallFunction
        f.TryTransitionToJitExecutionMode();
        CHECK(f.GetExecutionMode() == ExecutionMode::SimpleJit);
        CHECK(f.GetInterpretedCount() == 0);

        // Simple JIT EntryPoint Info keeps a count from the limit and decrements per call.
        // Since the stub entry point is initialized to 0, proceed with transition.

        // to full
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionBody::TryTransitionToNextExecutionMode
        //03 chakra!NativeCodeGenerator::TransitionFromSimpleJit
        //04 chakra!NativeCodeGenerator::Jit_TransitionFromSimpleJit
        f.TryTransitionToNextExecutionMode();
        CHECK(f.GetExecutionMode() == ExecutionMode::FullJit);
        CHECK(f.GetInterpretedCount() == 0);
    }

    // Emulate/test what happens when we have the cmd args similar to JS unittests as DynaPogo:
    // -bvt -BaselineMode  -DumpOnCrash -forceNative -off:simpleJit -bgJitDelay:0
    template <bool TFullJitPhase, ExecutionMode TInitialMode, ExecutionMode TFinalMode>
    void JSUnitTestDynapogo()
    {
        bool prevValue = FullJitPhaseOffFlag;
        FullJitPhaseOffFlag = TFullJitPhase;

        Js::Configuration::Global.flags.SetDynaPogoValues();
        Js::FunctionExecutionStateMachine f;
        Js::FunctionBody body(true, true, false);
        // to AutoProf
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::InitializeExecutionModeAndLimits
        //02 chakra!Js::FunctionBody::MarkScript
        //03 chakra!Js::ByteCodeWriter::End
        // ...then FullJit
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextInterpreterExecutionMode
        //03 chakra!Js::FunctionExecutionStateMachine::InitializeExecutionModeAndLimits
        //04 chakra!Js::FunctionBody::MarkScript
        //05 chakra!Js::ByteCodeWriter::End
        // ..then profiling
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextInterpreterExecutionMode
        //02 chakra!Js::FunctionExecutionStateMachine::InitializeExecutionModeAndLimits
        //03 chakra!Js::FunctionBody::MarkScript
        //04 chakra!Js::ByteCodeWriter::End
        f.InitializeExecutionModeAndLimits(&body);
        CHECK(f.GetExecutionMode() == TInitialMode);
        CHECK(f.GetInterpretedCount() == 0);

        // to full
        //00 chakra!Js::FunctionExecutionStateMachine::SetExecutionMode
        //01 chakra!Js::FunctionExecutionStateMachine::TryTransitionToNextExecutionMode
        //02 chakra!Js::FunctionExecutionStateMachine::TryTransitionToJitExecutionMode
        //03 chakra!Js::FunctionBody::TryTransitionToJitExecutionMode
        //04 chakra!NativeCodeGenerator::CheckCodeGen
        //05 chakra!NativeCodeGenerator::CheckCodeGenThunk
        f.TryTransitionToJitExecutionMode();
        CHECK(f.GetExecutionMode() == TFinalMode);
        CHECK(f.GetInterpretedCount() == 0);

        FullJitPhaseOffFlag = prevValue;
    }

    TEST_CASE("FuncExe_JSUnitTestDynapogo")
    {
        JSUnitTestDynapogo<false, ExecutionMode::ProfilingInterpreter, ExecutionMode::FullJit>();
    }

    TEST_CASE("FuncExe_JSUnitTestDynapogoNoFullJit")
    {
        JSUnitTestDynapogo<true, ExecutionMode::AutoProfilingInterpreter, ExecutionMode::AutoProfilingInterpreter>();
    }

    // test what hits TransitionToSimpleJitExecutionMode
    // NativeCodeGenerator::GenerateFunction with the following args
    //      -bgjit- -trace:executionmode -prejit -force:simplejit
    TEST_CASE("FuncExe_TransitionToSimpleJit")
    {

    }

    // test what hits TransitionToFullJitExecutionMode
    // Note: also called from
    // - BailOutRecord::ScheduleFunctionCodeGen, when we rejit after bailout
    // - NativeCodeGenerator::GetJobToProcessProactively, looks like it's for speculative jit
    // - NativeCodeGenerator::GenerateFunction, when prejitting with the following args
    //      -bgjit- -trace:executionmode -prejit
    TEST_CASE("FuncExe_TransitionToFullJit")
    {

    }
}

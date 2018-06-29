//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// This file contains stubs needed to make FunctionExecutionTest successfully compile and link as well
// as a means to emulate behavior of objects that interact with FunctionExecutionStateMachine

#include "..\..\lib\Common\Warnings.h"
#include "..\..\lib\Common\Core\CommonMinMax.h"

#define ENUM_CLASS_HELPERS(x, y)
#include "..\..\lib\Runtime\Language\ExecutionMode.h"

#define FieldWithBarrier(type) type

#define CONFIG_FLAG(flag) 10
#define PHASE_OFF(foo, bar) FunctionExecutionTest::PhaseOff(foo, bar)
#define PHASE_FORCE(foo, bar) false
#define NewSimpleJit 1
#define FullJitPhase 2
#undef DEFAULT_CONFIG_MinSimpleJitIterations
#define DEFAULT_CONFIG_MinSimpleJitIterations 0

namespace FunctionExecutionTest
{
    static bool FullJitPhaseOffFlag = false;
    bool PhaseOff(int phase, void*)
    {
        if (phase == FullJitPhase)
        {
            return FullJitPhaseOffFlag;
        }
        else
        {
            Assert(!"Unknown Phase");
            return false;
        }
    }
}

namespace Js
{
    class ConfigFlagsTable
    {
    public:
        uint16 AutoProfilingInterpreter0Limit;
        uint16 AutoProfilingInterpreter1Limit;
        uint16 ProfilingInterpreter0Limit;
        uint16 ProfilingInterpreter1Limit;
        uint16 SimpleJitLimit;
        bool EnforceExecutionModeLimits;

        void SetDefaults()
        {
            AutoProfilingInterpreter0Limit = 0xc;
            ProfilingInterpreter0Limit = 0x4;
            AutoProfilingInterpreter1Limit = 0x44;
            ProfilingInterpreter1Limit = 0;
            SimpleJitLimit = 0x15;
            EnforceExecutionModeLimits = false;
        }

        void SetInterpretedValues()
        {
            AutoProfilingInterpreter0Limit = 0;
            AutoProfilingInterpreter1Limit = 0;
            ProfilingInterpreter0Limit = 1;
            ProfilingInterpreter1Limit = 0;
            SimpleJitLimit = 1;
            EnforceExecutionModeLimits = true;
        }

        void SetDynaPogoValues()
        {
            AutoProfilingInterpreter0Limit = 0;
            AutoProfilingInterpreter1Limit = 0;
            ProfilingInterpreter0Limit = 0;
            ProfilingInterpreter1Limit = 0;
            SimpleJitLimit = 0;
            EnforceExecutionModeLimits = true;
        }
    };

    enum Phase
    {
        SimpleJitPhase
    };

    class Configuration
    {
        public:
            Configuration() {}
            ConfigFlagsTable flags;
            static Configuration Global;
    };
    Configuration        Configuration::Global;

    class FunctionEntryPointInfo
    {
    public:
        FunctionEntryPointInfo() : callsCount(0) {}
        int callsCount;
    };

    class Output
    {
    public:
        static size_t Print(const char16 *form, ...) { UNREFERENCED_PARAMETER(form);  return 0; }
    };

    class FunctionBody
    {
    public:
        bool DoInterpreterProfile() const { return doInterpreterProfile; }
        bool DoInterpreterAutoProfile() const { return doInterpreterAutoProfile; }
        bool DoSimpleJit() const { return doSimpleJit; }
        uint GetByteCodeCount() const { return 0; }
        uint GetByteCodeInLoopCount() const { return 0; }
        uint GetByteCodeWithoutLDACount() const { return 0; }
        FunctionEntryPointInfo* GetDefaultFunctionEntryPointInfo() { return &defaultInfo; }
        FunctionEntryPointInfo *GetSimpleJitEntryPointInfo() { return &simpleInfo; }
        void TraceExecutionMode(const char *const eventDescription = nullptr) const { UNREFERENCED_PARAMETER(eventDescription); }
        
        FunctionBody(bool interpreterProfile, bool interpreterAutoProfile, bool simpleJit):
            doInterpreterProfile(interpreterProfile),
            doInterpreterAutoProfile(interpreterAutoProfile),
            doSimpleJit(simpleJit)
        {}

    private:
        bool doInterpreterProfile;
        bool doInterpreterAutoProfile;
        bool doSimpleJit;
        FunctionEntryPointInfo defaultInfo;
        FunctionEntryPointInfo simpleInfo;
    };
}

#include "..\..\lib\Runtime\Base\FunctionExecutionStateMachine.h"
#include "..\..\lib\Runtime\Base\FunctionExecutionStateMachine.cpp"

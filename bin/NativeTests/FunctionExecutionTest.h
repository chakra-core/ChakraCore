//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// This file contains stubs needed to make FunctionExecutionTest successfully compile and link as well
// as a means to emulate behavior of objects that interact with FunctionExecutionStateMachine

#include "..\..\lib\Common\Core\CommonMinMax.h"

#define ENUM_CLASS_HELPERS(x, y)
#include "..\..\lib\Runtime\Language\ExecutionMode.h"

#define FieldWithBarrier(type) type

#define CONFIG_FLAG(flag) 10
#define PHASE_OFF(foo, bar) false
#define PHASE_FORCE(foo, bar) false
#define NewSimpleJit
#define FullJitPhase
#define DEFAULT_CONFIG_MinSimpleJitIterations 0

namespace Js
{
    class ConfigFlagsTable
    {
    public:
        int AutoProfilingInterpreter0Limit;
        int AutoProfilingInterpreter1Limit;
        int ProfilingInterpreter0Limit;
        int ProfilingInterpreter1Limit;
        int SimpleJitLimit;
        int EnforceExecutionModeLimits;
    };

    enum Phase
    {
        SimpleJitPhase
    };

    class Configuration
    {
        public:
            Configuration() {}
            ConfigFlagsTable           flags;
            static Configuration        Global;
    };
    Configuration        Configuration::Global;

    class FunctionEntryPointInfo
    {
    public:
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
        bool DoInterpreterProfile() const { return false; }
        bool DoInterpreterAutoProfile() const { return false; }
        bool DoSimpleJit() const { return false; }
        uint GetByteCodeCount() const { return 0; }
        uint GetByteCodeInLoopCount() const { return 0; }
        uint GetByteCodeWithoutLDACount() const { return 0; }
        FunctionEntryPointInfo* GetDefaultFunctionEntryPointInfo() const { return nullptr; }
        FunctionEntryPointInfo *GetSimpleJitEntryPointInfo() const { return nullptr; }
        void TraceExecutionMode(const char *const eventDescription = nullptr) const { UNREFERENCED_PARAMETER (eventDescription); }

    };
}

#include "..\..\lib\Runtime\Base\FunctionExecutionStateMachine.h"
#include "..\..\lib\Runtime\Base\FunctionExecutionStateMachine.cpp"

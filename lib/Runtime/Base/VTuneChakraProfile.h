//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef VTUNE_PROFILING

//
// Encapsulates all VTune Chakra profiling event logging and registration etc..
//
class VTuneChakraProfile
{
public:
    static void Register();
    static void UnRegister();

    static void LogMethodNativeLoadEvent(Js::FunctionBody* body, Js::FunctionEntryPointInfo* entryPoint);
    static void LogLoopBodyLoadEvent(Js::FunctionBody* body, Js::LoopEntryPointInfo* entryPoint, uint16 loopNumber);

    static const utf8char_t DynamicCode[];
    static bool isJitProfilingActive;

private:
    static utf8char_t* GetUrl(Js::FunctionBody* body, size_t* urlLength);
};

#endif

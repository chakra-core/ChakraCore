//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <signal.h>

//
// perf or perf_events is a tool for tracing in linux.
// In order to make sense of perf traces in the presence of a JIT,
// some metadata must be provided describing what memory address ranges
// correspond to what compiled function.
//


namespace PlatformAgnostic
{

//
// Encapsulates all perf event logging and registration related to symbol decoding.
//
class PerfTrace
{
public:
    static void Register();

    static void WritePerfMap();

    static volatile sig_atomic_t mapsRequested;
};

};

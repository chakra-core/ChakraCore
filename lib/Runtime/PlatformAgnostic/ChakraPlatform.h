//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "UnicodeText.h"

// Configure whether we configure a signal handler
// to produce perf-<pid>.map files
#ifndef PERFMAP_TRACE_ENABLED
#define PERFMAP_TRACE_ENABLED 0
#endif

#if PERFMAP_TRACE_ENABLED
#include "PerfTrace.h"
#endif

#include "PlatformAgnostic/DateTime.h"
#include "PlatformAgnostic/AssemblyCommon.h"

#if !defined(_WIN32) && defined(DEBUG)
#include <signal.h> // raise(SIGINT)
#endif

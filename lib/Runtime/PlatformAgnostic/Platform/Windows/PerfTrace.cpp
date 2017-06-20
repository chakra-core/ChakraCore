//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimePlatformAgnosticPch.h"

#include "ChakraPlatform.h"


using namespace Js;

namespace PlatformAgnostic
{

volatile sig_atomic_t PerfTrace::mapsRequested = 0;
  
void PerfTrace::Register()
{
    // TODO: Implement this on Windows?
}

void  PerfTrace::WritePerfMap()
{
    // TODO: Implement this on Windows?
}

}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

// Use nativetests.exe -? to get all command line options

int _cdecl main(int argc, char * const argv[])
{
    return Catch::Session().run(argc, argv);
}
